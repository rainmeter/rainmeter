/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasurePing.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "Rainmeter.h"
#include "Skin.h"
#include "System.h"
#include <Icmpapi.h>

namespace {

std::wstring LookupPingErrorCode(DWORD errorCode);
std::wstring LookupErrorCode(DWORD errorCode);
DWORD WINAPI NetworkThreadProc(void* pParam);

class PingCriticalSection
{
public:
	PingCriticalSection()
	{
		System::InitializeCriticalSection(&m_CriticalSection);
	}

	~PingCriticalSection()
	{
		DeleteCriticalSection(&m_CriticalSection);
	}

	void Enter()
	{
		EnterCriticalSection(&m_CriticalSection);
	}

	void Leave()
	{
		LeaveCriticalSection(&m_CriticalSection);
	}

private:
	CRITICAL_SECTION m_CriticalSection;
};

PingCriticalSection g_CriticalSection;

struct CriticalSectionLock
{
	CriticalSectionLock()
	{
		g_CriticalSection.Enter();
	}

	~CriticalSectionLock()
	{
		g_CriticalSection.Leave();
	}
};

}  // namespace

struct PingData
{
	PingData(MeasurePing* measure, Skin* skin) :
		measure(measure),
		skin(skin),
		value(0.0),
		destAddrInfo(nullptr),
		timeout(30000UL),
		timeoutValue(30000.0),
		updateRate(32UL),
		updateCounter(0UL),
		threadActive(false),
		finishAction()
	{
	}

	~PingData()
	{
		Dispose();
	}

	PingData(const PingData&) = delete;
	PingData& operator=(const PingData&) = delete;

	void Dispose()
	{
		if (destAddrInfo)
		{
			FreeAddrInfo(destAddrInfo);
			destAddrInfo = nullptr;
		}
	}

	MeasurePing* measure;
	Skin* skin;
	double value;
	PADDRINFOW destAddrInfo;
	DWORD timeout;
	double timeoutValue;
	DWORD updateRate;
	DWORD updateCounter;
	bool threadActive;
	std::wstring finishAction;
};

MeasurePing::MeasurePing(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Data(new PingData(this, skin))
{
}

MeasurePing::~MeasurePing()
{
	if (m_Data)
	{
		CriticalSectionLock lock;
		if (m_Data->threadActive)
		{
			m_Data->measure = nullptr;
			m_Data = nullptr;
		}
		else
		{
			delete m_Data;
			m_Data = nullptr;
		}
	}
}

void MeasurePing::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	const std::wstring destination = parser.ReadString(section, L"DestAddress", L"");
	if (!destination.empty())
	{
		WSADATA wsaData;
		int wsaStartupError = WSAStartup(0x0101, &wsaData);
		if (wsaStartupError == 0)
		{
			m_Data->Dispose();

			// Error codes: https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
			if (GetAddrInfo(destination.c_str(), nullptr, nullptr, &m_Data->destAddrInfo) != 0)
			{
				DWORD errorCode = WSAGetLastError();
				LogWarningF(this, L"Ping: WSA failed for: %s (Error %d: %s)", destination.c_str(), errorCode, LookupErrorCode(errorCode).c_str());
				m_Data->Dispose();
			}
			else
			{
				bool foundAnAddress = false;
				int i = 0;
				for (PADDRINFOW thisAddrInfo = m_Data->destAddrInfo; thisAddrInfo != nullptr; thisAddrInfo = thisAddrInfo->ai_next)
				{
					LogDebugF(this, L"Ping: Evaluating: %s (Index: %d)", destination.c_str(), i++);
					if (thisAddrInfo->ai_family == AF_INET)
					{
						foundAnAddress = true;
						LogDebugF(this, L"Ping: Found IPv4 address for: %s", destination.c_str());
					}
					else if (thisAddrInfo->ai_family == AF_INET6)
					{
						foundAnAddress = true;
						LogDebugF(this, L"Ping: Found IPv6 address for: %s", destination.c_str());
					}
				}

				if (!foundAnAddress)
				{
					LogWarningF(this, L"Ping: Could not find any IPv4 or IPv6 address for: %s", destination.c_str());
					m_Data->Dispose();
				}
			}

			WSACleanup();
		}
		else
		{
			LogWarningF(this, L"Ping: Unable to start WSA (Error %d: %s)", wsaStartupError, LookupErrorCode(wsaStartupError).c_str());
		}
	}

	m_Data->updateRate = parser.ReadUInt(section, L"UpdateRate", 32U);
	m_Data->timeout = parser.ReadUInt(section, L"Timeout", 30000U);
	m_Data->timeoutValue = parser.ReadFloat(section, L"TimeoutValue", 30000.0);
	m_Data->finishAction = parser.ReadString(section, L"FinishAction", L"", false);
}

void MeasurePing::UpdateValue()
{
	CriticalSectionLock lock;
	if (!m_Data->threadActive)
	{
		if (m_Data->updateCounter == 0UL)
		{
			DWORD id = 0UL;
			HANDLE thread = CreateThread(nullptr, 0ULL, NetworkThreadProc, m_Data, 0UL, &id);
			if (thread)
			{
				CloseHandle(thread);
				m_Data->threadActive = true;
			}
		}

		++m_Data->updateCounter;
		if (m_Data->updateCounter >= m_Data->updateRate)
		{
			m_Data->updateCounter = 0UL;
		}
	}

	m_Value = m_Data->value;
}

namespace {

DWORD WINAPI NetworkThreadProc(void* pParam)
{
	PingData* data = (PingData*)pParam;
	double value = data->timeoutValue;
	DWORD pingResult = IP_SUCCESS;

	bool doFinishAction = false;

	if (data->destAddrInfo)
	{
		bool useIPv6 = false;
		struct sockaddr* destAddr = nullptr;
		for (PADDRINFOW thisAddrInfo = data->destAddrInfo; thisAddrInfo != nullptr; thisAddrInfo = thisAddrInfo->ai_next)
		{
			if (thisAddrInfo->ai_family == AF_INET || thisAddrInfo->ai_family == AF_INET6)
			{
				destAddr = thisAddrInfo->ai_addr;
				useIPv6 = (thisAddrInfo->ai_family == AF_INET6);
				break;
			}
		}

		if (destAddr)
		{
			DWORD bufferSize = (useIPv6 ? sizeof(ICMPV6_ECHO_REPLY) : sizeof(ICMP_ECHO_REPLY)) + 32UL;
			BYTE* buffer = new BYTE[bufferSize]();

			HANDLE hIcmpFile = (useIPv6 ? Icmp6CreateFile() : IcmpCreateFile());
			if (hIcmpFile != INVALID_HANDLE_VALUE)
			{
				DWORD result = 0UL;
				if (useIPv6)
				{
					struct sockaddr_in6 sourceAddr = { 0 };
					sourceAddr.sin6_family = AF_INET6;
					sourceAddr.sin6_port = (USHORT)0;
					sourceAddr.sin6_flowinfo = 0UL;
					sourceAddr.sin6_addr = in6addr_any;

					result = Icmp6SendEcho2(hIcmpFile, nullptr, nullptr, nullptr, &sourceAddr,
						(struct sockaddr_in6*)destAddr, nullptr, 0, nullptr, buffer, bufferSize, data->timeout);
				}
				else
				{
					result = IcmpSendEcho2(hIcmpFile, nullptr, nullptr, nullptr, ((struct sockaddr_in*)destAddr)->sin_addr.s_addr,
						nullptr, 0, nullptr, buffer, bufferSize, data->timeout);
				}

				if (result != 0UL)
				{
					if (useIPv6)
					{
						ICMPV6_ECHO_REPLY* reply = (ICMPV6_ECHO_REPLY*)buffer;
						result = reply->Status;
						if (result == IP_SUCCESS)
						{
							value = (double)reply->RoundTripTime;
						}
					}
					else
					{
						ICMP_ECHO_REPLY* reply = (ICMP_ECHO_REPLY*)buffer;
						result = reply->Status;
						if (result == IP_SUCCESS)
						{
							value = (double)reply->RoundTripTime;
						}
					}
				}
				else
				{
					result = GetLastError();
				}

				pingResult = result;
				IcmpCloseHandle(hIcmpFile);

				doFinishAction = true;
			}

			delete [] buffer;
			buffer = nullptr;
		}
	}

	std::wstring finishAction;
	Skin* skin = nullptr;

	{
		CriticalSectionLock lock;
		if (data->measure)
		{
			if (pingResult != IP_SUCCESS && pingResult != IP_REQ_TIMED_OUT)
			{
				LogDebugF(data->measure, L"Ping: Ping failed (Error %d: %s)", pingResult, LookupPingErrorCode(pingResult).c_str());
			}

			data->value = value;
			data->threadActive = false;
			if (doFinishAction && !data->finishAction.empty())
			{
				finishAction = data->finishAction;
				skin = data->skin;
			}
		}
		else
		{
			delete data;
			data = nullptr;
		}
	}

	if (skin && !finishAction.empty())
	{
		GetRainmeter().ExecuteCommand(finishAction.c_str(), skin);
	}

	return 0;
}

std::wstring LookupErrorCode(DWORD errorCode)
{
	LPWSTR lpMsgBuf = nullptr;

	if (!FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpMsgBuf,
		0,
		nullptr))
	{
		return L"";
	}

	std::wstring retval(lpMsgBuf);

	LocalFree(lpMsgBuf);

	return retval;
}

std::wstring LookupPingErrorCode(DWORD errorCode)
{
	DWORD bufferSize = 1023UL;
	WCHAR* buffer = new WCHAR[bufferSize + 1UL]();

	if (GetIpErrorString(errorCode, buffer, &bufferSize) != NO_ERROR)
	{
		delete [] buffer;
		buffer = nullptr;
		return LookupErrorCode(errorCode);
	}

	std::wstring retval(buffer);

	delete [] buffer;
	buffer = nullptr;

	return retval;
}

}  // namespace
