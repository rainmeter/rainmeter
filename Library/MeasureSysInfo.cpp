/* Copyright (C) 2021 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureSysInfo.h"
#include "Rainmeter.h"
#include "System.h"
#include "../Common/NetworkUtil.h"
#include "../Common/Platform.h"

#include <sddl.h>
#include <LM.h>
#include <Powrprof.h>
#include <Ip2string.h>

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

LONGLONG MeasureSysInfo::s_LogonTime = 0LL;

MeasureSysInfo::MeasureSysInfo(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Type(SysInfoType::UNKNOWN),
	m_Data(0),
	m_SuppressError(false),
	m_HasBeenUpdated(false)
{
	if (s_LogonTime == 0LL)
	{
		HKEY hKey;
		if (RegOpenKey(HKEY_CURRENT_USER, L"Volatile Environment", &hKey) == ERROR_SUCCESS)
		{
			FILETIME lastWrite;
			if (RegQueryInfoKey(hKey, nullptr, nullptr, nullptr, nullptr, nullptr,
				nullptr, nullptr, nullptr, nullptr, nullptr, &lastWrite) == ERROR_SUCCESS)
			{
				FileTimeToLocalFileTime(&lastWrite, &lastWrite);

				LARGE_INTEGER li = { 0 };
				li.LowPart = lastWrite.dwLowDateTime;
				li.HighPart = lastWrite.dwHighDateTime;
				s_LogonTime = li.QuadPart;
			}
			RegCloseKey(hKey);
		}

		// Populate monitor information if necessary
		const size_t numOfMonitors = System::GetMonitorCount();  // Intentional
	}
}

MeasureSysInfo::~MeasureSysInfo()
{
}

void MeasureSysInfo::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	SysInfoType oldType = m_Type;
	int oldData = m_Data;

	int defaultData = -1;

	std::wstring typeStr = parser.ReadString(section, L"SysInfoType", L"");
	LPCWSTR type = typeStr.c_str();
	if (_wcsicmp(L"COMPUTER_NAME", type) == 0)					// BLOCK 1000
	{
		m_Type = SysInfoType::COMPUTER_NAME;
	}
	else if (_wcsicmp(L"USER_NAME", type) == 0)
	{
		m_Type = SysInfoType::USER_NAME;
	}
	else if (_wcsicmp(L"USER_SID", type) == 0)
	{
		m_Type = SysInfoType::USER_SID;
	}
	else if (_wcsicmp(L"OS_VERSION", type) == 0)
	{
		m_Type = SysInfoType::OS_VERSION;
	}
	else if (_wcsicmp(L"PAGESIZE", type) == 0)
	{
		m_Type = SysInfoType::PAGESIZE;
	}
	else if (_wcsicmp(L"OS_BITS", type) == 0)
	{
		m_Type = SysInfoType::OS_BITS;
	}
	else if (_wcsicmp(L"NUM_MONITORS", type) == 0)				// BLOCK 2000
	{
		m_Type = SysInfoType::NUM_MONITORS;
	}
	else if (_wcsicmp(L"SCREEN_SIZE", type) == 0)
	{
		m_Type = SysInfoType::SCREEN_SIZE;
	}
	else if (_wcsicmp(L"WORK_AREA", type) == 0)
	{
		m_Type = SysInfoType::WORK_AREA;
	}
	else if (_wcsicmp(L"SCREEN_WIDTH", type) == 0)				// BLOCK 2500
	{
		m_Type = SysInfoType::SCREEN_WIDTH;
	}
	else if (_wcsicmp(L"SCREEN_HEIGHT", type) == 0)
	{
		m_Type = SysInfoType::SCREEN_HEIGHT;
	}
	else if (_wcsicmp(L"WORK_AREA_LEFT", type) == 0)
	{
		m_Type = SysInfoType::WORK_AREA_LEFT;
	}
	else if (_wcsicmp(L"WORK_AREA_TOP", type) == 0)
	{
		m_Type = SysInfoType::WORK_AREA_TOP;
	}
	else if (_wcsicmp(L"WORK_AREA_WIDTH", type) == 0)
	{
		m_Type = SysInfoType::WORK_AREA_WIDTH;
	}
	else if (_wcsicmp(L"WORK_AREA_HEIGHT", type) == 0)
	{
		m_Type = SysInfoType::WORK_AREA_HEIGHT;
	}
	else if (_wcsicmp(L"VIRTUAL_SCREEN_LEFT", type) == 0)
	{
		m_Type = SysInfoType::VIRTUAL_SCREEN_LEFT;
	}
	else if (_wcsicmp(L"VIRTUAL_SCREEN_TOP", type) == 0)
	{
		m_Type = SysInfoType::VIRTUAL_SCREEN_TOP;
	}
	else if (_wcsicmp(L"VIRTUAL_SCREEN_WIDTH", type) == 0)
	{
		m_Type = SysInfoType::VIRTUAL_SCREEN_WIDTH;
	}
	else if (_wcsicmp(L"VIRTUAL_SCREEN_HEIGHT", type) == 0)
	{
		m_Type = SysInfoType::VIRTUAL_SCREEN_HEIGHT;
	}
	else if (_wcsicmp(L"HOST_NAME", type) == 0)					// BLOCK 3000
	{
		m_Type = SysInfoType::HOST_NAME;
	}
	else if (_wcsicmp(L"DOMAIN_NAME", type) == 0)
	{
		m_Type = SysInfoType::DOMAIN_NAME;
	}
	else if (_wcsicmp(L"DNS_SERVER", type) == 0)
	{
		m_Type = SysInfoType::DNS_SERVER;
	}
	else if (_wcsicmp(L"DOMAIN_WORKGROUP", type) == 0 ||
		_wcsicmp(L"DOMAINWORKGROUP", type) == 0)
	{
		m_Type = SysInfoType::DOMAIN_WORKGROUP;
	}
	else if (_wcsicmp(L"INTERNET_CONNECTIVITY", type) == 0)		// BLOCK 3250
	{
		m_Type = SysInfoType::INTERNET_CONNECTIVITY;
	}
	else if (_wcsicmp(L"INTERNET_CONNECTIVITY_V4", type) == 0)
	{
		m_Type = SysInfoType::INTERNET_CONNECTIVITY_V4;
	}
	else if (_wcsicmp(L"INTERNET_CONNECTIVITY_V6", type) == 0)
	{
		m_Type = SysInfoType::INTERNET_CONNECTIVITY_V6;
	}
	else if (_wcsicmp(L"LAN_CONNECTIVITY", type) == 0)
	{
		m_Type = SysInfoType::LAN_CONNECTIVITY;
	}
	else if (_wcsicmp(L"LAN_CONNECTIVITY_V4", type) == 0)
	{
		m_Type = SysInfoType::LAN_CONNECTIVITY_V4;
	}
	else if (_wcsicmp(L"LAN_CONNECTIVITY_V6", type) == 0)
	{
		m_Type = SysInfoType::LAN_CONNECTIVITY_V6;
	}
	else if (_wcsicmp(L"ADAPTER_DESCRIPTION", type) == 0)		// BLOCK 3500
	{
		defaultData = 0;
		m_Type = SysInfoType::ADAPTER_DESCRIPTION;
	}
	else if (_wcsicmp(L"ADAPTER_ALIAS", type) == 0)
	{
		defaultData = 0;
		m_Type = SysInfoType::ADAPTER_ALIAS;
	}
	else if (_wcsicmp(L"ADAPTER_GUID", type) == 0)
	{
		defaultData = 0;
		m_Type = SysInfoType::ADAPTER_GUID;
	}
	else if (_wcsicmp(L"ADAPTER_TYPE", type) == 0)
	{
		defaultData = 0;
		m_Type = SysInfoType::ADAPTER_TYPE;
	}
	else if (_wcsicmp(L"ADAPTER_STATE", type) == 0)
	{
		defaultData = 0;
		m_Type = SysInfoType::ADAPTER_STATE;
	}
	else if (_wcsicmp(L"ADAPTER_STATUS", type) == 0)
	{
		defaultData = 0;
		m_Type = SysInfoType::ADAPTER_STATUS;
	}
	else if (_wcsicmp(L"ADAPTER_TRANSMIT_SPEED", type) == 0)
	{
		defaultData = 0;
		m_Type = SysInfoType::ADAPTER_TRANSMIT_SPEED;
	}
	else if (_wcsicmp(L"ADAPTER_RECEIVE_SPEED", type) == 0)
	{
		defaultData = 0;
		m_Type = SysInfoType::ADAPTER_RECEIVE_SPEED;
	}
	else if (_wcsicmp(L"MAC_ADDRESS", type) == 0)
	{
		defaultData = 0;
		m_Type = SysInfoType::MAC_ADDRESS;
	}
	else if (_wcsicmp(L"NET_MASK", type) == 0)
	{
		defaultData = 0;
		m_Type = SysInfoType::NET_MASK;
	}
	else if (_wcsicmp(L"IP_ADDRESS", type) == 0)
	{
		defaultData = 0;
		m_Type = SysInfoType::IP_ADDRESS;
	}
	else if (_wcsicmp(L"GATEWAY_ADDRESS", type) == 0)
	{
		defaultData = 0;
		m_Type = SysInfoType::GATEWAY_ADDRESS;
	}
	else if (_wcsicmp(L"GATEWAY_ADDRESS_V4", type) == 0)
	{
		defaultData = 0;
		m_Type = SysInfoType::GATEWAY_ADDRESS_V4;
	}
	else if (_wcsicmp(L"GATEWAY_ADDRESS_V6", type) == 0)
	{
		defaultData = 0;
		m_Type = SysInfoType::GATEWAY_ADDRESS_V6;
	}
	else if (_wcsicmp(L"TIMEZONE_ISDST", type) == 0)			// BLOCK 4000
	{
		m_Type = SysInfoType::TIMEZONE_ISDST;
	}
	else if (_wcsicmp(L"TIMEZONE_BIAS", type) == 0)
	{
		m_Type = SysInfoType::TIMEZONE_BIAS;
	}
	else if (_wcsicmp(L"TIMEZONE_STANDARD_BIAS", type) == 0)
	{
		m_Type = SysInfoType::TIMEZONE_STANDARD_BIAS;
	}
	else if (_wcsicmp(L"TIMEZONE_STANDARD_NAME", type) == 0)
	{
		m_Type = SysInfoType::TIMEZONE_STANDARD_NAME;
	}
	else if (_wcsicmp(L"TIMEZONE_DAYLIGHT_BIAS", type) == 0)
	{
		m_Type = SysInfoType::TIMEZONE_DAYLIGHT_BIAS;
	}
	else if (_wcsicmp(L"TIMEZONE_DAYLIGHT_NAME", type) == 0)
	{
		m_Type = SysInfoType::TIMEZONE_DAYLIGHT_NAME;
	}
	else if (_wcsicmp(L"IDLE_TIME", type) == 0)					// BLOCK 5000
	{
		m_Type = SysInfoType::IDLE_TIME;
	}
	else if (_wcsicmp(L"USER_LOGONTIME", type) == 0)
	{
		m_Type = SysInfoType::USER_LOGON_TIME;
	}
	else if (_wcsicmp(L"LAST_SLEEP_TIME", type) == 0)
	{
		m_Type = SysInfoType::LAST_SLEEP_TIME;
	}
	else if (_wcsicmp(L"LAST_WAKE_TIME", type) == 0)
	{
		m_Type = SysInfoType::LAST_WAKE_TIME;
	}
	else
	{
		LogErrorF(this, L"SysInfo: SysInfoType=%s is not valid.", type);
		m_Type = SysInfoType::UNKNOWN;
	}

	if (m_Type >= SysInfoType::ADAPTER_DESCRIPTION && m_Type <= SysInfoType::GATEWAY_ADDRESS_V6) // BLOCK 3500
	{
		std::wstring siData = parser.ReadString(section, L"SysInfoData", L"BEST");
		if (!siData.empty() && !std::all_of(siData.begin(), siData.end(), iswdigit))
		{
			m_Data = NetworkUtil::FindBestInterface(siData.c_str());
		}
		else
		{
			m_Data = parser.ReadInt(section, L"SysInfoData", defaultData);
			if (m_Data <= 0)
			{
				LogNoticeF(this,
					L"SysInfo: SysInfoData=%i is not valid for SysInfoType=%s, using \"Best\" interface.",
					m_Data, typeStr.c_str());
				m_Data = NetworkUtil::FindBestInterface(L"BEST");
			}
		}
	}
	else
	{
		m_Data = parser.ReadInt(section, L"SysInfoData", defaultData);
	}

	if (m_HasBeenUpdated)
	{
		m_SuppressError = oldType == m_Type && oldData == m_Data;
	}
}

void MeasureSysInfo::UpdateValue()
{
	m_Value = 0.0;
	m_StringValue.clear();
	m_HasBeenUpdated = true;

	// Process numeric types first
	if (m_Type >= SysInfoType::SCREEN_WIDTH && m_Type <= SysInfoType::VIRTUAL_SCREEN_HEIGHT)  // BLOCK 2500
	{
		const MultiMonitorInfo& monitorsInfo = System::GetMultiMonitorInfo();
		const std::vector<MonitorInfo>& monitors = monitorsInfo.monitors;

		// Valid values are |-1| (default) or |1 to [# of monitors]|
		const size_t index = (m_Data > 0) ? (size_t)(m_Data - 1) : 0ULL;
		if (m_Data < -1 || m_Data == 0 || index > System::GetMonitorCount())
		{
			m_Value = 0.0;
			return;
		}

		switch (m_Type)
		{
		case SysInfoType::SCREEN_WIDTH:
			m_Value = (m_Data > 0)
				? (monitors[index].screen.right - monitors[index].screen.left)
				: (double)GetSystemMetrics(SM_CXSCREEN);
			break;

		case SysInfoType::SCREEN_HEIGHT:
			m_Value = (m_Data > 0)
				? (monitors[index].screen.bottom - monitors[index].screen.top)
				: (double)GetSystemMetrics(SM_CYSCREEN);
			break;

		case SysInfoType::WORK_AREA_LEFT:
			m_Value = (m_Data > 0)
				? monitors[index].work.left
				: (double)monitors[0].work.left;
			break;

		case SysInfoType::WORK_AREA_TOP:
			m_Value = (m_Data > 0)
				? monitors[index].work.top
				: (double)monitors[0].work.top;
			break;

		case SysInfoType::WORK_AREA_WIDTH:
			m_Value = (m_Data > 0)
				? (monitors[index].work.right - monitors[index].work.left)
				: (double)GetSystemMetrics(SM_CXFULLSCREEN);
			break;

		case SysInfoType::WORK_AREA_HEIGHT:
			m_Value = (m_Data > 0)
				? (monitors[index].work.bottom - monitors[index].work.top)
				: (double)GetSystemMetrics(SM_CYFULLSCREEN);
			break;

		case SysInfoType::VIRTUAL_SCREEN_LEFT:
			m_Value = (m_Data > 0)
				? monitors[index].screen.left
				: (double)GetSystemMetrics(SM_XVIRTUALSCREEN);
			break;

		case SysInfoType::VIRTUAL_SCREEN_TOP:
			 m_Value = (m_Data > 0)
				? monitors[index].screen.top
				: (double)GetSystemMetrics(SM_YVIRTUALSCREEN);
			break;

		case SysInfoType::VIRTUAL_SCREEN_WIDTH:
			m_Value = (double)GetSystemMetrics(SM_CXVIRTUALSCREEN);
			break;

		case SysInfoType::VIRTUAL_SCREEN_HEIGHT:
			m_Value = (double)GetSystemMetrics(SM_CYVIRTUALSCREEN);
			break;
		}
		return;
	}
	else if (m_Type >= SysInfoType::INTERNET_CONNECTIVITY && m_Type <= SysInfoType::LAN_CONNECTIVITY_V6)  // BLOCK 3250
	{
		const auto connectivity = GetNetworkConnectivity();
		switch (m_Type)
		{
		case SysInfoType::INTERNET_CONNECTIVITY:
			m_Value = (connectivity & NLM_CONNECTIVITY_IPV4_INTERNET ||
				connectivity & NLM_CONNECTIVITY_IPV6_INTERNET) ? 1.0 : -1.0;
			break;

		case SysInfoType::INTERNET_CONNECTIVITY_V4:
			m_Value = (connectivity & NLM_CONNECTIVITY_IPV4_INTERNET) ? 1.0 : -1.0;
			break;

		case SysInfoType::INTERNET_CONNECTIVITY_V6:
			m_Value = (connectivity & NLM_CONNECTIVITY_IPV6_INTERNET) ? 1.0 : -1.0;
			break;

		case SysInfoType::LAN_CONNECTIVITY:
			m_Value = GetNetworkConnectivity() != NLM_CONNECTIVITY_DISCONNECTED ? 1.0 : -1.0;
			break;

		case SysInfoType::LAN_CONNECTIVITY_V4:
			m_Value = (connectivity & NLM_CONNECTIVITY_IPV4_LOCALNETWORK) ? 1.0 : -1.0;
			break;

		case SysInfoType::LAN_CONNECTIVITY_V6:
			m_Value = (connectivity & NLM_CONNECTIVITY_IPV6_LOCALNETWORK) ? 1.0 : -1.0;
			break;
		}
		return;
	}
	else if (m_Type >= SysInfoType::TIMEZONE_ISDST && m_Type <= SysInfoType::TIMEZONE_DAYLIGHT_BIAS)  // BLOCk 4000
	{
		TIME_ZONE_INFORMATION tzi = { 0 };
		const DWORD result = GetTimeZoneInformation(&tzi);
		switch (m_Type)
		{
		case SysInfoType::TIMEZONE_ISDST:
			m_Value = (result == TIME_ZONE_ID_UNKNOWN) ? -1.0 : (result - 1.0);
			break;

		case SysInfoType::TIMEZONE_BIAS:
			m_Value = (double)tzi.Bias;
			break;

		case SysInfoType::TIMEZONE_STANDARD_BIAS:
			m_Value = (double)tzi.StandardBias;
			break;

		case SysInfoType::TIMEZONE_DAYLIGHT_BIAS:
			m_Value = (double)tzi.DaylightBias;
			break;
		}
		return;
	}

	switch (m_Type)
	{
	case SysInfoType::PAGESIZE:
	case SysInfoType::OS_BITS:
		{
			SYSTEM_INFO si = { 0 };
			GetNativeSystemInfo(&si);

			if (m_Type == SysInfoType::PAGESIZE)
			{
				m_Value = (double)si.dwPageSize;
			}
			else // OS_BITS
			{
				m_Value = (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
					si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) ? 64.0 : 32.0;
			}
		}
		return;

	case SysInfoType::NUM_MONITORS:
		m_Value = (double)GetSystemMetrics(SM_CMONITORS);
		return;

	case SysInfoType::ADAPTER_TYPE:
	case SysInfoType::ADAPTER_STATE:
	case SysInfoType::ADAPTER_STATUS:
	case SysInfoType::ADAPTER_TRANSMIT_SPEED:
	case SysInfoType::ADAPTER_RECEIVE_SPEED:
		{
			MIB_IF_ROW2* table = NetworkUtil::GetInterfaceTable();
			if (!table) break;

			const ULONG interfaceCount = NetworkUtil::GetInterfaceCount();
			for (size_t i = 0ULL; i < interfaceCount; ++i)
			{
				if (table[i].InterfaceIndex != m_Data) continue;

				switch (m_Type)
				{
				case SysInfoType::ADAPTER_TYPE:
					m_Value = (double)table[i].Type;
					break;

				case SysInfoType::ADAPTER_STATE:
					switch (table[i].MediaConnectState)
					{
					case MediaConnectStateDisconnected: m_Value = -1.0; break;
					default:                            m_Value =  0.0; break;  // Unknown
					case MediaConnectStateConnected:    m_Value =  1.0; break;
					}
					break;

				case SysInfoType::ADAPTER_STATUS:
					switch (table[i].OperStatus)
					{
					case IfOperStatusNotPresent:     m_Value = -3.0; break;
					case IfOperStatusLowerLayerDown: m_Value = -2.0; break;
					case IfOperStatusDown:           m_Value = -1.0; break;
					default:                         m_Value =  0.0; break;  // Unknown
					case IfOperStatusUp:             m_Value =  1.0; break;
					case IfOperStatusDormant:        m_Value =  2.0; break;
					case IfOperStatusTesting:        m_Value =  3.0; break;
					}
					break;

				case SysInfoType::ADAPTER_TRANSMIT_SPEED:
					m_Value = (double)table[i].TransmitLinkSpeed;
					break;

				case SysInfoType::ADAPTER_RECEIVE_SPEED:
					m_Value = (double)table[i].ReceiveLinkSpeed;
					break;
				}
				break;
			}
		}
		break;  // Process string value later

	case SysInfoType::IDLE_TIME:
		{
			LASTINPUTINFO idle = { sizeof(LASTINPUTINFO) };
			GetLastInputInfo(&idle);
			m_Value = (double)((GetTickCount64() - idle.dwTime) / 1000);
		}
		return;

	case SysInfoType::USER_LOGON_TIME:
		m_Value = s_LogonTime / 10000000.0;
		return;

	case SysInfoType::LAST_SLEEP_TIME:
	case SysInfoType::LAST_WAKE_TIME:
		{
			const bool isWake = m_Type == SysInfoType::LAST_WAKE_TIME;
			ULONGLONG nano = 0ULL;
			const LONG status = CallNtPowerInformation(isWake ? LastWakeTime : LastSleepTime,
				nullptr, 0UL, &nano, sizeof(ULONGLONG));
			if (status == STATUS_SUCCESS)
			{
				m_Value = (s_LogonTime + (LONGLONG)nano) / 10000000.0;
			}
			else if (!m_SuppressError)
			{
				// NTSTATUS codes:
				// https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-erref/596a1078-e883-4972-9bbc-49e60bebca55
				LogErrorF(this, L"Last %s time error: 0x%08x", isWake ? L"wake" : L"sleep", status);
				m_SuppressError = true;
			}
		}
		return;
	}

	// Check for string values (set internal value for use in GetStringValue)
	WCHAR buffer[256] = { 0 };
	DWORD bufferLen = _countof(buffer);

	switch (m_Type)
	{
	case SysInfoType::COMPUTER_NAME:
		GetComputerName(buffer, &bufferLen);
		break;

	case SysInfoType::USER_NAME:
		GetUserName(buffer, &bufferLen);
		break;

	case SysInfoType::USER_SID:
		{
			HANDLE hToken = INVALID_HANDLE_VALUE;
			if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) == TRUE)
			{
				std::vector<BYTE> tokenBuffer;
				DWORD tokenBufferLen = 0UL;
				if ((GetTokenInformation(hToken, TokenUser, nullptr, 0UL, &tokenBufferLen) == FALSE) &&
					(GetLastError() == ERROR_INSUFFICIENT_BUFFER) &&
					(tokenBufferLen > 0UL))
				{
					tokenBuffer.resize(tokenBufferLen);
					PTOKEN_USER token = reinterpret_cast<PTOKEN_USER>(&tokenBuffer[0]);
					LPWSTR temp = nullptr;
					if ((GetTokenInformation(hToken, TokenUser, token, tokenBufferLen, &tokenBufferLen) == TRUE) &&
						(IsValidSid(token->User.Sid) == TRUE) &&
						(ConvertSidToStringSid(token->User.Sid, &temp) == TRUE))
					{
						m_StringValue = temp;
					}
					LocalFree(temp);
				}
				CloseHandle(hToken);
				hToken = nullptr;
			}
		}
		return;

	case SysInfoType::OS_VERSION:
		m_StringValue = GetPlatform().GetName();
		return;

	case SysInfoType::SCREEN_SIZE:
		_snwprintf_s(buffer, bufferLen, L"%i x %i",
			GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		break;

	case SysInfoType::WORK_AREA:
		_snwprintf_s(buffer, bufferLen, L"%i x %i",
			GetSystemMetrics(SM_CXFULLSCREEN), GetSystemMetrics(SM_CYFULLSCREEN));
		break;

	case SysInfoType::HOST_NAME:
	case SysInfoType::DOMAIN_NAME:
	case SysInfoType::DNS_SERVER:
		{
			ULONG paramSize = 0UL;
			GetNetworkParams(nullptr, &paramSize);
			if (paramSize <= 0UL) break;

			auto tmp = std::make_unique<BYTE[]>(paramSize);
			if (GetNetworkParams((PFIXED_INFO)tmp.get(), &paramSize) != ERROR_SUCCESS) break;

			PFIXED_INFO info = (PFIXED_INFO)tmp.get();
			switch (m_Type)
			{
			case SysInfoType::HOST_NAME:   m_StringValue = StringUtil::Widen(info->HostName); break;
			case SysInfoType::DOMAIN_NAME: m_StringValue = StringUtil::Widen(info->DomainName); break;
			case SysInfoType::DNS_SERVER:  m_StringValue = (info->CurrentDnsServer)
				? StringUtil::Widen(info->CurrentDnsServer->IpAddress.String)
				: StringUtil::Widen(info->DnsServerList.IpAddress.String); break;
			}
		}
		return;

	case SysInfoType::DOMAIN_WORKGROUP:
		{
			LPWKSTA_INFO_102 info = nullptr;
			if (NetWkstaGetInfo(nullptr, 102, (LPBYTE*)&info) == NERR_Success)
			{
				m_StringValue = info->wki102_langroup;
				NetApiBufferFree(info);
			}
		}
		return;

	case SysInfoType::ADAPTER_DESCRIPTION:
	case SysInfoType::ADAPTER_ALIAS:
	case SysInfoType::ADAPTER_GUID:
	case SysInfoType::ADAPTER_TYPE:
	case SysInfoType::ADAPTER_STATE:
	case SysInfoType::ADAPTER_STATUS:
	case SysInfoType::MAC_ADDRESS:
		{
			MIB_IF_ROW2* table = NetworkUtil::GetInterfaceTable();
			if (!table) break;

			const ULONG interfaceCount = NetworkUtil::GetInterfaceCount();
			for (size_t i = 0ULL; i < interfaceCount; ++i)
			{
				if (table[i].InterfaceIndex != m_Data) continue;

				switch (m_Type)
				{
				case SysInfoType::ADAPTER_DESCRIPTION:
					m_StringValue = table[i].Description;
					break;

				case SysInfoType::ADAPTER_ALIAS:
					m_StringValue = table[i].Alias;
					break;

				case SysInfoType::ADAPTER_GUID:
				{
					WCHAR guid[64] = { 0 };
					if (StringFromGUID2(table[i].InterfaceGuid, guid, 64) > 0)
					{
						m_StringValue = guid;
					}
				}
				break;

				case SysInfoType::ADAPTER_TYPE:
					m_StringValue = NetworkUtil::GetInterfaceTypeString(table[i].Type);
					break;

				case SysInfoType::ADAPTER_STATE:
					m_StringValue = NetworkUtil::GetInterfaceMediaConnectionString(table[i].MediaConnectState);
					break;

				case SysInfoType::ADAPTER_STATUS:
					m_StringValue = NetworkUtil::GetInterfaceOperStatusString(table[i].OperStatus);
					break;

				case SysInfoType::MAC_ADDRESS:
					for (ULONG j = 0UL; j < table[i].PhysicalAddressLength; ++j)
					{
						if (j > 0UL) m_StringValue += L"-";
						_snwprintf_s(buffer, bufferLen, L"%02X", table[i].PhysicalAddress[j]);
						m_StringValue += buffer;
					}
					break;
				}
				break;
			}
		}
		return;

	case SysInfoType::NET_MASK:
	case SysInfoType::IP_ADDRESS:
		{
			const bool isIpAddress = m_Type == SysInfoType::IP_ADDRESS;
			ULONG tableSize = 0UL;
			GetIpAddrTable(nullptr, &tableSize, TRUE);
			if (tableSize <= 0UL) break;

			auto tmp = std::make_unique<BYTE[]>(tableSize);
			if (GetIpAddrTable((PMIB_IPADDRTABLE)tmp.get(),
				&tableSize, TRUE) != NO_ERROR) break;

			PMIB_IPADDRTABLE ipTable = (PMIB_IPADDRTABLE)tmp.get();
			for (ULONG i = 0UL; i < ipTable->dwNumEntries; ++i)
			{
				if (ipTable->table[i].dwIndex != m_Data) continue;

				DWORD ip = isIpAddress ? ipTable->table[i].dwAddr : ipTable->table[i].dwMask;
				_snwprintf_s(buffer, bufferLen, L"%i.%i.%i.%i",
					ip % 256, (ip >> 8) % 256, (ip >> 16) % 256, (ip >> 24) % 256);
				break;
			}
		}
		break;

	case SysInfoType::GATEWAY_ADDRESS:
	case SysInfoType::GATEWAY_ADDRESS_V4:
	case SysInfoType::GATEWAY_ADDRESS_V6:
		{
			ULONG family = m_Type == SysInfoType::GATEWAY_ADDRESS_V6 ? AF_INET6 : AF_INET;
			ULONG adapterSize = 0UL;
			GetAdaptersAddresses(family, 0UL, nullptr, nullptr, &adapterSize);
			if (adapterSize <= 0UL) break;

			ULONG flags = GAA_FLAG_INCLUDE_GATEWAYS;
			auto tmp = std::make_unique<BYTE[]>(adapterSize);
			if (GetAdaptersAddresses(family, flags, nullptr,
				(PIP_ADAPTER_ADDRESSES)tmp.get(), &adapterSize) != ERROR_SUCCESS) break;

			PIP_ADAPTER_ADDRESSES info = (PIP_ADAPTER_ADDRESSES)tmp.get();
			for (; info != nullptr; info = info->Next)
			{
				if (info->IfIndex != m_Data) continue;

				PIP_ADAPTER_GATEWAY_ADDRESS gateway = info->FirstGatewayAddress;
				for (; gateway != nullptr; gateway = gateway->Next)
				{
					SOCKET_ADDRESS socket = info->FirstGatewayAddress->Address;
					switch (socket.lpSockaddr->sa_family)
					{
						case AF_INET:
						{
							sockaddr_in* inAddr = (sockaddr_in*)socket.lpSockaddr;
							RtlIpv4AddressToString(&inAddr->sin_addr, buffer);
						}
						break;

					case AF_INET6:
						{
							sockaddr_in6* inAddr = (sockaddr_in6*)socket.lpSockaddr;
							RtlIpv6AddressToString(&inAddr->sin6_addr, buffer);
						}
						break;
					}
				}
				break;
			}
		}
		break;

	case SysInfoType::TIMEZONE_STANDARD_NAME:
	case SysInfoType::TIMEZONE_DAYLIGHT_NAME:
		{
			TIME_ZONE_INFORMATION tzi = { 0 };
			GetTimeZoneInformation(&tzi);
			m_StringValue = m_Type == SysInfoType::TIMEZONE_STANDARD_NAME ?
				tzi.StandardName : tzi.DaylightName;
		}
		return;
	}

	if (buffer && *buffer)
	{
		m_StringValue = buffer;
	}
}

const WCHAR* MeasureSysInfo::GetStringValue()
{
	// Note: In the old plugin, |SysInfoType=IP_ADDRESS| would
	// return an empty string, instead of |nullptr|.
	return !m_StringValue.empty() || m_Type == SysInfoType::IP_ADDRESS
		? CheckSubstitute(m_StringValue.c_str())
		: nullptr;
}

NLM_CONNECTIVITY MeasureSysInfo::GetNetworkConnectivity()
{
	NLM_CONNECTIVITY connectivity = NLM_CONNECTIVITY_DISCONNECTED;

	INetworkListManager* nlm = { 0 };
	HRESULT hr = CoCreateInstance(
		CLSID_NetworkListManager, NULL, CLSCTX_INPROC_SERVER, __uuidof(INetworkListManager), (LPVOID*)&nlm);
	if (SUCCEEDED(hr))
	{
		nlm->GetConnectivity(&connectivity);
		nlm->Release();
	}

	return connectivity;
}
