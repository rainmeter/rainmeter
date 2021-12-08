/* Copyright (C) 2021 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "NetworkUtil.h"

ULONG NetworkUtil::s_InterfaceCount = 0UL;
MIB_IF_TABLE2* NetworkUtil::s_InterfaceTable = nullptr;

void NetworkUtil::Initialize()
{
}

void NetworkUtil::Finalize()
{
	DisposeInterfaceTable();
}

bool NetworkUtil::UpdateInterfaceTable()
{
	static ULONGLONG s_LastUpdateTickCount = 0ULL;
	const ULONGLONG updateInterval = 250ULL; // ms

	ULONGLONG tickCount = GetTickCount64();
	if (tickCount >= (s_LastUpdateTickCount + updateInterval))
	{
		s_LastUpdateTickCount = tickCount;

		DisposeInterfaceTable();
		if (GetIfTable2(&s_InterfaceTable) == NO_ERROR)
		{
			s_InterfaceCount = s_InterfaceTable->NumEntries;
			return true;
		}
	}
	return false;
}

ULONG NetworkUtil::FindBestInterface(LPCWSTR interfaceName)
{
	if (s_InterfaceTable)
	{
		if (_wcsicmp(interfaceName, L"BEST") == 0)
		{
			DWORD dwBestIndex = 0;
			if (NO_ERROR == GetBestInterface(INADDR_ANY, &dwBestIndex))
			{
				return (ULONG)dwBestIndex;
			}
		}
		else
		{
			auto findIndex = [&](bool useAlias) -> ULONG
			{
				ULONG firstMatch = ULONG_MAX;
				ULONG bestStatus = ULONG_MAX;
				ULONG bestConnected = ULONG_MAX;
				ULONG bestDisconnected = ULONG_MAX;
				MIB_IF_ROW2* table = s_InterfaceTable->Table;

				for (ULONG i = 0UL; i < s_InterfaceCount; ++i)
				{
					bool found = useAlias ?
						(_wcsicmp(interfaceName, table[i].Alias) == 0) :
						(_wcsicmp(interfaceName, table[i].Description) == 0);
					if (found)
					{
						if (firstMatch == ULONG_MAX)
						{
							firstMatch = table[i].InterfaceIndex;
						}

						bool isPresent = table[i].OperStatus != IfOperStatusNotPresent;
						bool isConnected = table[i].MediaConnectState == MediaConnectStateConnected;
						bool isDisconnected = table[i].MediaConnectState == MediaConnectStateDisconnected;

						if (isPresent && isConnected)
						{
							return table[i].InterfaceIndex;
						}

						if (isPresent) bestStatus = table[i].InterfaceIndex;
						if (isConnected) bestConnected = table[i].InterfaceIndex;
						if (isDisconnected) bestDisconnected = table[i].InterfaceIndex;
					}
				}

				if (bestConnected != ULONG_MAX)         return bestConnected;
				else if (bestStatus != ULONG_MAX)       return bestStatus;
				else if (bestDisconnected != ULONG_MAX) return bestDisconnected;
				else if (firstMatch != ULONG_MAX)       return firstMatch;

				return ULONG_MAX;
			};

			ULONG index = findIndex(false);			// Search for "Description" first
			if (index == ULONG_MAX)
			{
				index = findIndex(true);			// If no "Description" is matched, search for "Alias"
			}
			if (index != ULONG_MAX) return index;
		}
	}
	return 0UL;
}

ULONG NetworkUtil::GetIndexFromIfIndex(const ULONG ifIndex)
{
	if (s_InterfaceTable)
	{
		for (ULONG i = 0; i < s_InterfaceCount; ++i)
		{
			MIB_IF_ROW2* table = s_InterfaceTable->Table;
			if (ifIndex == table[i].InterfaceIndex)
			{
				return i;
			}
		}
	}
	return 0UL;
}

LPCWSTR NetworkUtil::GetInterfaceTypeString(const ULONG ifIndex)
{
	switch (ifIndex)
	{
	case IF_TYPE_ETHERNET_CSMACD:   return L"Ethernet";
	case IF_TYPE_FDDI:              return L"Fiber";
	case IF_TYPE_PPP:               return L"PPP";
	case IF_TYPE_SOFTWARE_LOOPBACK: return L"Loopback";
	case IF_TYPE_IEEE80211:         return L"Wireless";
	case IF_TYPE_TUNNEL:            return L"Tunnel";
	case IF_TYPE_IEEE1394:          return L"Firewire";
	case IF_TYPE_IEEE80216_WMAN:    return L"Mobile WiMax";
	case IF_TYPE_WWANPP:            return L"Mobile GSM";
	case IF_TYPE_WWANPP2:           return L"Mobile CDMA";
	}
	return L"Other";
}

LPCWSTR NetworkUtil::GetInterfaceMediaConnectionString(const NET_IF_MEDIA_CONNECT_STATE state)
{
	switch (state)
	{
	case MediaConnectStateConnected:    return L"Connected";
	case MediaConnectStateDisconnected: return L"Disconnected";
	}
	return L"Unknown";
}

LPCWSTR NetworkUtil::GetInterfaceOperStatusString(const IF_OPER_STATUS status)
{
	switch (status)
	{
	case IfOperStatusUp:             return L"Up";
	case IfOperStatusDown:           return L"Down";
	case IfOperStatusTesting:        return L"Testing";
	case IfOperStatusDormant:        return L"Dormant";
	case IfOperStatusNotPresent:     return L"Not Present";
	case IfOperStatusLowerLayerDown: return L"Lower Layer Down";
	}
	return L"Unknown";
}

void NetworkUtil::DisposeInterfaceTable()
{
	if (s_InterfaceTable)
	{
		FreeMibTable(s_InterfaceTable);
		s_InterfaceTable = nullptr;
	}
	s_InterfaceCount = 0UL;
}
