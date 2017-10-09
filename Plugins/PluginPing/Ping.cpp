/* Copyright (C) 2005 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include <windows.h>
#include <Winsock2.h>
#include <string>
#include <Ipexport.h>
#include <Icmpapi.h>
#include "../../Library/Export.h"	// Rainmeter's exported functions

struct MeasureData
{
	IPAddr destAddr;
	DWORD timeout;
	double timeoutValue;
	DWORD updateRate;
	DWORD updateCounter;
	bool threadActive;
	double value;
	std::wstring finishAction;
	void* skin;

	MeasureData() :
		destAddr(),
		timeout(),
		timeoutValue(),
		updateRate(),
		updateCounter(),
		threadActive(false),
		value(),
		finishAction(),
		skin(nullptr)
	{
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
	MeasureData* measure = new MeasureData;
	*data = measure;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;

	LPCWSTR value = RmReadString(rm, L"DestAddress", L"");
	if (*value)
	{
		int strLen = (int)wcslen(value) + 1;
		int bufLen = WideCharToMultiByte(CP_ACP, 0, value, strLen, nullptr, 0, nullptr, nullptr);
		if (bufLen > 0)
		{
			char* buffer = new char[bufLen];
			WideCharToMultiByte(CP_ACP, 0, value, strLen, buffer, bufLen, nullptr, nullptr);

			measure->destAddr = inet_addr(buffer);
			if (measure->destAddr == INADDR_NONE)
			{
				WSADATA wsaData;
				if (WSAStartup(0x0101, &wsaData) == 0)
				{
					LPHOSTENT pHost = gethostbyname(buffer);
					if (pHost)
					{
						measure->destAddr = *(DWORD*)pHost->h_addr;
					}
					else
					{
						RmLog(rm, LOG_WARNING, L"PingPlugin.dll: Unable to get host by name");
					}

					WSACleanup();
				}
				else
				{
					RmLog(rm, LOG_WARNING, L"PingPlugin.dll: Unable to start WSA");
				}
			}

			delete [] buffer;
		}
	}

	measure->updateRate = RmReadInt(rm, L"UpdateRate", 32);
	measure->timeout = RmReadInt(rm, L"Timeout", 30000);
	measure->timeoutValue = RmReadDouble(rm, L"TimeoutValue", 30000.0);
	measure->finishAction = RmReadString(rm, L"FinishAction", L"", false);
	measure->skin = RmGetSkin(rm);
}

DWORD WINAPI NetworkThreadProc(void* pParam)
{
	// NOTE: Do not use CRT functions (since thread was created by CreateThread())!

	MeasureData* measure = (MeasureData*)pParam;
	const DWORD bufferSize = sizeof(ICMP_ECHO_REPLY) + 32;
	BYTE buffer[bufferSize];

	double value = 0.0;
	HANDLE hIcmpFile = IcmpCreateFile();
	if (hIcmpFile != INVALID_HANDLE_VALUE)
	{
		IcmpSendEcho(hIcmpFile, measure->destAddr, nullptr, 0, nullptr, buffer, bufferSize, measure->timeout);
		IcmpCloseHandle(hIcmpFile);

		ICMP_ECHO_REPLY* reply = (ICMP_ECHO_REPLY*)buffer;
		value = (reply->Status != IP_SUCCESS) ? measure->timeoutValue : reply->RoundTripTime;

		if (!measure->finishAction.empty())
		{
			RmExecute(measure->skin, measure->finishAction.c_str());
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
