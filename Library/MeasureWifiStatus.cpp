/* Copyright (C) 2020 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureWifiStatus.h"
#include "Rainmeter.h"
#include "../Common/StringUtil.h"

UINT MeasureWifiStatus::s_Instances = 0U;
HANDLE MeasureWifiStatus::s_Client = nullptr;
PWLAN_INTERFACE_INFO MeasureWifiStatus::s_Interface = nullptr;
PWLAN_INTERFACE_INFO_LIST MeasureWifiStatus::s_InterfaceList = nullptr;

MeasureWifiStatus::MeasureWifiStatus(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Type(MeasureType::UNINITIALIZED),
	m_ListStyle(0U),
	m_ListMax(5U),
	m_StatusString()
{
	++s_Instances;

	if (s_Instances == 1U)
	{
		// Create WINLAN API Handle
		if (!s_Client)
		{
			DWORD dwNegotiatedVersion = 0UL;
			DWORD dwErr =WlanOpenHandle(WLAN_API_VERSION, nullptr, &dwNegotiatedVersion, &s_Client);
			if (ERROR_SUCCESS != dwErr)
			{
				FinalizeHandle();
				LogDebugF(this,
					L"WifiStatus: Unable to open WLAN API Handle. Error code (%u): %s",
					dwErr, GetErrorString(dwErr));
				return;
			}
		}

		// Query list of WLAN interfaces
		if (!s_InterfaceList)
		{
			DWORD dwErr = WlanEnumInterfaces(s_Client, nullptr, &s_InterfaceList);
			if (ERROR_SUCCESS != dwErr)
			{
				FinalizeHandle();
				LogDebugF(this,
					L"WifiStatus: Unable to find any WLAN interfaces/adapters. Error code %u", dwErr);
				return;
			}
			else if (s_InterfaceList->dwNumberOfItems == 0UL)
			{
				FinalizeHandle();
				LogDebugF(this,
					L"WifiStatus: No WLAN interfaces/adapters available.");
				return;
			}
		}
	}
}

MeasureWifiStatus::~MeasureWifiStatus()
{
	if (s_Instances > 0U)
	{
		--s_Instances;

		if (s_Instances == 0U)
		{
			FinalizeHandle();
		}
	}
}

void MeasureWifiStatus::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	if (!s_Client) return;

	bool changed = false;

	int value = parser.ReadInt(section, L"WifiIntfID", 0);
	if (value >= (int)s_InterfaceList->dwNumberOfItems)
	{
		LogErrorF(this, L"WifiStatus: WifiIntfID=%i is not valid", value);
		value = 0;
	}
	s_Interface = &s_InterfaceList->InterfaceInfo[value];

	value = parser.ReadInt(section, L"WifiListStyle", 0);
	if (value < 0 || value > 7)
	{
		LogErrorF(this, L"WifiStatus: WifiListStyle=%i is not valid", value);
		value = 0;
	}
	m_ListStyle = value;

	value = parser.ReadInt(section, L"WifiListLimit", 5);
	if (value <= 0)
	{
		LogErrorF(this, L"WifiStatus: WifiListLimit=%i is not valid", value);
		value = 5;
	}
	m_ListMax = value;

	MeasureType infoType = MeasureType::UNKNOWN;
	LPCWSTR type = parser.ReadString(section, L"WifiInfoType", L"").c_str();
	if (_wcsicmp(L"SSID", type) == 0)
	{
		infoType = MeasureType::SSID;
	}
	else if (_wcsicmp(L"ENCRYPTION", type) == 0)
	{
		infoType = MeasureType::ENCRYPTION;
	}
	else if (_wcsicmp(L"AUTH", type) == 0)
	{
		infoType = MeasureType::AUTH;
	}
	else if (_wcsicmp(L"LIST", type) == 0)
	{
		infoType = MeasureType::LIST;
	}
	else if (_wcsicmp(L"PHY", type) == 0)
	{
		infoType = MeasureType::PHY;
	}
	else if (_wcsicmp(L"QUALITY", type) == 0)
	{
		infoType = MeasureType::QUALITY;
	}
	else if (_wcsicmp(L"TXRATE", type) == 0)
	{
		infoType = MeasureType::TXRATE;
	}
	else if (_wcsicmp(L"RXRATE", type) == 0)
	{
		infoType = MeasureType::RXRATE;
	}
	else
	{
		LogErrorF(this, L"WifiStatus: WifiInfoType=%s not valid", type);
	}

	if (infoType != m_Type)
	{
		changed = true;
	}
	m_Type = infoType;

	if (changed)
	{
		m_StatusString.clear();

		switch (infoType)
		{
		case MeasureType::SSID:
		case MeasureType::ENCRYPTION:
		case MeasureType::AUTH:
			m_MaxValue = 0.0;
			break;
		case MeasureType::QUALITY:
			m_MaxValue = 100.0;
			break;
		}
	}
}

void MeasureWifiStatus::UpdateValue()
{
	if (!s_Interface || m_Type == MeasureType::UNKNOWN)
	{
		m_Value = 0.0;
		return;
	}

	if (m_Type == MeasureType::LIST)
	{
		PWLAN_AVAILABLE_NETWORK_LIST pwnl = nullptr;
		DWORD dwErr =
			WlanGetAvailableNetworkList(s_Client, &s_Interface->InterfaceGuid, 0UL, nullptr, &pwnl);

		if (ERROR_SUCCESS != dwErr)
		{
			m_StatusString = L"Error";
		}
		else
		{
			// Size of network name can be up to 64 chars, set to 80 to add room for delimiters
			m_StatusString.clear();
			m_StatusString.reserve(80 * m_ListMax);

			UINT printed = 0U;  // count of how many networks have been printed already

			// Check all items in WLAN NETWORK LIST
			for (size_t i = 0ULL; i < pwnl->dwNumberOfItems ; ++i)
			{
				if (printed == m_ListMax) break;

				// SSID is in UCHAR, convert to WCHAR
				std::wstring ssid = StringUtil::Widen(
					(LPCSTR)pwnl->Network[i].dot11Ssid.ucSSID, (int)pwnl->Network[i].dot11Ssid.uSSIDLength);

				// Prevent duplicates that result from profiles, check using SSID
				if (!ssid.empty() && ssid[0] && wcsstr(m_StatusString.c_str(), ssid.c_str()) == nullptr)
				{
					++printed;
					m_StatusString += ssid;
					if (m_ListStyle > 0U)
					{
						if (m_ListStyle == 1U || m_ListStyle == 3U || m_ListStyle == 5U || m_ListStyle == 7U)
						{
							// ADD PHY type
							m_StatusString += L" @";
							m_StatusString += GetPHYString(pwnl->Network[i].dot11PhyTypes[0]);
						}
						if (m_ListStyle == 2U || m_ListStyle == 3U || m_ListStyle == 6U || m_ListStyle == 7U)
						{
							// ADD cipher and authentication
							m_StatusString += L" (";
							m_StatusString += GetCipherAlgorithmString(pwnl->Network[i].dot11DefaultCipherAlgorithm);
							m_StatusString += L':';
							m_StatusString += GetAuthAlgorithmString(pwnl->Network[i].dot11DefaultAuthAlgorithm);
							m_StatusString += L')';
						}
						if (m_ListStyle == 4U || m_ListStyle == 5U || m_ListStyle == 6U || m_ListStyle == 7U)
						{
							// ADD signal quality
							WCHAR buffer[32];
							_snwprintf_s(buffer, _TRUNCATE, L"%lu", (ULONG)pwnl->Network[i].wlanSignalQuality);

							m_StatusString += L" [";
							m_StatusString += buffer;
							m_StatusString += L']';
						}
					}
					m_StatusString += L'\n';
				}
			}

			WlanFreeMemory(pwnl);
		}
	}
	else
	{
		ULONG outsize = 0UL;
		PWLAN_CONNECTION_ATTRIBUTES wlan_cattr = nullptr;
		DWORD dwErr = WlanQueryInterface(s_Client,
			&s_Interface->InterfaceGuid, wlan_intf_opcode_current_connection, nullptr, &outsize, (PVOID*)&wlan_cattr, nullptr);

		if (ERROR_SUCCESS != dwErr)
		{
			switch (m_Type)
			{
			case MeasureType::SSID:
			case MeasureType::PHY:
			case MeasureType::ENCRYPTION:
			case MeasureType::AUTH:
				m_StatusString = L"-1";
				break;
			case MeasureType::QUALITY:
				m_Value = 0.0;
				break;
			}
		}
		else
		{
			switch (m_Type)
			{
			case MeasureType::SSID:
				// Need to convert ucSSID to wchar from uchar
				m_StatusString = StringUtil::Widen(
					(LPCSTR)wlan_cattr->wlanAssociationAttributes.dot11Ssid.ucSSID,
					(int)wlan_cattr->wlanAssociationAttributes.dot11Ssid.uSSIDLength);

				// If not connected yet add current status
				m_StatusString += GetInterfaceStateString(wlan_cattr->isState);
				break;

			case MeasureType::PHY:
				m_StatusString = GetPHYString(wlan_cattr->wlanAssociationAttributes.dot11PhyType);
				break;

			case MeasureType::ENCRYPTION:
				m_StatusString = GetCipherAlgorithmString(wlan_cattr->wlanSecurityAttributes.dot11CipherAlgorithm);
				break;

			case MeasureType::AUTH:
				m_StatusString = GetAuthAlgorithmString(wlan_cattr->wlanSecurityAttributes.dot11AuthAlgorithm);
				break;

			case MeasureType::QUALITY:
				m_Value = (double)wlan_cattr->wlanAssociationAttributes.wlanSignalQuality;
				break;

			case MeasureType::TXRATE:
				m_Value = (double)wlan_cattr->wlanAssociationAttributes.ulRxRate;
				break;

			case MeasureType::RXRATE:
				m_Value = (double)wlan_cattr->wlanAssociationAttributes.ulRxRate;
				break;

			default:  // Invalid type
				m_StatusString.clear();
				break;
			}

			WlanFreeMemory(wlan_cattr);
		}
	}
}

const WCHAR* MeasureWifiStatus::GetStringValue()
{
	if (!s_Interface) return nullptr;

	switch (m_Type)
	{
	case MeasureType::LIST:
	case MeasureType::SSID:
	case MeasureType::PHY:
	case MeasureType::ENCRYPTION:
	case MeasureType::AUTH:
		return m_StatusString.c_str();

	default:
		return nullptr;
	}
}

void MeasureWifiStatus::FinalizeHandle()
{
	s_Interface = nullptr;

	if (s_InterfaceList)
	{
		WlanFreeMemory(s_InterfaceList);
		s_InterfaceList = nullptr;
	}

	if (s_Client)
	{
		WlanCloseHandle(s_Client, nullptr);
		s_Client = nullptr;
	}
}

const WCHAR* MeasureWifiStatus::GetCipherAlgorithmString(DOT11_CIPHER_ALGORITHM value)
{
	switch (value)
	{
	case DOT11_CIPHER_ALGO_NONE: return L"NONE";
	case DOT11_CIPHER_ALGO_WEP40: return L"WEP40";
	case DOT11_CIPHER_ALGO_TKIP: return L"TKIP";
	case DOT11_CIPHER_ALGO_CCMP: return L"AES";
	case DOT11_CIPHER_ALGO_WEP104: return L"WEP104";
	case DOT11_CIPHER_ALGO_BIP: return L"BIP";
	case DOT11_CIPHER_ALGO_GCMP: return L"GCMP";
	case DOT11_CIPHER_ALGO_WPA_USE_GROUP: return L"WPA-GROUP";
	case DOT11_CIPHER_ALGO_WEP: return L"WEP";
	default: return L"???";
	}
}

const WCHAR* MeasureWifiStatus::GetAuthAlgorithmString(DOT11_AUTH_ALGORITHM value)
{
	switch (value)
	{
	case DOT11_AUTH_ALGO_80211_OPEN: return L"Open";
	case DOT11_AUTH_ALGO_80211_SHARED_KEY: return L"Shared";
	case DOT11_AUTH_ALGO_WPA: return L"WPA-Enterprise";
	case DOT11_AUTH_ALGO_WPA_PSK: return L"WPA-Personal";
	case DOT11_AUTH_ALGO_WPA_NONE: return L"WPA-NONE";
	case DOT11_AUTH_ALGO_RSNA: return L"WPA2-Enterprise";
	case DOT11_AUTH_ALGO_RSNA_PSK: return L"WPA2-Personal";
	case DOT11_AUTH_ALGO_WPA3: return L"WPA3-Enterprise";
	case DOT11_AUTH_ALGO_WPA3_SAE: return L"WPA3-Personal";
	default: return L"???";
	}
}

const WCHAR* MeasureWifiStatus::GetInterfaceStateString(WLAN_INTERFACE_STATE value)
{
	switch (value)
	{
	case wlan_interface_state_connected: return L"";
	case wlan_interface_state_authenticating: return L"(authorizing...)";
	default: return L"(connecting...)";
	}
}

const WCHAR* MeasureWifiStatus::GetPHYString(DOT11_PHY_TYPE value)
{
	switch (value)
	{
	case dot11_phy_type_fhss: return L"FHSS";
	case dot11_phy_type_dsss: return L"DSSS";
	case dot11_phy_type_irbaseband: return L"IR-Band";
	case dot11_phy_type_ofdm: return L"802.11a";
	case dot11_phy_type_hrdsss: return L"802.11b";
	case dot11_phy_type_erp: return L"802.11g";
	case dot11_phy_type_ht: return L"802.11n";
	case dot11_phy_type_vht: return L"802.11ac";
	case dot11_phy_type_dmg: return L"802.11ad";
	case dot11_phy_type_he: return L"802.11ax";
	default: return L"???";
	}
}

const WCHAR* MeasureWifiStatus::GetErrorString(DWORD value)
{
	switch (value)
	{
	case ERROR_INVALID_PARAMETER: return L"Invalid parameters";
	case ERROR_NOT_ENOUGH_MEMORY: return L"Not enough memory";
	case ERROR_REMOTE_SESSION_LIMIT_EXCEEDED: return L"Too many handles already issued";
	case ERROR_SERVICE_NOT_ACTIVE: return L"WLAN Auto Config (WLANSVC) service is not active";
	default: return L"Unknown error code";
	}
}
