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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include "../API/RainmeterAPI.h"
#include "../../Library/Export.h"
#include "../../Library/DisableThreadLibraryCalls.h"	// contains DllMain entry point

typedef struct
{
	int count;						// Number of monitors
	HMONITOR m_Monitors[32];		// Monitor info
	RECT m_MonitorRect[32];			// Monitor rect on virtual screen
	MONITORINFO m_MonitorInfo[32];	// Monitor information
} MULTIMONITOR_INFO;

MULTIMONITOR_INFO m_Monitors = {0};

enum MeasureType
{
	MEASURE_COMPUTER_NAME,
	MEASURE_USER_NAME,
	MEASURE_WORK_AREA,
	MEASURE_SCREEN_SIZE,
	MEASURE_RAS_STATUS,
	MEASURE_OS_VERSION,
	MEASURE_OS_BITS,
	MEASURE_ADAPTER_DESCRIPTION,
	MEASURE_NET_MASK,
	MEASURE_IP_ADDRESS,
	MEASURE_GATEWAY_ADDRESS,
	MEASURE_HOST_NAME,
	MEASURE_DOMAIN_NAME,
	MEASURE_DNS_SERVER,
	MEASURE_WORK_AREA_TOP,
	MEASURE_WORK_AREA_LEFT,
	MEASURE_WORK_AREA_WIDTH,
	MEASURE_WORK_AREA_HEIGHT,
	MEASURE_SCREEN_WIDTH,
	MEASURE_SCREEN_HEIGHT,
	MEASURE_NUM_MONITORS,
	MEASURE_VIRTUAL_SCREEN_TOP,
	MEASURE_VIRTUAL_SCREEN_LEFT,
	MEASURE_VIRTUAL_SCREEN_WIDTH,
	MEASURE_VIRTUAL_SCREEN_HEIGHT
};

struct MeasureData
{
	MeasureType type;
	int data;

	MeasureData() : type(), data() {}
};

LPCWSTR GetPlatformName();
BOOL CALLBACK MyInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

bool g_Initialized = false;

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData;
	*data = measure;

	if (!g_Initialized)
	{
		if (GetSystemMetrics(SM_CMONITORS) > 32)
		{
			LSLog(LOG_ERROR, NULL, L"SysInfo.dll: Max amount of monitors supported is 32.");
		}

		m_Monitors.count = 0;
		EnumDisplayMonitors(NULL, NULL, MyInfoEnumProc, (LPARAM)(&m_Monitors));
		g_Initialized = true;
	}
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;

	LPCTSTR type = RmReadString(rm, L"SysInfoType", L"");
	if (_wcsicmp(L"COMPUTER_NAME", type) == 0)
	{
		measure->type = MEASURE_COMPUTER_NAME;
	}
	else if (_wcsicmp(L"USER_NAME", type) == 0)
	{
		measure->type = MEASURE_USER_NAME;
	}
	else if (_wcsicmp(L"WORK_AREA", type) == 0)
	{
		measure->type = MEASURE_WORK_AREA;
	}
	else if (_wcsicmp(L"SCREEN_SIZE", type) == 0)
	{
		measure->type = MEASURE_SCREEN_SIZE;
	}
	else if (_wcsicmp(L"RAS_STATUS", type) == 0)
	{
		measure->type = MEASURE_RAS_STATUS;
	}
	else if (_wcsicmp(L"OS_VERSION", type) == 0)
	{
		measure->type = MEASURE_OS_VERSION;
	}
	else if (_wcsicmp(L"OS_BITS", type) == 0)
	{
		measure->type = MEASURE_OS_BITS;
	}
	else if (_wcsicmp(L"ADAPTER_DESCRIPTION", type) == 0)
	{
		measure->type = MEASURE_ADAPTER_DESCRIPTION;
	}
	else if (_wcsicmp(L"NET_MASK", type) == 0)
	{
		measure->type = MEASURE_NET_MASK;
	}
	else if (_wcsicmp(L"IP_ADDRESS", type) == 0)
	{
		measure->type = MEASURE_IP_ADDRESS;
	}
	else if (_wcsicmp(L"GATEWAY_ADDRESS", type) == 0)
	{
		measure->type = MEASURE_GATEWAY_ADDRESS;
	}
	else if (_wcsicmp(L"HOST_NAME", type) == 0)
	{
		measure->type = MEASURE_HOST_NAME;
	}
	else if (_wcsicmp(L"DOMAIN_NAME", type) == 0)
	{
		measure->type = MEASURE_DOMAIN_NAME;
	}
	else if (_wcsicmp(L"DNS_SERVER", type) == 0)
	{
		measure->type = MEASURE_DNS_SERVER;
	}
	else if (_wcsicmp(L"WORK_AREA_TOP", type) == 0)
	{
		measure->type = MEASURE_WORK_AREA_TOP;
	}
	else if (_wcsicmp(L"WORK_AREA_LEFT", type) == 0)
	{
		measure->type = MEASURE_WORK_AREA_LEFT;
	}
	else if (_wcsicmp(L"WORK_AREA_WIDTH", type) == 0)
	{
		measure->type = MEASURE_WORK_AREA_WIDTH;
	}
	else if (_wcsicmp(L"WORK_AREA_HEIGHT", type) == 0)
	{
		measure->type = MEASURE_WORK_AREA_HEIGHT;
	}
	else if (_wcsicmp(L"SCREEN_WIDTH", type) == 0)
	{
		measure->type = MEASURE_SCREEN_WIDTH;
	}
	else if (_wcsicmp(L"SCREEN_HEIGHT", type) == 0)
	{
		measure->type = MEASURE_SCREEN_HEIGHT;
	}
	else if (_wcsicmp(L"NUM_MONITORS", type) == 0)
	{
		measure->type = MEASURE_NUM_MONITORS;
	}
	else if (_wcsicmp(L"VIRTUAL_SCREEN_TOP", type) == 0)
	{
		measure->type = MEASURE_VIRTUAL_SCREEN_TOP;
	}
	else if (_wcsicmp(L"VIRTUAL_SCREEN_LEFT", type) == 0)
	{
		measure->type = MEASURE_VIRTUAL_SCREEN_LEFT;
	}
	else if (_wcsicmp(L"VIRTUAL_SCREEN_WIDTH", type) == 0)
	{
		measure->type = MEASURE_VIRTUAL_SCREEN_WIDTH;
	}
	else if (_wcsicmp(L"VIRTUAL_SCREEN_HEIGHT", type) == 0)
	{
		measure->type = MEASURE_VIRTUAL_SCREEN_HEIGHT;
	}
	else
	{
		WCHAR buffer[256];
		_snwprintf_s(buffer, _TRUNCATE, L"SysInfo.dll: SysInfoType=%s is not valid in [%s]", type, RmGetMeasureName(rm));
		RmLog(LOG_ERROR, buffer);
	}

	measure->data = RmReadInt(rm, L"SysInfoData", -1);
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	MeasureData* measure = (MeasureData*)data;

	static WCHAR buffer[4096];
	DWORD len = 4095;

	auto convertToWide = [&](LPCSTR str)->LPCWSTR
	{
		int strLen = (int)strlen(str);
		int bufLen = MultiByteToWideChar(CP_ACP, 0, str, strLen, NULL, 0);
		MultiByteToWideChar(CP_ACP, 0, str, strLen, buffer, min(bufLen, 4095));
		return buffer;
	};

	switch (measure->type)
	{
	case MEASURE_COMPUTER_NAME:
		GetComputerName(buffer, &len);
		return buffer;

	case MEASURE_USER_NAME:
		GetUserName(buffer, &len);
		return buffer;

	case MEASURE_WORK_AREA:
		wsprintf(buffer, L"%i x %i", GetSystemMetrics(SM_CXFULLSCREEN), GetSystemMetrics(SM_CYFULLSCREEN));
		return buffer;

	case MEASURE_SCREEN_SIZE:
		wsprintf(buffer, L"%i x %i", GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		return buffer;

	case MEASURE_OS_VERSION:
		return GetPlatformName();

	case MEASURE_ADAPTER_DESCRIPTION:
		if (ERROR_SUCCESS == GetAdaptersInfo((IP_ADAPTER_INFO*)buffer, &len))
		{
			PIP_ADAPTER_INFO info = (IP_ADAPTER_INFO*)buffer;
			int i = 0;
			while (info)
			{
				if (i == measure->data)
				{
					return convertToWide(info->Description);
				}

				info = info->Next;
				i++;
			}
		}
		break;

	case MEASURE_IP_ADDRESS:
		if (NO_ERROR == GetIpAddrTable((PMIB_IPADDRTABLE)buffer, &len, FALSE))
		{
			PMIB_IPADDRTABLE ipTable = (PMIB_IPADDRTABLE)buffer;
			if (measure->data >= 1000)
			{
				measure->data = measure->data-999;
				for (UINT i = 0; i < ipTable->dwNumEntries; ++i)
				{
					if ((ipTable->table[i].wType) & MIB_IPADDR_DISCONNECTED) continue;
					--measure->data;
					if (measure->data==0)
					{
						DWORD ip = ipTable->table[i].dwAddr;
						wsprintf(buffer, L"%i.%i.%i.%i", ip % 256, (ip >> 8) % 256, (ip >> 16) % 256, (ip >> 24) % 256);
						return buffer;
					}
				}
			}
			else if (measure->data < ipTable->dwNumEntries)
			{
				DWORD ip = ipTable->table[measure->data].dwAddr;
				wsprintf(buffer, L"%i.%i.%i.%i", ip % 256, (ip >> 8) % 256, (ip >> 16) % 256, (ip >> 24) % 256);
				return buffer;
			}
		}
		return L"";

	case MEASURE_NET_MASK:
		if (NO_ERROR == GetIpAddrTable((PMIB_IPADDRTABLE)buffer, &len, FALSE))
		{
			PMIB_IPADDRTABLE ipTable = (PMIB_IPADDRTABLE)buffer;
			if (measure->data < ipTable->dwNumEntries)
			{
				DWORD ip = ipTable->table[measure->data].dwMask;
				wsprintf(buffer, L"%i.%i.%i.%i", ip % 256, (ip >> 8) % 256, (ip >> 16) % 256, (ip >> 24) % 256);
				return buffer;
			}
		}
		break;

	case MEASURE_GATEWAY_ADDRESS:
		if (ERROR_SUCCESS == GetAdaptersInfo((IP_ADAPTER_INFO*)buffer, &len))
		{
			PIP_ADAPTER_INFO info = (IP_ADAPTER_INFO*)buffer;
			int i = 0;
			while (info)
			{
				if (i == measure->data)
				{
					return convertToWide(info->GatewayList.IpAddress.String);
				}
				info = info->Next;
				++i;
			}
		}
		break;

	case MEASURE_HOST_NAME:
		if (ERROR_SUCCESS == GetNetworkParams((PFIXED_INFO)buffer, &len))
		{
			PFIXED_INFO info = (PFIXED_INFO)buffer;
			return convertToWide(info->HostName);
		}
		break;

	case MEASURE_DOMAIN_NAME:
		if (ERROR_SUCCESS == GetNetworkParams((PFIXED_INFO)buffer, &len))
		{
			PFIXED_INFO info = (PFIXED_INFO)buffer;
			return convertToWide(info->DomainName);
		}
		break;

	case MEASURE_DNS_SERVER:
		if (ERROR_SUCCESS == GetNetworkParams((PFIXED_INFO)buffer, &len))
		{
			PFIXED_INFO info = (PFIXED_INFO)buffer;
			if (info->CurrentDnsServer)
			{
				return convertToWide(info->CurrentDnsServer->IpAddress.String);
			}
			else
			{
				return convertToWide(info->DnsServerList.IpAddress.String);
			}
		}
		break;
	}

	return NULL;
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;

	switch (measure->type)
	{
	case MEASURE_OS_BITS:
		{
			SYSTEM_INFO si = {0};
			GetNativeSystemInfo(&si);
			return (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
				si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) ? 64.0 : 32.0;
		}

	case MEASURE_WORK_AREA_WIDTH:
		return (measure->data != -1)
			? m_Monitors.m_MonitorInfo[measure->data - 1].rcWork.right - m_Monitors.m_MonitorInfo[measure->data - 1].rcWork.left
			: GetSystemMetrics(SM_CXFULLSCREEN);

	case MEASURE_WORK_AREA_HEIGHT:
		return (measure->data != -1)
			? m_Monitors.m_MonitorInfo[measure->data - 1].rcWork.bottom - m_Monitors.m_MonitorInfo[measure->data - 1].rcWork.top
			: GetSystemMetrics(SM_CYFULLSCREEN);

	case MEASURE_SCREEN_WIDTH:
		return (measure->data != -1)
			? m_Monitors.m_MonitorInfo[measure->data - 1].rcMonitor.right - m_Monitors.m_MonitorInfo[measure->data - 1].rcMonitor.left
			: GetSystemMetrics(SM_CXSCREEN);

	case MEASURE_SCREEN_HEIGHT:
		return (measure->data != -1)
			? m_Monitors.m_MonitorInfo[measure->data - 1].rcMonitor.bottom - m_Monitors.m_MonitorInfo[measure->data - 1].rcMonitor.top
			: GetSystemMetrics(SM_CYSCREEN);

	case MEASURE_VIRTUAL_SCREEN_WIDTH:

		return GetSystemMetrics(SM_CXVIRTUALSCREEN);
	case MEASURE_VIRTUAL_SCREEN_HEIGHT:
		return GetSystemMetrics(SM_CYVIRTUALSCREEN);

	case MEASURE_NUM_MONITORS:
		return GetSystemMetrics(SM_CMONITORS);

	case MEASURE_WORK_AREA_TOP:
		return (measure->data != -1)
			? m_Monitors.m_MonitorInfo[measure->data - 1].rcWork.top
			: m_Monitors.m_MonitorInfo[0].rcWork.top;

	case MEASURE_WORK_AREA_LEFT:
		return (measure->data != -1)
			? m_Monitors.m_MonitorInfo[measure->data - 1].rcWork.left
			: m_Monitors.m_MonitorInfo[0].rcWork.left;

	case MEASURE_VIRTUAL_SCREEN_TOP:
		return (measure->data != -1)
			? m_Monitors.m_MonitorInfo[measure->data - 1].rcMonitor.top
			: GetSystemMetrics(SM_YVIRTUALSCREEN);

	case MEASURE_VIRTUAL_SCREEN_LEFT:
		return (measure->data != -1)
			? m_Monitors.m_MonitorInfo[measure->data - 1].rcMonitor.left
			: GetSystemMetrics(SM_XVIRTUALSCREEN);
	}

	return 0.0;
}

BOOL CALLBACK MyInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	MULTIMONITOR_INFO* m = (MULTIMONITOR_INFO*)dwData;
	m->m_Monitors[m->count] = hMonitor;
	memcpy(&(m->m_MonitorRect[m->count]), lprcMonitor, sizeof(RECT));
	m->m_MonitorInfo[m->count].cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(hMonitor, &(m->m_MonitorInfo[m->count]));
	++m->count;
	return true;
}

PLUGIN_EXPORT void Finalize(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	delete measure;
}

LPCWSTR GetPlatformName()
{
	OSVERSIONINFOEX osvi = {sizeof(OSVERSIONINFOEX)};
	if (GetVersionEx((OSVERSIONINFO*)&osvi))
	{
		if (osvi.dwMajorVersion == 5)
		{
			if (osvi.dwMinorVersion == 2)
			{
				return L"Windows 2003";
			}
			else if (osvi.dwMinorVersion == 1)
			{
				return L"Windows XP";
			}
		}
		else
		{
			if (osvi.dwMinorVersion == 1 && osvi.wProductType == VER_NT_WORKSTATION)
			{
				return L"Windows 7";
			}
			else if (osvi.dwMinorVersion == 1 && osvi.wProductType != VER_NT_WORKSTATION)
			{
				return L"Windows Server 2008 R2";
			}
			else if (osvi.dwMinorVersion == 0 && osvi.wProductType == VER_NT_WORKSTATION)
			{
				return L"Windows Vista";
			}
			else if (osvi.dwMinorVersion == 0 && osvi.wProductType != VER_NT_WORKSTATION)
			{
				return L"Windows Server 2008";
			}
		}
	}

	return L"Unknown";
}
