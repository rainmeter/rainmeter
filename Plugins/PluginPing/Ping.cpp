/* Copyright (C) 2005 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include <windows.h>
#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <string>
#include <Ipexport.h>
#include <Icmpapi.h>
#include <Iphlpapi.h>
#include "../../Library/Export.h"	// Rainmeter's exported functions

struct MeasureData
{
	void* rm;
	void* skin;
	double value;
	PADDRINFOW destAddrInfo;
	DWORD timeout;
	double timeoutValue;
	DWORD updateRate;
	DWORD updateCounter;
	bool threadActive;
	std::wstring finishAction;

	MeasureData(void* rm) :
		rm(rm),
		skin(RmGetSkin(rm)),
		value(),
		destAddrInfo(),
		timeout(),
		timeoutValue(),
		updateRate(),
		updateCounter(),
		threadActive(false),
		finishAction()
	{
	}

	~MeasureData()
	{
		Dispose();
	}

	MeasureData(const MeasureData&) = delete;
	MeasureData& operator=(const MeasureData&) = delete;

	void Dispose()
	{
		if (destAddrInfo)
		{
			FreeAddrInfoW(destAddrInfo);
			destAddrInfo = nullptr;
		}
	}
};

static CRITICAL_SECTION g_CriticalSection;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		InitializeCriticalSection(&g_CriticalSection);

		// Disable DLL_THREAD_ATTACH and DLL_THREAD_DETACH notification calls.
		DisableThreadLibraryCalls(hinstDLL);
		break;

	case DLL_PROCESS_DETACH:
		DeleteCriticalSection(&g_CriticalSection);
		break;
	}

	return TRUE;
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData(rm);
	*data = measure;
}

std::wstring lookupErrorCode(DWORD errorCode) {
	LPWSTR lpMsgBuf;

	if (!FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)& lpMsgBuf,
		0,
		NULL))
	{
		return L"";
	}

	std::wstring retval(lpMsgBuf);

	LocalFree(lpMsgBuf);

	return retval;
}

std::wstring lookupPingErrorCode(DWORD errorCode) {
	DWORD bufSize = 1023;
	WCHAR* buf = new WCHAR[(size_t)bufSize + 1]();

	if (GetIpErrorString(errorCode, buf, &bufSize) != NO_ERROR) {
		return lookupErrorCode(errorCode);
	}

	std::wstring retval(buf);

	delete[] buf;

	return retval;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;

	LPCWSTR destination = RmReadString(rm, L"DestAddress", L"");
	if (*destination)
	{
		WSADATA wsaData;
		int wsaStartupError = WSAStartup(0x0101, &wsaData);
		if (wsaStartupError == 0)
		{
			measure->Dispose();

			// Error codes: https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
			if (GetAddrInfoW(destination, nullptr, nullptr, &measure->destAddrInfo) != 0)
			{
				DWORD errorCode = WSAGetLastError();
				RmLogF(rm, LOG_WARNING,
					L"PingPlugin.dll: WSA failed for: %ls (Error %d: %ls)", destination, errorCode, lookupErrorCode(errorCode).c_str());
				measure->Dispose();
			}
			else
			{
				bool foundAnAddress = false;
				int i = 0;
				for (PADDRINFOW thisAddrInfo = measure->destAddrInfo; thisAddrInfo != nullptr; thisAddrInfo = thisAddrInfo->ai_next)
				{
					RmLogF(rm, LOG_DEBUG, L"PingPlugin.dll: Evaluating: %ls (Index: %d)", destination, i++);
					if (thisAddrInfo->ai_family == AF_INET)
					{
						foundAnAddress = true;
						RmLogF(rm, LOG_DEBUG, L"PingPlugin.dll: Found IPv4 address for: %ls", destination);
					}
					else if (thisAddrInfo->ai_family == AF_INET6)
					{
						foundAnAddress = true;
						RmLogF(rm, LOG_DEBUG, L"PingPlugin.dll: Found IPv6 address for: %ls", destination);
					}
				}

				if (!foundAnAddress)
				{
					RmLogF(rm, LOG_WARNING,
						L"PingPlugin.dll: Could not find any IPv4 or IPv6 address for: %ls", destination);
					measure->Dispose();
				}
			}

			WSACleanup();
		}
		else
		{
			RmLogF(rm, LOG_WARNING, L"PingPlugin.dll: Unable to start WSA (Error %d: %ls)", wsaStartupError, lookupErrorCode(wsaStartupError).c_str());
		}
	}

	measure->updateRate = RmReadInt(rm, L"UpdateRate", 32);
	measure->timeout = RmReadInt(rm, L"Timeout", 30000);
	measure->timeoutValue = RmReadDouble(rm, L"TimeoutValue", 30000.0);
	measure->finishAction = RmReadString(rm, L"FinishAction", L"", false);
}

DWORD WINAPI NetworkThreadProc(void* pParam)
{
	// NOTE: Do not use CRT functions (since thread was created by CreateThread())!

	MeasureData* measure = (MeasureData*)pParam;
	double value = measure->timeoutValue;

	if (measure->destAddrInfo)
	{
		bool useIPv6 = false;
		struct sockaddr* destAddr = nullptr;
		for (PADDRINFOW thisAddrInfo = measure->destAddrInfo; thisAddrInfo != nullptr; thisAddrInfo = thisAddrInfo->ai_next)
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
			DWORD bufferSize = (useIPv6 ? sizeof(ICMPV6_ECHO_REPLY) : sizeof(ICMP_ECHO_REPLY)) + 32;
			BYTE* buffer = new BYTE[bufferSize]();

			HANDLE hIcmpFile = (useIPv6 ? Icmp6CreateFile() : IcmpCreateFile());
			if (hIcmpFile != INVALID_HANDLE_VALUE)
			{
				DWORD result = 0;
				if (useIPv6)
				{
					struct sockaddr_in6 sourceAddr;
					sourceAddr.sin6_family = AF_INET6;
					sourceAddr.sin6_port = (USHORT)0;
					sourceAddr.sin6_flowinfo = 0UL;
					sourceAddr.sin6_addr = in6addr_any;

					result = Icmp6SendEcho2(hIcmpFile, nullptr, nullptr, nullptr, &sourceAddr,
						(struct sockaddr_in6*)destAddr, nullptr, 0, nullptr, buffer, bufferSize, measure->timeout);
				}
				else
				{
					result = IcmpSendEcho2(hIcmpFile, nullptr, nullptr, nullptr, ((struct sockaddr_in*)destAddr)->sin_addr.s_addr,
						nullptr, 0, nullptr, buffer, bufferSize, measure->timeout);
				}

				if (result != 0)
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

				if (result != IP_SUCCESS && result != IP_REQ_TIMED_OUT)
				{
					RmLogF(measure->rm, LOG_DEBUG, L"PingPlugin.dll: Ping failed (Error %d: %ls)", result, lookupPingErrorCode(result).c_str());
				}
				IcmpCloseHandle(hIcmpFile);

				if (!measure->finishAction.empty())
				{
					RmExecute(measure->skin, measure->finishAction.c_str());
				}
			}

			delete[] buffer;
		}
	}

	HMODULE module = nullptr;

	EnterCriticalSection(&g_CriticalSection);
	if (measure->threadActive)
	{
		measure->value = value;
		measure->threadActive = false;
	}
	else
	{
		// Thread is not attached to an existing measure any longer, so delete
		// unreferenced data.
		delete measure;

		DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
		GetModuleHandleEx(flags, (LPCWSTR)DllMain, &module);
	}
	LeaveCriticalSection(&g_CriticalSection);

	if (module)
	{
		// Decrement the ref count and possibly unload the module if this is
		// the last instance.
		FreeLibraryAndExitThread(module, 0);
	}

	return 0;
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;

	EnterCriticalSection(&g_CriticalSection);
	if (!measure->threadActive)
	{
		if (measure->updateCounter == 0)
		{
			// Launch a new thread to fetch the web data
			DWORD id;
			HANDLE thread = CreateThread(nullptr, 0, NetworkThreadProc, measure, 0, &id);
			if (thread)
			{
				CloseHandle(thread);
				measure->threadActive = true;
			}
		}

		measure->updateCounter++;
		if (measure->updateCounter >= measure->updateRate)
		{
			measure->updateCounter = 0;
		}
	}

	double value = measure->value;
	LeaveCriticalSection(&g_CriticalSection);

	return value;
}

PLUGIN_EXPORT void Finalize(void* data)
{
	MeasureData* measure = (MeasureData*)data;

	EnterCriticalSection(&g_CriticalSection);
	if (measure->threadActive)
	{
		// Increment ref count of this module so that it will not be unloaded prior to
		// thread completion.
		DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS;
		HMODULE module;
		GetModuleHandleEx(flags, (LPCWSTR)DllMain, &module);

		// Thread will perform cleanup.
		measure->threadActive = false;
	}
	else
	{
		delete measure;
	}
	LeaveCriticalSection(&g_CriticalSection);
}
