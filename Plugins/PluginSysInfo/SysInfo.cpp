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

#include <algorithm>
#include <windows.h>
#include <Iphlpapi.h>
#include <Netlistmgr.h>
#include <stdio.h>
#include <stdlib.h>
#include "../API/RainmeterAPI.h"
#include "../../Library/Export.h"
#include"../../Common/Platform.h"
#include "../../Common/StringUtil.h"

#define INADDR_ANY (ULONG)0x00000000

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
	MEASURE_IDLE_TIME,
	MEASURE_ADAPTER_DESCRIPTION,
	MEASURE_NET_MASK,
	MEASURE_IP_ADDRESS,
	MEASURE_GATEWAY_ADDRESS,
	MEASURE_HOST_NAME,
	MEASURE_DOMAIN_NAME,
	MEASURE_DNS_SERVER,
	MEASURE_INTERNET_CONNECTIVITY,
	MEASURE_LAN_CONNECTIVITY,
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
	MEASURE_VIRTUAL_SCREEN_HEIGHT,
	MEASURE_TIMEZONE_ISDST,
	MEASURE_TIMEZONE_BIAS,
	MEASURE_TIMEZONE_STANDARD_BIAS,
	MEASURE_TIMEZONE_STANDARD_NAME,
	MEASURE_TIMEZONE_DAYLIGHT_BIAS,
	MEASURE_TIMEZONE_DAYLIGHT_NAME
};

struct MeasureData
{
	MeasureType type;
	int data;

	bool useBestInterface;

	MeasureData() : type(), data(), useBestInterface(false) {}
};

NLM_CONNECTIVITY GetNetworkConnectivity();
BOOL CALLBACK MyInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
int GetBestInterfaceOrByName(LPCWSTR data, bool& found);

bool g_Initialized = false;

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData;
	*data = measure;

	if (!g_Initialized)
	{
		if (GetSystemMetrics(SM_CMONITORS) > 32)
		{
			LSLog(LOG_ERROR, nullptr, L"SysInfo.dll: Max amount of monitors supported is 32.");
		}

		m_Monitors.count = 0;
		EnumDisplayMonitors(nullptr, nullptr, MyInfoEnumProc, (LPARAM)(&m_Monitors));
		g_Initialized = true;
	}
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;

	int defaultData = -1;

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
	else if (_wcsicmp(L"IDLE_TIME", type) == 0)
	{
		measure->type = MEASURE_IDLE_TIME;
	}
	else if (_wcsicmp(L"ADAPTER_DESCRIPTION", type) == 0)
	{
		defaultData = 0;
		measure->type = MEASURE_ADAPTER_DESCRIPTION;
	}
	else if (_wcsicmp(L"NET_MASK", type) == 0)
	{
		defaultData = 0;
		measure->type = MEASURE_NET_MASK;
	}
	else if (_wcsicmp(L"IP_ADDRESS", type) == 0)
	{
		defaultData = 0;
		measure->type = MEASURE_IP_ADDRESS;
	}
	else if (_wcsicmp(L"GATEWAY_ADDRESS", type) == 0)
	{
		defaultData = 0;
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
	else if (_wcsicmp(L"INTERNET_CONNECTIVITY", type) == 0)
	{
		measure->type = MEASURE_INTERNET_CONNECTIVITY;
	}
	else if (_wcsicmp(L"LAN_CONNECTIVITY", type) == 0)
	{
		measure->type = MEASURE_LAN_CONNECTIVITY;
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
	else if (_wcsicmp(L"TIMEZONE_ISDST", type) == 0)
	{
		measure->type = MEASURE_TIMEZONE_ISDST;
	}
	else if (_wcsicmp(L"TIMEZONE_BIAS", type) == 0)
	{
		measure->type = MEASURE_TIMEZONE_BIAS;
	}
	else if (_wcsicmp(L"TIMEZONE_STANDARD_BIAS", type) == 0)
	{
		measure->type = MEASURE_TIMEZONE_STANDARD_BIAS;
	}
	else if (_wcsicmp(L"TIMEZONE_STANDARD_NAME", type) == 0)
	{
		measure->type = MEASURE_TIMEZONE_STANDARD_NAME;
	}
	else if (_wcsicmp(L"TIMEZONE_DAYLIGHT_BIAS", type) == 0)
	{
		measure->type = MEASURE_TIMEZONE_DAYLIGHT_BIAS;
	}
	else if (_wcsicmp(L"TIMEZONE_DAYLIGHT_NAME", type) == 0)
	{
		measure->type = MEASURE_TIMEZONE_DAYLIGHT_NAME;
	}
	else
	{
		WCHAR buffer[256];
		_snwprintf_s(buffer, _TRUNCATE, L"SysInfo.dll: SysInfoType=%s is not valid in [%s]", type, RmGetMeasureName(rm));
		RmLog(LOG_ERROR, buffer);
	}

	measure->useBestInterface = false;
	if (measure->type >= MEASURE_ADAPTER_DESCRIPTION && measure->type <= MEASURE_GATEWAY_ADDRESS)
	{
		std::wstring siData = RmReadString(rm, L"SysInfoData", L"");
		if (!siData.empty() && !std::all_of(siData.begin(), siData.end(), iswdigit))
		{
			measure->data = GetBestInterfaceOrByName(siData.c_str(), measure->useBestInterface);
		}
		else
		{
			measure->data = RmReadInt(rm, L"SysInfoData", defaultData);
		}
	}
	else
	{
		measure->data = RmReadInt(rm, L"SysInfoData", defaultData);
	}
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	MeasureData* measure = (MeasureData*)data;

	static WCHAR sBuffer[256];
	DWORD sBufferLen = _countof(sBuffer);

	BYTE tmpBuffer[7168];
	ULONG tmpBufferLen = _countof(tmpBuffer);

	auto convertToWide = [&](LPCSTR str)->LPCWSTR
	{
		MultiByteToWideChar(CP_ACP, 0, str, -1, sBuffer, 256);
		return sBuffer;
	};

	switch (measure->type)
	{
	case MEASURE_COMPUTER_NAME:
		GetComputerName(sBuffer, &sBufferLen);
		return sBuffer;

	case MEASURE_USER_NAME:
		GetUserName(sBuffer, &sBufferLen);
		return sBuffer;

	case MEASURE_WORK_AREA:
		wsprintf(sBuffer, L"%i x %i", GetSystemMetrics(SM_CXFULLSCREEN), GetSystemMetrics(SM_CYFULLSCREEN));
		return sBuffer;

	case MEASURE_SCREEN_SIZE:
		wsprintf(sBuffer, L"%i x %i", GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		return sBuffer;

	case MEASURE_OS_VERSION:
		return Platform::GetPlatformName();

	case MEASURE_ADAPTER_DESCRIPTION:
		if (ERROR_SUCCESS == GetAdaptersInfo((IP_ADAPTER_INFO*)tmpBuffer, &tmpBufferLen))
		{
			PIP_ADAPTER_INFO info = (IP_ADAPTER_INFO*)tmpBuffer;
			int i = 0;
			while (info)
			{
				if (measure->useBestInterface)
				{
					if (info->Index == measure->data)
					{
						return convertToWide(info->Description);
					}
				}
				else
				{
					if (i == measure->data)
					{
						return convertToWide(info->Description);
					}
				}

				info = info->Next;
				++i;
			}
		}
		break;

	case MEASURE_IP_ADDRESS:
		if (NO_ERROR == GetIpAddrTable((PMIB_IPADDRTABLE)tmpBuffer, &tmpBufferLen, FALSE))
		{
			PMIB_IPADDRTABLE ipTable = (PMIB_IPADDRTABLE)tmpBuffer;
			if (measure->useBestInterface)
			{
				for (UINT i = 0; i < ipTable->dwNumEntries; ++i)
				{
					if (ipTable->table[i].dwIndex == measure->data)
					{
						DWORD ip = ipTable->table[i].dwAddr;
						wsprintf(sBuffer, L"%i.%i.%i.%i", ip % 256, (ip >> 8) % 256, (ip >> 16) % 256, (ip >> 24) % 256);
						return sBuffer;
					}
				}
			}
			else if (measure->data >= 1000)
			{
				measure->data = measure->data - 999;
				for (UINT i = 0; i < ipTable->dwNumEntries; ++i)
				{
					if ((ipTable->table[i].wType) & MIB_IPADDR_DISCONNECTED) continue;
					--measure->data;
					if (measure->data == 0)
					{
						DWORD ip = ipTable->table[i].dwAddr;
						wsprintf(sBuffer, L"%i.%i.%i.%i", ip % 256, (ip >> 8) % 256, (ip >> 16) % 256, (ip >> 24) % 256);
						return sBuffer;
					}
				}
			}
			else if (measure->data < (int)ipTable->dwNumEntries)
			{
				DWORD ip = ipTable->table[measure->data].dwAddr;
				wsprintf(sBuffer, L"%i.%i.%i.%i", ip % 256, (ip >> 8) % 256, (ip >> 16) % 256, (ip >> 24) % 256);
				return sBuffer;
			}
		}
		return L"";

	case MEASURE_NET_MASK:
		if (NO_ERROR == GetIpAddrTable((PMIB_IPADDRTABLE)tmpBuffer, &tmpBufferLen, FALSE))
		{
			PMIB_IPADDRTABLE ipTable = (PMIB_IPADDRTABLE)tmpBuffer;
			if (measure->useBestInterface)
			{
				for (UINT i = 0; i < ipTable->dwNumEntries; ++i)
				{
					if (ipTable->table[i].dwIndex == measure->data)
					{
						DWORD ip = ipTable->table[i].dwMask;
						wsprintf(sBuffer, L"%i.%i.%i.%i", ip % 256, (ip >> 8) % 256, (ip >> 16) % 256, (ip >> 24) % 256);
						return sBuffer;
					}
				}
			}
			else if (measure->data < (int)ipTable->dwNumEntries)
			{
				DWORD ip = ipTable->table[measure->data].dwMask;
				wsprintf(sBuffer, L"%i.%i.%i.%i", ip % 256, (ip >> 8) % 256, (ip >> 16) % 256, (ip >> 24) % 256);
				return sBuffer;
			}
		}
		break;

	case MEASURE_GATEWAY_ADDRESS:
		if (ERROR_SUCCESS == GetAdaptersInfo((IP_ADAPTER_INFO*)tmpBuffer, &tmpBufferLen))
		{
			PIP_ADAPTER_INFO info = (IP_ADAPTER_INFO*)tmpBuffer;
			int i = 0;
			while (info)
			{
				if (measure->useBestInterface)
				{
					if (info->Index == measure->data)
					{
						return convertToWide(info->GatewayList.IpAddress.String);
					}
				}
				else if (i == measure->data)
				{
					return convertToWide(info->GatewayList.IpAddress.String);
				}
				info = info->Next;
				++i;
			}
		}
		break;

	case MEASURE_HOST_NAME:
		if (ERROR_SUCCESS == GetNetworkParams((PFIXED_INFO)tmpBuffer, &tmpBufferLen))
		{
			PFIXED_INFO info = (PFIXED_INFO)tmpBuffer;
			return convertToWide(info->HostName);
		}
		break;

	case MEASURE_DOMAIN_NAME:
		if (ERROR_SUCCESS == GetNetworkParams((PFIXED_INFO)tmpBuffer, &tmpBufferLen))
		{
			PFIXED_INFO info = (PFIXED_INFO)tmpBuffer;
			return convertToWide(info->DomainName);
		}
		break;

	case MEASURE_DNS_SERVER:
		if (ERROR_SUCCESS == GetNetworkParams((PFIXED_INFO)tmpBuffer, &tmpBufferLen))
		{
			PFIXED_INFO info = (PFIXED_INFO)tmpBuffer;
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

	case MEASURE_TIMEZONE_STANDARD_NAME:
		{
			TIME_ZONE_INFORMATION tzi;
			GetTimeZoneInformation(&tzi);
			wcscpy(sBuffer, tzi.StandardName);
			return sBuffer;
		}

	case MEASURE_TIMEZONE_DAYLIGHT_NAME:
		{
			TIME_ZONE_INFORMATION tzi;
			GetTimeZoneInformation(&tzi);
			wcscpy(sBuffer, tzi.DaylightName);
			return sBuffer;
		}
	}

	return nullptr;
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

	case MEASURE_IDLE_TIME:
		{
			LASTINPUTINFO idle = { sizeof(LASTINPUTINFO) };
			GetLastInputInfo(&idle);
			return (double)((GetTickCount() - idle.dwTime) / 1000);
		}

	case MEASURE_INTERNET_CONNECTIVITY:
		{
			const auto connectivity = GetNetworkConnectivity();
			return (connectivity & NLM_CONNECTIVITY_IPV4_INTERNET ||
				connectivity & NLM_CONNECTIVITY_IPV6_INTERNET) ? 1.0 : -1.0;
		}

	case MEASURE_LAN_CONNECTIVITY:
		return GetNetworkConnectivity() != NLM_CONNECTIVITY_DISCONNECTED ? 1.0 : -1.0;

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

	case MEASURE_TIMEZONE_ISDST:
		{
			TIME_ZONE_INFORMATION tzi;
			DWORD ret = GetTimeZoneInformation(&tzi);
			return ret == TIME_ZONE_ID_UNKNOWN ? -1.0 : ret - 1.0;
		}
	case MEASURE_TIMEZONE_BIAS:
		{
			TIME_ZONE_INFORMATION tzi;
			GetTimeZoneInformation(&tzi);
			return (double)tzi.Bias;
		}
	case MEASURE_TIMEZONE_STANDARD_BIAS:
		{
			TIME_ZONE_INFORMATION tzi;
			GetTimeZoneInformation(&tzi);
			return (double)tzi.StandardBias;
		}
	case MEASURE_TIMEZONE_DAYLIGHT_BIAS:
		{
			TIME_ZONE_INFORMATION tzi;
			GetTimeZoneInformation(&tzi);
			return (double)tzi.DaylightBias;
		}
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

NLM_CONNECTIVITY GetNetworkConnectivity()
{
	// This is initialized like this in case INetworkListManager is not available (i.e. on WinXP).
	// In such cases, we simply assume that there is an internet connection.
	NLM_CONNECTIVITY connectivity =
		(NLM_CONNECTIVITY)((int)NLM_CONNECTIVITY_IPV4_INTERNET | (int)NLM_CONNECTIVITY_IPV4_LOCALNETWORK);

	INetworkListManager* nlm;
	HRESULT hr = CoCreateInstance(
		CLSID_NetworkListManager, NULL, CLSCTX_INPROC_SERVER, __uuidof(INetworkListManager), (void**)&nlm);
	if (SUCCEEDED(hr))
	{
		nlm->GetConnectivity(&connectivity);
		nlm->Release();
	}

	return connectivity;
}

int GetBestInterfaceOrByName(LPCWSTR data, bool& found)
{
	int index = 0;

	if (_wcsicmp(data, L"BEST") == 0)
	{
		DWORD dwBestIndex;
		if (NO_ERROR == GetBestInterface(INADDR_ANY, &dwBestIndex))
		{
			index = (int)dwBestIndex;
			found = true;
		}
	}
	else
	{
		BYTE buffer[7168];
		ULONG bufLen = _countof(buffer);

		if (ERROR_SUCCESS == GetAdaptersInfo((IP_ADAPTER_INFO*)buffer, &bufLen))
		{
			PIP_ADAPTER_INFO info = (IP_ADAPTER_INFO*)buffer;
			int i = 0;
			while (info)
			{
				if (_wcsicmp(data, StringUtil::Widen(info->Description).c_str()) == 0)
				{
					index = info->Index;
					found = true;
					break;
				}

				info = info->Next;
				++i;
			}
		}
	}

	return index;
}
