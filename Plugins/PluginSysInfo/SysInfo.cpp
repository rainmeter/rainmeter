/*
  Copyright (C) 2004 Kimmo Pekkola

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
#include <Ras.h>
#include <Iphlpapi.h>
#include "..\..\Library\Export.h"	// Rainmeter's exported functions

/* The exported functions */
extern "C"
{
__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
__declspec( dllexport ) LPCTSTR GetString(UINT id, UINT flags);
__declspec( dllexport ) double Update2(UINT id);
__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
__declspec( dllexport ) UINT GetPluginVersion();
__declspec( dllexport ) LPCTSTR GetPluginAuthor();
}

typedef struct 
{
	int count;						//Number of monitors
	HMONITOR m_Monitors[32];		//Monitor info
	RECT m_MonitorRect[32];			//Monitor rect on virtual screen
	MONITORINFO m_MonitorInfo[32];	//Monitor information
} MULTIMONITOR_INFO;

MULTIMONITOR_INFO m_Monitors = { 0 };

BOOL CheckConnection();
void GetOSVersion(WCHAR* buffer);
void GetOSBits(WCHAR* buffer);
BOOL CALLBACK MyInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

enum TYPE
{
	COMPUTER_NAME,
	USER_NAME,
	WORK_AREA,
	SCREEN_SIZE,
	RAS_STATUS,
	OS_VERSION,
	OS_BITS,
	ADAPTER_DESCRIPTION,
	NET_MASK,
	IP_ADDRESS,
	GATEWAY_ADDRESS,
	HOST_NAME,
	DOMAIN_NAME,
	DNS_SERVER,

	WORK_AREA_TOP,
	WORK_AREA_LEFT,
	WORK_AREA_WIDTH,
	WORK_AREA_HEIGHT,
	SCREEN_WIDTH,
	SCREEN_HEIGHT,
	NUM_MONITORS,
	VIRTUAL_SCREEN_TOP,
	VIRTUAL_SCREEN_LEFT,
	VIRTUAL_SCREEN_WIDTH,
	VIRTUAL_SCREEN_HEIGHT,
};

static std::map<UINT, TYPE> g_Types;
static std::map<UINT, UINT> g_Datas;

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
	/* Read our own settings from the ini-file */
	LPCTSTR type = ReadConfigString(section, L"SysInfoType", L"");
	if(type) 
	{
		if (_wcsicmp(L"COMPUTER_NAME", type) == 0)
		{
			g_Types[id] = COMPUTER_NAME;
		} 
		else if (_wcsicmp(L"USER_NAME", type) == 0)
		{
			g_Types[id] = USER_NAME;
		} 
		else if (_wcsicmp(L"WORK_AREA", type) == 0)
		{
			g_Types[id] = WORK_AREA;
		} 
		else if (_wcsicmp(L"SCREEN_SIZE", type) == 0)
		{
			g_Types[id] = SCREEN_SIZE;
		} 
		else if (_wcsicmp(L"RAS_STATUS", type) == 0)
		{
			g_Types[id] = RAS_STATUS;
		} 
		else if (_wcsicmp(L"OS_VERSION", type) == 0)
		{
			g_Types[id] = OS_VERSION;
		} 
		else if (_wcsicmp(L"OS_BITS", type) == 0)
		{
			g_Types[id] = OS_BITS;
		} 
		else if (_wcsicmp(L"ADAPTER_DESCRIPTION", type) == 0)
		{
			g_Types[id] = ADAPTER_DESCRIPTION;
		} 
		else if (_wcsicmp(L"NET_MASK", type) == 0)
		{
			g_Types[id] = NET_MASK;
		} 
		else if (_wcsicmp(L"IP_ADDRESS", type) == 0)
		{
			g_Types[id] = IP_ADDRESS;
		} 
		else if (_wcsicmp(L"GATEWAY_ADDRESS", type) == 0)
		{
			g_Types[id] = GATEWAY_ADDRESS;
		} 
		else if (_wcsicmp(L"HOST_NAME", type) == 0)
		{
			g_Types[id] = HOST_NAME;
		} 
		else if (_wcsicmp(L"DOMAIN_NAME", type) == 0)
		{
			g_Types[id] = DOMAIN_NAME;
		} 
		else if (_wcsicmp(L"DNS_SERVER", type) == 0)
		{
			g_Types[id] = DNS_SERVER;
		} 

		else if (_wcsicmp(L"WORK_AREA_TOP", type) == 0)
		{
			g_Types[id] = WORK_AREA_TOP;
		} 
		else if (_wcsicmp(L"WORK_AREA_LEFT", type) == 0)
		{
			g_Types[id] = WORK_AREA_LEFT;
		} 
		else if (_wcsicmp(L"WORK_AREA_WIDTH", type) == 0)
		{
			g_Types[id] = WORK_AREA_WIDTH;
		} 
		else if (_wcsicmp(L"WORK_AREA_HEIGHT", type) == 0)
		{
			g_Types[id] = WORK_AREA_HEIGHT;
		} 
		else if (_wcsicmp(L"SCREEN_WIDTH", type) == 0)
		{
			g_Types[id] = SCREEN_WIDTH;
		} 
		else if (_wcsicmp(L"SCREEN_HEIGHT", type) == 0)
		{
			g_Types[id] = SCREEN_HEIGHT;
		} 
		else if (_wcsicmp(L"NUM_MONITORS", type) == 0)
		{
			g_Types[id] = NUM_MONITORS;
		} 
		else if (_wcsicmp(L"VIRTUAL_SCREEN_TOP", type) == 0)
		{
			g_Types[id] = VIRTUAL_SCREEN_TOP;
		} 
		else if (_wcsicmp(L"VIRTUAL_SCREEN_LEFT", type) == 0)
		{
			g_Types[id] = VIRTUAL_SCREEN_LEFT;
		} 
		else if (_wcsicmp(L"VIRTUAL_SCREEN_WIDTH", type) == 0)
		{
			g_Types[id] = VIRTUAL_SCREEN_WIDTH;
		} 
		else if (_wcsicmp(L"VIRTUAL_SCREEN_HEIGHT", type) == 0)
		{
			g_Types[id] = VIRTUAL_SCREEN_HEIGHT;
		} 
		else
		{
			std::wstring error = L"SysInfoType=";
			error += type;
			error += L" is not valid in measure [";
			error += section;
			error += L"].";
			MessageBox(NULL, error.c_str(), L"Rainmeter", MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
		}
	}

	LPCTSTR data = ReadConfigString(section, L"SysInfoData", L"0");
	if (data)
	{
		g_Datas[id] = _wtoi(data);
	}

	return 0;
}

std::wstring ConvertToWide(LPCSTR str)
{
	std::wstring szWide;

	if (str && *str)
	{
		int strLen = (int)strlen(str) + 1;
		int bufLen = MultiByteToWideChar(CP_ACP, 0, str, strLen, NULL, 0);
		if (bufLen > 0)
		{
			WCHAR* wideSz = new WCHAR[bufLen];
			wideSz[0] = 0;
			MultiByteToWideChar(CP_ACP, 0, str, strLen, wideSz, bufLen);
			szWide = wideSz;
			delete [] wideSz;
		}
	}
	return szWide;
}

/*
  This function is called when the value should be
  returned as a string.
*/
LPCTSTR GetString(UINT id, UINT flags) 
{
	static WCHAR buffer[4096];
	UINT data;
	DWORD len = 4095;
	std::map<UINT, TYPE>::iterator typeIter = g_Types.find(id);
	std::map<UINT, UINT>::iterator dataIter = g_Datas.find(id);

	if(typeIter == g_Types.end()) return NULL;
	if(dataIter == g_Datas.end())
	{
		data = 0;
	}
	else
	{
		data = (*dataIter).second;
	}

	switch((*typeIter).second)
	{
	case COMPUTER_NAME:
		GetComputerName(buffer, &len);
		return buffer;

	case USER_NAME:
		GetUserName(buffer, &len);
		return buffer;

	case WORK_AREA:
		wsprintf(buffer, L"%i x %i", GetSystemMetrics(SM_CXFULLSCREEN), GetSystemMetrics(SM_CYFULLSCREEN));
		return buffer;

	case SCREEN_SIZE:
		wsprintf(buffer, L"%i x %i", GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		return buffer;

	case RAS_STATUS:
		wsprintf(buffer, L"%s", CheckConnection()?"Online":"Offline");
		return buffer;

	case OS_VERSION:
		GetOSVersion(buffer);
		return buffer;

	case OS_BITS:
		GetOSBits(buffer);
		return buffer;

	case ADAPTER_DESCRIPTION:
		if (ERROR_SUCCESS == GetAdaptersInfo((IP_ADAPTER_INFO*)buffer, &len))
		{
			PIP_ADAPTER_INFO info = (IP_ADAPTER_INFO*)buffer;
			int i = 0;
			while (info)
			{
				if (i == data)
				{
					wcscpy(buffer, ConvertToWide(info->Description).c_str());
					return buffer;
				}
				info = info->Next;
				i++;
			}
		}
		break;

	case IP_ADDRESS:
		if (NO_ERROR == GetIpAddrTable((PMIB_IPADDRTABLE)buffer, &len, FALSE))
		{
			PMIB_IPADDRTABLE ipTable = (PMIB_IPADDRTABLE)buffer;
			if (data >= 1000)
			{
				data = data-999;
				for(UINT i=0; i<ipTable->dwNumEntries; i++)
				{
					if((ipTable->table[i].wType)&MIB_IPADDR_DISCONNECTED) continue;
					data--;
					if(data==0)
					{
						DWORD ip = ipTable->table[i].dwAddr;
						wsprintf(buffer, L"%i.%i.%i.%i", ip%256, (ip>>8)%256, (ip>>16)%256, (ip>>24)%256);
						return buffer;
					}
				}
			}
			else if (data < ipTable->dwNumEntries)
			{
				DWORD ip = ipTable->table[data].dwAddr;
				wsprintf(buffer, L"%i.%i.%i.%i", ip%256, (ip>>8)%256, (ip>>16)%256, (ip>>24)%256);
				return buffer;
			}
		}
		wsprintf(buffer, L"");
		return buffer;
		break;

	case NET_MASK:
		if (NO_ERROR == GetIpAddrTable((PMIB_IPADDRTABLE)buffer, &len, FALSE))
		{
			PMIB_IPADDRTABLE ipTable = (PMIB_IPADDRTABLE)buffer;
			if (data < ipTable->dwNumEntries)
			{
				DWORD ip = ipTable->table[data].dwMask;
				wsprintf(buffer, L"%i.%i.%i.%i", ip%256, (ip>>8)%256, (ip>>16)%256, (ip>>24)%256);
				return buffer;
			}
		}
		break;

	case GATEWAY_ADDRESS:
		if (ERROR_SUCCESS == GetAdaptersInfo((IP_ADAPTER_INFO*)buffer, &len))
		{
			PIP_ADAPTER_INFO info = (IP_ADAPTER_INFO*)buffer;
			int i = 0;
			while (info)
			{
				if (i == data)
				{
					wcscpy(buffer, ConvertToWide(info->GatewayList.IpAddress.String).c_str());
					return buffer;
				}
				info = info->Next;
				i++;
			}
		}
		break;

	case HOST_NAME:
		if (ERROR_SUCCESS == GetNetworkParams((PFIXED_INFO)buffer, &len))
		{
			PFIXED_INFO info = (PFIXED_INFO)buffer;
			wcscpy(buffer, ConvertToWide(info->HostName).c_str());
			return buffer;
		}
		break;

	case DOMAIN_NAME:
		if (ERROR_SUCCESS == GetNetworkParams((PFIXED_INFO)buffer, &len))
		{
			PFIXED_INFO info = (PFIXED_INFO)buffer;
			wcscpy(buffer, ConvertToWide(info->DomainName).c_str());
			return buffer;
		}
		break;

	case DNS_SERVER:
		if (ERROR_SUCCESS == GetNetworkParams((PFIXED_INFO)buffer, &len))
		{
			PFIXED_INFO info = (PFIXED_INFO)buffer;
			if (info->CurrentDnsServer)
			{
				wcscpy(buffer, ConvertToWide(info->CurrentDnsServer->IpAddress.String).c_str());
			}
			else
			{
				wcscpy(buffer, ConvertToWide(info->DnsServerList.IpAddress.String).c_str());
			}
			return buffer;
		}
		break;
	}

	return NULL;
}

/*
  This function is called when new value should be measured.
  The function returns the new value.
*/
double Update2(UINT id)
{
	UINT data;
	std::map<UINT, TYPE>::iterator typeIter = g_Types.find(id);
	std::map<UINT, UINT>::iterator dataIter = g_Datas.find(id);

	if(typeIter == g_Types.end()) return NULL;
	if(dataIter == g_Datas.end())
	{
		data = 0;
	}
	else
	{
		data = (*dataIter).second;
	}

	if(data) //For speed purposes, only check if they specify a non-primary monitor.
	{
		if(GetSystemMetrics(SM_CMONITORS)>32) 
		{
			std::wstring error = L"That's alot of monitors! 32 is the max.";
			MessageBox(NULL, error.c_str(), L"Rainmeter", MB_OK | MB_TOPMOST | MB_ICONERROR);
			exit(-1);
		}
		m_Monitors.count = 0;
		EnumDisplayMonitors(NULL, NULL, MyInfoEnumProc, (LPARAM)(&m_Monitors)); 
	}


	switch((*typeIter).second)
	{
	case WORK_AREA_WIDTH:
		if (data)
			return m_Monitors.m_MonitorInfo[data-1].rcWork.right-m_Monitors.m_MonitorInfo[data-1].rcWork.left;
		else
			return GetSystemMetrics(SM_CXFULLSCREEN);
	case WORK_AREA_HEIGHT:
		if (data)
			return m_Monitors.m_MonitorInfo[data-1].rcWork.bottom-m_Monitors.m_MonitorInfo[data-1].rcWork.top;
		else
			return GetSystemMetrics(SM_CYFULLSCREEN);
	case SCREEN_WIDTH:
		if (data)
			return m_Monitors.m_MonitorInfo[data-1].rcMonitor.right-m_Monitors.m_MonitorInfo[data-1].rcMonitor.left;
		else
			return GetSystemMetrics(SM_CXSCREEN);
	case SCREEN_HEIGHT:
		if (data)
			return m_Monitors.m_MonitorInfo[data-1].rcMonitor.bottom-m_Monitors.m_MonitorInfo[data-1].rcMonitor.top;
		else
			return GetSystemMetrics(SM_CYSCREEN);
	case VIRTUAL_SCREEN_WIDTH:
		return GetSystemMetrics(SM_CXVIRTUALSCREEN);
	case VIRTUAL_SCREEN_HEIGHT:
		return GetSystemMetrics(SM_CYVIRTUALSCREEN);
	case NUM_MONITORS:
		return GetSystemMetrics(SM_CMONITORS);

	/* can be negative */
	case WORK_AREA_TOP:
		if (data)
			return m_Monitors.m_MonitorInfo[data-1].rcWork.top;
		else
			return m_Monitors.m_MonitorInfo[0].rcWork.top;			// guessing that this is the primary monitor
	case WORK_AREA_LEFT:
		if (data)
			return m_Monitors.m_MonitorInfo[data-1].rcWork.left;
		else
			return m_Monitors.m_MonitorInfo[0].rcWork.left;			// guessing that this is the primary monitor
	case VIRTUAL_SCREEN_TOP:	// virtual coords
		if (data)
			return m_Monitors.m_MonitorInfo[data-1].rcMonitor.top;
		else
			return GetSystemMetrics(SM_YVIRTUALSCREEN);				// seems reasonable to return this if they don't specify a monitor
	case VIRTUAL_SCREEN_LEFT:	// virtual coords
		if (data)
			return m_Monitors.m_MonitorInfo[data-1].rcMonitor.left;
		else
			return GetSystemMetrics(SM_XVIRTUALSCREEN);				// seems reasonable to return this if they don't specify a monitor

	}

	return NULL;
}

BOOL CALLBACK MyInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	MULTIMONITOR_INFO *m = (MULTIMONITOR_INFO *)dwData;
	m->m_Monitors[m->count] = hMonitor;
	memcpy(&(m->m_MonitorRect[m->count]),lprcMonitor,sizeof RECT);
	m->m_MonitorInfo[m->count].cbSize = sizeof ( MONITORINFO );
	GetMonitorInfo(hMonitor,&(m->m_MonitorInfo[m->count]));
	m->count++;
	return true;
}

/*
  If the measure needs to free resources before quitting.
  The plugin can export Finalize function, which is called
  when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	std::map<UINT, TYPE>::iterator i1 = g_Types.find(id);
	if (i1 != g_Types.end())
	{
		g_Types.erase(i1);
	}

	std::map<UINT, UINT>::iterator i2 = g_Datas.find(id);
	if (i2 != g_Datas.end())
	{
		g_Datas.erase(i2);
	}
}

/*
  Fills the buffer with OS version
*/
void GetOSVersion(WCHAR* buffer)
{
	OSVERSIONINFOEX version;
	version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO*)&version);

	if (version.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		if (version.dwMajorVersion <= 4)
		{
			wcscpy(buffer, L"Windows NT");
		}
		else if (version.dwMajorVersion == 5)
		{
			if (version.dwMinorVersion == 2)
			{
				wcscpy(buffer, L"Windows 2003");
			}
			else if (version.dwMinorVersion == 1)
			{
				wcscpy(buffer, L"Windows XP");
			}
			else if (version.dwMinorVersion == 0)
			{
				wcscpy(buffer, L"Windows 2000");
			}
			else
			{
				wcscpy(buffer, L"Unknown");
			}
		}
		else
		{
			if (version.dwMinorVersion == 1 && version.wProductType == VER_NT_WORKSTATION)
			{
				wcscpy(buffer, L"Windows 7");
			}
			else if (version.dwMinorVersion == 1 && version.wProductType != VER_NT_WORKSTATION)
			{
				wcscpy(buffer, L"Windows Server 2008 R2");
			}
			else if (version.dwMinorVersion == 0 && version.wProductType == VER_NT_WORKSTATION)
			{
				wcscpy(buffer, L"Windows Vista");
			}
			else if (version.dwMinorVersion == 0 && version.wProductType != VER_NT_WORKSTATION)
			{
				wcscpy(buffer, L"Windows Server 2008");
			}
			else
			{
				wcscpy(buffer, L"Unknown");
			}
		}
	}
	else
	{
		if (version.dwMinorVersion < 10)
		{
			wcscpy(buffer, L"Windows 95");
		}
		else if (version.dwMinorVersion < 90)
		{
			wcscpy(buffer, L"Windows 98");
		}
		else
		{
			wcscpy(buffer, L"Windows ME");
		}
	}
}

void GetOSBits(WCHAR* buffer)
{
	SYSTEM_INFO systemInfo = {0};

	typedef void (WINAPI *FPGETNATIVESYSTEMINFO)(LPSYSTEM_INFO lpSystemInfo);
	FPGETNATIVESYSTEMINFO GetNativeSystemInfo = (FPGETNATIVESYSTEMINFO)GetProcAddress(GetModuleHandle(L"kernel32"), "GetNativeSystemInfo");
	if (GetNativeSystemInfo != NULL)
	{
		GetNativeSystemInfo(&systemInfo);
	}
	else
	{
		GetSystemInfo(&systemInfo);
	}

	if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
		systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
	{
		wcscpy(buffer, L"64");
	}
	else
	{
		wcscpy(buffer, L"32");
	}
}

/*
  Tests if there is a RAS connection or not. Don't know
  If this works or not (especially on Win9x):-(
*/
BOOL CheckConnection()
{
	static HRASCONN g_hRasConn=NULL;
	RASCONNSTATUS rasStatus;
	LPRASCONN lpRasConn=NULL;
    DWORD cbBuf=0;
    DWORD cConn=1;
    DWORD dwRet=0;

	if(g_hRasConn==NULL) {
	    // Enumerate connections
		cbBuf=sizeof(RASCONN);
		if(((lpRasConn=(LPRASCONN)malloc((UINT)cbBuf))!= NULL)) {            
			lpRasConn->dwSize=sizeof(RASCONN);
			if(0==RasEnumConnections(lpRasConn, &cbBuf, &cConn)) {
				if(cConn!=0) {
					g_hRasConn=lpRasConn->hrasconn;
				}
			}
			free(lpRasConn);
		}
	}

	if(g_hRasConn!=NULL) {
		// get connection status
		rasStatus.dwSize=sizeof(RASCONNSTATUS);
		dwRet=RasGetConnectStatus(g_hRasConn, &rasStatus );
		if(dwRet==0) {
			// Check for connection
			if(rasStatus.rasconnstate==RASCS_Connected) return TRUE;
		} else {
			g_hRasConn=NULL;
		}
	}

    return FALSE;
}

UINT GetPluginVersion()
{
	return 1004;
}

LPCTSTR GetPluginAuthor()
{
	return L"Rainy (rainy@iki.fi) - Additions by Mordred (kbuffington@gmail.com)";
}