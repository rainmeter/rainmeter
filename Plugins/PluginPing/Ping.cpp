/*
  Copyright (C) 2005 Kimmo Pekkola

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <windows.h>
#include <Ipexport.h>
#include <Icmpapi.h>
#include "../../Library/Export.h"	// Rainmeter's exported functions

#include "../../Library/DisableThreadLibraryCalls.h"	// contains DllMain entry point

struct MeasureData
{
	IPAddr destAddr;
	DWORD timeout;
	double timeoutValue;
	DWORD updateRate;
	DWORD updateCounter;
	HANDLE threadHandle;
	double value;

	MeasureData() :
		destAddr(),
		timeout(),
		timeoutValue(),
		updateRate(),
		updateCounter(),
		threadHandle(),
		value()
	{
	}
};

static CRITICAL_SECTION g_CriticalSection;
static UINT g_Instances = 0;

PLUGIN_EXPORT void Initialize(void** data)
{
	MeasureData* measure = new MeasureData;
	*data = measure;

	if (g_Instances == 0)
	{
		InitializeCriticalSection(&g_CriticalSection);
	}

	++g_Instances;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;

	LPCWSTR value = RmReadString(rm, L"DestAddress", L"");
	if (*value)
	{
		int strLen = (int)wcslen(value) + 1;
		int bufLen = WideCharToMultiByte(CP_ACP, 0, value, strLen, NULL, 0, NULL, NULL);
		if (bufLen > 0)
		{
			char* buffer = new char[bufLen];
			WideCharToMultiByte(CP_ACP, 0, value, strLen, buffer, bufLen, NULL, NULL);

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
						RmLog(LOG_WARNING, L"PingPlugin.dll: Unable to get host by name");
					}

					WSACleanup();
				}
				else
				{
					RmLog(LOG_WARNING, L"PingPlugin.dll: Unable to start WSA");
				}
			}

			delete [] buffer;
		}
	}

	measure->updateRate = RmReadInt(rm, L"UpdateRate", 32);
	measure->timeout = RmReadInt(rm, L"Timeout", 30000);
	measure->timeoutValue = RmReadDouble(rm, L"TimeoutValue", 30000.0);
}

DWORD WINAPI NetworkThreadProc(LPVOID pParam)
{
	MeasureData* measure = (MeasureData*)pParam;

	const DWORD replySize = sizeof(ICMP_ECHO_REPLY) + 32;
	BYTE* reply = new BYTE[replySize];

	HANDLE hIcmpFile = IcmpCreateFile();

	if (hIcmpFile != INVALID_HANDLE_VALUE)
	{
		IcmpSendEcho(hIcmpFile, measure->destAddr, NULL, 0, NULL, reply, replySize, measure->timeout);
		IcmpCloseHandle(hIcmpFile);
	}

	EnterCriticalSection(&g_CriticalSection);

	ICMP_ECHO_REPLY* pReply = (ICMP_ECHO_REPLY*)reply;
	if (pReply->Status == IP_REQ_TIMED_OUT)
	{
		measure->value = measure->timeoutValue;
	}
	else
	{
		measure->value = pReply->RoundTripTime;
	}

	delete [] reply;

	CloseHandle(measure->threadHandle);
	measure->threadHandle = NULL;

	LeaveCriticalSection(&g_CriticalSection);

	return 0;
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	double value = 0.0;

	EnterCriticalSection(&g_CriticalSection);
	value = measure->value;
	LeaveCriticalSection(&g_CriticalSection);

	if (measure->threadHandle == NULL)
	{
		if (measure->updateCounter == 0)
		{
			// Launch a new thread to fetch the web data
			DWORD id;
			measure->threadHandle = CreateThread(NULL, 0, NetworkThreadProc, measure, 0, &id);
		}

		measure->updateCounter++;
		if (measure->updateCounter >= measure->updateRate)
		{
			measure->updateCounter = 0;
		}
	}

	return value;
}

PLUGIN_EXPORT void Finalize(void* data)
{
	MeasureData* measure = (MeasureData*)data;

	EnterCriticalSection(&g_CriticalSection);
	if (measure->threadHandle)
	{
		// Should really wait until the thread finishes instead terminating it...
		TerminateThread(measure->threadHandle, 0);
	}
	LeaveCriticalSection(&g_CriticalSection);

	--g_Instances;
	if (g_Instances == 0)
	{
		DeleteCriticalSection(&g_CriticalSection);
	}

	delete measure;
}