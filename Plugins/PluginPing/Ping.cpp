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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#pragma warning(disable: 4786)
#pragma warning(disable: 4996)

#include <windows.h>
#include <math.h>
#include <string>
#include <map>
#include <vector>
#include <time.h>
#include <Ipexport.h>
#include <Windns.h>
#include <stdlib.h>
#include "..\..\Library\Export.h"	// Rainmeter's exported functions

/* The exported functions */
extern "C"
{
__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
__declspec( dllexport ) double Update2(UINT id);
__declspec( dllexport ) UINT GetPluginVersion();
__declspec( dllexport ) LPCTSTR GetPluginAuthor();
}

struct pingData
{
	IPAddr destAddr;
	DWORD timeout;
	double timeoutValue;
	DWORD updateRate;
	DWORD updateCounter;
	HANDLE threadHandle;
	double value;
};

typedef struct tagIPINFO
{
	u_char nTtl;			// Time To Live
	u_char nTos;			// Type Of Service
	u_char nIPFlags;		// IP flags
	u_char nOptSize;		// Size of options data
	u_char FAR* pOptions;	// Options data buffer

} IPINFO;

typedef IPINFO* PIPINFO;

static std::map<UINT, pingData*> g_Values;
static CRITICAL_SECTION g_CriticalSection; 
static bool g_Initialized = false;
static HINSTANCE g_ICMPInstance = NULL;

typedef HANDLE (WINAPI *IcmpCreateFile)(VOID);
typedef BOOL (WINAPI *IcmpCloseHandle)(HANDLE);
typedef DWORD (WINAPI *IcmpSendEcho)(HANDLE, DWORD, LPVOID, WORD, PIPINFO, LPVOID, DWORD, DWORD);
typedef DWORD (WINAPI *IcmpSendEcho2)(HANDLE, HANDLE, FARPROC, PVOID, IPAddr, LPVOID, WORD, PIP_OPTION_INFORMATION, LPVOID, DWORD, DWORD);

static IcmpCreateFile g_IcmpCreateFile = NULL;
static IcmpCloseHandle g_IcmpCloseHandle = NULL;
static IcmpSendEcho g_IcmpSendEcho = NULL;
static IcmpSendEcho2 g_IcmpSendEcho2 = NULL;

std::string ConvertToAscii(LPCTSTR str)
{
	std::string szAscii;

	if (str && *str)
	{
		int strLen = (int)wcslen(str) + 1;
		int bufLen = WideCharToMultiByte(CP_ACP, 0, str, strLen, NULL, 0, NULL, NULL);
		if (bufLen > 0)
		{
			char* tmpSz = new char[bufLen];
			tmpSz[0] = 0;
			WideCharToMultiByte(CP_ACP, 0, str, strLen, tmpSz, bufLen, NULL, NULL);
			szAscii = tmpSz;
			delete [] tmpSz;
		}
	}
	return szAscii;
}

/*
  This function is called when the measure is initialized.
  The function must return the maximum value that can be measured. 
  The return value can also be 0, which means that Rainmeter will
  track the maximum value automatically. The parameters for this
  function are:

  instance  The instance of this DLL
  iniFile   The name of the ini-file (usually Rainmeter.ini)
  section   The name of the section in the ini-file for this measure
  id        The identifier for the measure. This is used to identify the measures that use the same plugin.
*/
UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id)
{
	bool valid = false;
	pingData* pData = new pingData;

	if (!g_Initialized)
	{
		InitializeCriticalSection(&g_CriticalSection);

		g_ICMPInstance = LoadLibrary(L"ICMP.DLL");

		if (g_ICMPInstance)
		{
			g_IcmpCreateFile = (IcmpCreateFile)GetProcAddress(g_ICMPInstance, "IcmpCreateFile");
			g_IcmpCloseHandle = (IcmpCloseHandle)GetProcAddress(g_ICMPInstance, "IcmpCloseHandle");
			g_IcmpSendEcho = (IcmpSendEcho)GetProcAddress(g_ICMPInstance,"IcmpSendEcho");
			g_IcmpSendEcho2 = (IcmpSendEcho2)GetProcAddress(g_ICMPInstance,"IcmpSendEcho2");
		}
	
		g_Initialized = true;
	}

	memset(pData, 0, sizeof(pingData));

	/* Read our own settings from the ini-file */
	LPCTSTR data = ReadConfigString(section, L"DestAddress", NULL);
	if (data)
	{
		std::string szData = ConvertToAscii(data);

		pData->destAddr = inet_addr(szData.c_str());
		if (pData->destAddr == INADDR_NONE)
		{
			WSADATA wsaData;
		    if (WSAStartup(0x0101, &wsaData) == 0)
			{
				LPHOSTENT pHost;

				pHost = gethostbyname(szData.c_str());
				if (pHost)
				{
					pData->destAddr = *(DWORD*)pHost->h_addr;
				}
				else
				{
					LSLog(LOG_DEBUG, L"Rainmeter", L"Unable to get the host by name.");
				}

				WSACleanup();
			}
			else
			{
				LSLog(LOG_DEBUG, L"Rainmeter", L"Unable to initialize Windows Sockets.");
			}
		}
		valid = true;
	}

	data = ReadConfigString(section, L"UpdateRate", L"32");
	if (data)
	{
		pData->updateRate = _wtoi(data);
	}

	data = ReadConfigString(section, L"Timeout", L"30000");
	if (data)
	{
		pData->timeout = _wtoi(data);
	}

	data = ReadConfigString(section, L"TimeoutValue", L"30000");
	if (data)
	{
		pData->timeoutValue = wcstod(data, NULL);
	}
	
	if (valid)
	{
		g_Values[id] = pData;
	}

	return pData->timeout;
}

DWORD WINAPI NetworkThreadProc(LPVOID pParam)
{
	if (g_IcmpCreateFile && g_IcmpCloseHandle && g_IcmpSendEcho)
	{
		pingData* pData = (pingData*)pParam;

		const DWORD replySize = sizeof(ICMP_ECHO_REPLY) + 32;
		BYTE* reply = new BYTE[replySize];

		HANDLE hIcmpFile;
		hIcmpFile = g_IcmpCreateFile();

		if (hIcmpFile != INVALID_HANDLE_VALUE)
		{
			DWORD res = g_IcmpSendEcho(
				hIcmpFile,
				pData->destAddr,
				NULL, 
				0,
				NULL,
				reply, 
				replySize,
				pData->timeout);

			g_IcmpCloseHandle(hIcmpFile);
		}

		EnterCriticalSection(&g_CriticalSection);
		ICMP_ECHO_REPLY* pReply = (ICMP_ECHO_REPLY*)reply;
		if (pReply->Status == IP_REQ_TIMED_OUT)
		{
			pData->value = pData->timeoutValue;
		}
		else
		{
			pData->value = pReply->RoundTripTime;
		}

		delete [] reply;
		CloseHandle(pData->threadHandle);
		pData->threadHandle = 0;
		LeaveCriticalSection(&g_CriticalSection);
	}
    return 0;   // thread completed successfully
}

/*
This function is called when new value should be measured.
The function returns the new value.
*/
double Update2(UINT id)
{
	double value = 0.0;

	std::map<UINT, pingData*>::iterator i = g_Values.find(id);
	if (i != g_Values.end())
	{
		pingData* pData = (*i).second;

		EnterCriticalSection(&g_CriticalSection);
		value = pData->value;
		LeaveCriticalSection(&g_CriticalSection);

		if (pData->threadHandle == NULL)
		{
			if (pData->updateCounter == 0)
			{
				// Launch a new thread to fetch the web data
				DWORD id;
				pData->threadHandle = CreateThread(NULL, 0, NetworkThreadProc, pData, 0, &id);
			}

			pData->updateCounter++;
			if (pData->updateCounter >= pData->updateRate)
			{
				pData->updateCounter = 0;
			}
		}
	}
	return value;
}

/*
  If the measure needs to free resources before quitting.
  The plugin can export Finalize function, which is called
  when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	std::map<UINT, pingData*>::iterator i1 = g_Values.find(id);
	if (i1 != g_Values.end())
	{
		EnterCriticalSection(&g_CriticalSection);
		if ((*i1).second->threadHandle)
		{
			// Should really wait until the thread finishes instead terminating it...
			LSLog(LOG_DEBUG, L"Rainmeter", L"PingPlugin: Thread still running -> Terminate.");
			TerminateThread((*i1).second->threadHandle, 0);
		}
		LeaveCriticalSection(&g_CriticalSection);

		delete (*i1).second;
		g_Values.erase(i1);
	}

	// Last instance deletes the critical section
	if (g_Values.empty()) 
	{
		if (g_ICMPInstance)
		{
			FreeLibrary(g_ICMPInstance);
			g_ICMPInstance = NULL;
		}

		if (g_Initialized)
		{
			DeleteCriticalSection(&g_CriticalSection);
			g_Initialized = false;
		}
	}
}

UINT GetPluginVersion()
{
	return 1002;
}

LPCTSTR GetPluginAuthor()
{
	return L"Rainy (rainy@iki.fi)";
}