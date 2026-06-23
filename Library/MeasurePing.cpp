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
#include "AsyncTask.h"
#include <Icmpapi.h>

namespace {

std::wstring LookupPingErrorCode(DWORD errorCode);
std::wstring LookupErrorCode(DWORD errorCode);

}  // namespace

class PingTask : public AsyncTask
{
public:
	typedef void (* ResultCallback)(const PingTask*, void*, double, bool);

	static PingTask* Create(void* requestor, std::wstring destination, DWORD timeout, double timeoutValue, ResultCallback resultCallback)
	{
		auto* task = new PingTask(requestor, std::move(destination), timeout, timeoutValue, resultCallback);
		if (!task->Start())
		{
			delete task;
			return nullptr;
		}

		return task;
	}

private:
	PingTask(void* requestor, std::wstring destination, DWORD timeout, double timeoutValue, ResultCallback resultCallback) :
		AsyncTask(requestor),
		m_Destination(std::move(destination)),
		m_Timeout(timeout),
		m_Value(timeoutValue),
		m_ResultCallback(resultCallback)
	{
	}

	void StartWorkOnWorkerThread() override;
	void FinishWorkOnMainThread() override;

	std::wstring m_Destination;
	DWORD m_Timeout;
	double m_Value;
	ResultCallback m_ResultCallback;
	bool m_DoFinishAction = false;
};

void PingTask::StartWorkOnWorkerThread()
{
	if (m_Destination.empty() || m_AbortRequested)
	{
		return;
	}

	WSADATA wsaData;
	int wsaStartupError = WSAStartup(0x0101, &wsaData);
	if (wsaStartupError != 0)
	{
		LogErrorF(L"Ping: Unable to start WSA (Error %d: %s)", wsaStartupError, LookupErrorCode(wsaStartupError).c_str());
		return;
	}

	PADDRINFOW destAddrInfo = nullptr;

	if (GetAddrInfo(m_Destination.c_str(), nullptr, nullptr, &destAddrInfo) != 0)
	{
		DWORD errorCode = WSAGetLastError();
		LogErrorF(L"Ping: WSA failed for: %s (Error %d: %s)", m_Destination.c_str(), errorCode, LookupErrorCode(errorCode).c_str());
		WSACleanup();
		return;
	}

	bool foundAnAddress = false;
	int i = 0;
	for (PADDRINFOW thisAddrInfo = destAddrInfo; thisAddrInfo != nullptr; thisAddrInfo = thisAddrInfo->ai_next)
	{
		if (thisAddrInfo->ai_family == AF_INET)
		{
			foundAnAddress = true;
		}
		else if (thisAddrInfo->ai_family == AF_INET6)
		{
			foundAnAddress = true;
		}
	}

	if (!foundAnAddress)
	{
		LogErrorF(L"Ping: Could not find any IPv4 or IPv6 address for: %s", m_Destination.c_str());
		FreeAddrInfo(destAddrInfo);
		WSACleanup();
		return;
	}

	bool useIPv6 = false;
	struct sockaddr* destAddr = nullptr;
	for (PADDRINFOW thisAddrInfo = destAddrInfo; thisAddrInfo != nullptr; thisAddrInfo = thisAddrInfo->ai_next)
	{
		if (thisAddrInfo->ai_family == AF_INET || thisAddrInfo->ai_family == AF_INET6)
		{
			destAddr = thisAddrInfo->ai_addr;
			useIPv6 = (thisAddrInfo->ai_family == AF_INET6);
			break;
		}
	}

	if (destAddr && !m_AbortRequested)
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
					(struct sockaddr_in6*)destAddr, nullptr, 0, nullptr, buffer, bufferSize, m_Timeout);
			}
			else
			{
				result = IcmpSendEcho2(hIcmpFile, nullptr, nullptr, nullptr, ((struct sockaddr_in*)destAddr)->sin_addr.s_addr,
					nullptr, 0, nullptr, buffer, bufferSize, m_Timeout);
			}

			if (result != 0UL)
			{
				if (useIPv6)
				{
					ICMPV6_ECHO_REPLY* reply = (ICMPV6_ECHO_REPLY*)buffer;
					result = reply->Status;
					if (result == IP_SUCCESS)
					{
						m_Value = (double)reply->RoundTripTime;
					}
				}
				else
				{
					ICMP_ECHO_REPLY* reply = (ICMP_ECHO_REPLY*)buffer;
					result = reply->Status;
					if (result == IP_SUCCESS)
					{
						m_Value = (double)reply->RoundTripTime;
					}
				}
			}
			else
			{
				result = GetLastError();
			}

			if (result != IP_SUCCESS && result != IP_REQ_TIMED_OUT)
			{
				LogDebugF(L"Ping: Ping failed (Error %d: %s)", result, LookupPingErrorCode(result).c_str());
			}

			IcmpCloseHandle(hIcmpFile);

			m_DoFinishAction = true;
		}

		delete [] buffer;
		buffer = nullptr;
	}

	FreeAddrInfo(destAddrInfo);
	WSACleanup();
}

void PingTask::FinishWorkOnMainThread()
{
	if (m_AbortRequested)
	{
		return;
	}

	if (m_ResultCallback)
	{
		m_ResultCallback(this, m_Requestor, m_Value, m_DoFinishAction);
	}
}

MeasurePing::MeasurePing(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Destination(),
	m_Timeout(30000UL),
	m_TimeoutValue(30000.0),
	m_UpdateRate(32UL),
	m_UpdateCounter(0UL),
	m_FinishAction(),
	m_Task(nullptr)
{
}

MeasurePing::~MeasurePing()
{
	if (m_Task)
	{
		m_Task->AbortWhenPossible();
		m_Task = nullptr;
	}
}

void MeasurePing::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	m_Destination = parser.ReadString(section, L"DestAddress", L"");
	m_UpdateRate = parser.ReadUInt(section, L"UpdateRate", 32U);
	m_Timeout = parser.ReadUInt(section, L"Timeout", 30000U);
	m_TimeoutValue = parser.ReadFloat(section, L"TimeoutValue", 30000.0);
	m_FinishAction = parser.ReadString(section, L"FinishAction", L"", false);
}

void MeasurePing::UpdateValue()
{
	if (m_Task) return;

	if (m_UpdateCounter == 0UL)
	{
		m_Task = PingTask::Create(
			this, m_Destination, m_Timeout, m_TimeoutValue,
			[](const PingTask* task, void* requestor, double value, bool doFinishAction)
			{
				auto measure = (MeasurePing*)requestor;
				if (measure->m_Task == task)
				{
					measure->m_Task = nullptr;
					measure->m_Value = value;

					if (doFinishAction && !measure->m_FinishAction.empty())
					{
						GetRainmeter().ExecuteCommand(measure->m_FinishAction.c_str(), measure->GetSkin());
					}
				}
			});
	}

	++m_UpdateCounter;
	if (m_UpdateCounter >= m_UpdateRate)
	{
		m_UpdateCounter = 0UL;
	}
}

namespace {

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
