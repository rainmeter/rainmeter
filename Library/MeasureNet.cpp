/*
  Copyright (C) 2001 Kimmo Pekkola

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

#include "StdAfx.h"
#include "MeasureNet.h"
#include "Rainmeter.h"
#include "System.h"

std::vector<ULONG64> CMeasureNet::c_StatValues;
std::vector<ULONG64> CMeasureNet::c_OldStatValues;
BYTE* CMeasureNet::c_Table = NULL;
ULONG CMeasureNet::c_Size = 0;
UINT CMeasureNet::c_NumOfTables = 0;
BYTE* CMeasureNet::c_AATable = NULL;
ULONG CMeasureNet::c_AASize = 0;
bool CMeasureNet::c_IpInterfaceChanged = true;
HANDLE CMeasureNet::c_NotificationHandle = NULL;
FPGETIFENTRY2 CMeasureNet::c_GetIfEntry2 = NULL;
FPNOTIFYIPINTERFACECHANGE CMeasureNet::c_NotifyIpInterfaceChange = NULL;
FPCANCELMIBCHANGENOTIFY2 CMeasureNet::c_CancelMibChangeNotify2 = NULL;

extern CRainmeter* Rainmeter;

/*
** CMeasureNet
**
** The constructor. This is the base class for the net-meters.
**
*/
CMeasureNet::CMeasureNet(CMeterWindow* meterWindow, const WCHAR* name) : CMeasure(meterWindow, name),
	m_CurrentTraffic(),
	m_TrafficValue(),
	m_Interface(),
	m_Cumulative(false)
{
}

/*
** ~CMeasureNet
**
** The destructor
**
*/
CMeasureNet::~CMeasureNet()
{
}

/*
** Update
**
** Checks if Action should be executed.
**
*/
bool CMeasureNet::Update()
{
	if (!CMeasure::PreUpdate()) return false;

	if (m_MeterWindow)
	{
		if (!m_TrafficAction.empty())
		{
			if (m_CurrentTraffic > m_TrafficValue)
			{
				m_CurrentTraffic = 0;
				Rainmeter->ExecuteCommand(m_TrafficAction.c_str(), m_MeterWindow);
			}

			m_CurrentTraffic += m_Value;
		}
	}

	return PostUpdate();
}

/*
** IpInterfaceChangeCallback
**
*/
VOID NETIOAPI_API_ CMeasureNet::IpInterfaceChangeCallback(PVOID CallerContext, PMIB_IPINTERFACE_ROW Row, MIB_NOTIFICATION_TYPE NotificationType)
{
	if (NotificationType == MibAddInstance || NotificationType == MibDeleteInstance)
	{
		c_IpInterfaceChanged = true;
	}
}

/*
** UpdateIFTable
**
** Reads the tables for all net interfaces
**
*/
void CMeasureNet::UpdateIFTable()
{
	if (c_IpInterfaceChanged || c_NotificationHandle == NULL || c_AATable == NULL)
	{
		// Gotta reserve few bytes for the tables
		ULONG flags = GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST;

		ULONG ret = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, (PIP_ADAPTER_ADDRESSES)c_AATable, &c_AASize);
		if (ret == ERROR_BUFFER_OVERFLOW)
		{
			delete [] c_AATable;
			c_AATable = new BYTE[c_AASize];

			ret = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, (PIP_ADAPTER_ADDRESSES)c_AATable, &c_AASize);
		}

		if (ret == ERROR_SUCCESS)
		{
			UINT numOfTables = 0;
			for (PIP_ADAPTER_ADDRESSES addrPtr = (PIP_ADAPTER_ADDRESSES)c_AATable; addrPtr != NULL; addrPtr = addrPtr->Next)
			{
				++numOfTables;
			}

			if (c_NumOfTables != numOfTables)
			{
				c_NumOfTables = numOfTables;
				delete [] c_Table;

				if (c_NumOfTables > 0)
				{
					c_Size = ((c_GetIfEntry2) ? sizeof(MIB_IF_ROW2) : sizeof(MIB_IFROW)) * c_NumOfTables;
					c_Table = new BYTE[c_Size];
				}
				else
				{
					c_Table = NULL;
					c_Size = 0;
				}

				c_IpInterfaceChanged = true;
			}

			if (c_IpInterfaceChanged && CRainmeter::GetDebug())
			{
				Log(LOG_DEBUG, L"------------------------------");
				LogWithArgs(LOG_DEBUG, L"* NETWORK-INTERFACE: %s", (c_GetIfEntry2) ? L"GetIfEntry2" : L"GetIfEntry");

				if (c_Table)
				{
					int i = 0;
					for (PIP_ADAPTER_ADDRESSES addrPtr = (PIP_ADAPTER_ADDRESSES)c_AATable; addrPtr != NULL; addrPtr = addrPtr->Next)
					{
						std::wstring type;
						switch (addrPtr->IfType)
						{
						case IF_TYPE_ETHERNET_CSMACD:
							type += L"Ethernet";
							break;
						case IF_TYPE_PPP:
							type += L"PPP";
							break;
						case IF_TYPE_SOFTWARE_LOOPBACK:
							type += L"Loopback";
							break;
						case IF_TYPE_IEEE80211:
							type += L"IEEE802.11";
							break;
						case IF_TYPE_TUNNEL:
							type += L"Tunnel";
							break;
						case IF_TYPE_IEEE1394:
							type += L"IEEE1394";
							break;
						default:
							type += L"Other";
							break;
						}

						std::wstring oper;
						switch (addrPtr->OperStatus)
						{
						case IfOperStatusUp:
							oper += L"Up";
							break;
						case IfOperStatusDown:
							oper += L"Down";
							break;
						case IfOperStatusTesting:
							oper += L"Testing";
							break;
						case IfOperStatusUnknown:
							oper += L"Unknown";
							break;
						case IfOperStatusDormant:
							oper += L"Dormant";
							break;
						case IfOperStatusNotPresent:
							oper += L"NotPresent";
							break;
						case IfOperStatusLowerLayerDown:
							oper += L"LowerLayerDown";
							break;
						}

						LogWithArgs(LOG_DEBUG, L"%i: %s", ++i, addrPtr->Description);
						LogWithArgs(LOG_DEBUG, L"  IfIndex=%u, LUID=%llu, AdapterName=%s", addrPtr->IfIndex, addrPtr->Luid.Value, ConvertToWide(addrPtr->AdapterName).c_str());
						LogWithArgs(LOG_DEBUG, L"  Type=%s (%i), Oper=%s (%i)", type.c_str(), addrPtr->IfType, oper.c_str(), addrPtr->OperStatus);
					}
				}
				else
				{
					Log(LOG_DEBUG, L"None.");
				}

				Log(LOG_DEBUG, L"------------------------------");
			}
		}
		else
		{
			// Something's wrong. Unable to get the table.
			DisposeBuffer();
		}

		c_IpInterfaceChanged = false;
	}

	if (c_AATable && c_Table)
	{
		memset(c_Table, 0, c_Size);

		if (c_GetIfEntry2)
		{
			PMIB_IF_ROW2 ifRowPtr = (PMIB_IF_ROW2)c_Table;
			for (PIP_ADAPTER_ADDRESSES addrPtr = (PIP_ADAPTER_ADDRESSES)c_AATable; addrPtr != NULL; addrPtr = addrPtr->Next)
			{
				(*ifRowPtr).InterfaceIndex = addrPtr->IfIndex;
				c_GetIfEntry2(ifRowPtr);
				++ifRowPtr;
			}
		}
		else
		{
			PMIB_IFROW ifRowPtr = (PMIB_IFROW)c_Table;
			for (PIP_ADAPTER_ADDRESSES addrPtr = (PIP_ADAPTER_ADDRESSES)c_AATable; addrPtr != NULL; addrPtr = addrPtr->Next)
			{
				(*ifRowPtr).dwIndex = addrPtr->IfIndex;
				GetIfEntry(ifRowPtr);
				++ifRowPtr;
			}
		}
	}
}

/*
** GetNetOctets
**
** Reads the amount of octets. This is the same for in, out and total.
** the net-parameter informs which inherited class called this method.
**
*/
ULONG64 CMeasureNet::GetNetOctets(NET net)
{
	ULONG64 value = 0;

	if (c_GetIfEntry2)
	{
		PMIB_IF_ROW2 table = (PMIB_IF_ROW2)c_Table;

		if (m_Interface == 0)
		{
			// Get all interfaces
			for (UINT i = 0; i < c_NumOfTables; ++i)
			{
				// Ignore the loopback interface
				if (table[i].Type == IF_TYPE_SOFTWARE_LOOPBACK) continue;

				switch (net)
				{
				case NET_IN:
					value += table[i].InOctets;
					break;

				case NET_OUT:
					value += table[i].OutOctets;
					break;

				case NET_TOTAL:
					value += table[i].InOctets;
					value += table[i].OutOctets;
					break;
				}
			}
		}
		else
		{
			// Get the selected interface
			if (m_Interface <= c_NumOfTables)
			{
				switch (net)
				{
				case NET_IN:
					value += table[m_Interface - 1].InOctets;
					break;

				case NET_OUT:
					value += table[m_Interface - 1].OutOctets;
					break;

				case NET_TOTAL:
					value += table[m_Interface - 1].InOctets;
					value += table[m_Interface - 1].OutOctets;
					break;
				}
			}
		}
	}
	else
	{
		PMIB_IFROW table = (PMIB_IFROW)c_Table;

		if (m_Interface == 0)
		{
			// Get all interfaces
			for (UINT i = 0; i < c_NumOfTables; ++i)
			{
				// Ignore the loopback interface
				if (table[i].dwType == IF_TYPE_SOFTWARE_LOOPBACK) continue;

				switch (net)
				{
				case NET_IN:
					value += table[i].dwInOctets;
					break;

				case NET_OUT:
					value += table[i].dwOutOctets;
					break;

				case NET_TOTAL:
					value += table[i].dwInOctets;
					value += table[i].dwOutOctets;
					break;
				}
			}
		}
		else
		{
			// Get the selected interface
			if (m_Interface <= c_NumOfTables)
			{
				switch (net)
				{
				case NET_IN:
					value += table[m_Interface - 1].dwInOctets;
					break;

				case NET_OUT:
					value += table[m_Interface - 1].dwOutOctets;
					break;

				case NET_TOTAL:
					value += table[m_Interface - 1].dwInOctets;
					value += table[m_Interface - 1].dwOutOctets;
					break;
				}
			}
		}
	}

	return value;
}

/*
** GetNetStatsValue
**
** Returns the stats value of the interface
**
*/
ULONG64 CMeasureNet::GetNetStatsValue(NET net)
{
	ULONG64 value = 0;
	size_t statsSize = c_StatValues.size() / 2;

	if (m_Interface == 0)
	{
		// Get all interfaces
		for (size_t i = 0; i < statsSize; ++i)
		{
			// Ignore the loopback interface
			if (c_NumOfTables == statsSize)
			{
				if (c_GetIfEntry2)
				{
					if (((PMIB_IF_ROW2)c_Table)[i].Type == IF_TYPE_SOFTWARE_LOOPBACK) continue;
				}
				else
				{
					if (((PMIB_IFROW)c_Table)[i].dwType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
				}
			}

			switch (net)
			{
			case NET_IN:
				value += c_StatValues[i * 2 + 0];
				break;

			case NET_OUT:
				value += c_StatValues[i * 2 + 1];
				break;

			case NET_TOTAL:
				value += c_StatValues[i * 2 + 0];
				value += c_StatValues[i * 2 + 1];
				break;
			}
		}
	}
	else
	{
		// Get the selected interface
		if (m_Interface <= statsSize)
		{
			switch (net)
			{
			case NET_IN:
				value += c_StatValues[(m_Interface - 1) * 2 + 0];
				break;

			case NET_OUT:
				value += c_StatValues[(m_Interface - 1) * 2 + 1];
				break;

			case NET_TOTAL:
				value += c_StatValues[(m_Interface - 1) * 2 + 0];
				value += c_StatValues[(m_Interface - 1) * 2 + 1];
				break;
			}
		}
	}

	return value;
}

/*
** ReadConfig
**
** Reads the measure specific configs. This is the same for in, out and total.
** the net-parameter informs which inherited class called this method.
**
*/
void CMeasureNet::ReadConfig(CConfigParser& parser, const WCHAR* section, NET net)
{
	double value;
	const WCHAR* netName = NULL;

	if (net == NET_IN)
	{
		netName = L"NetInSpeed";
		value = CRainmeter::GetGlobalConfig().netInSpeed;
	}
	else if (net == NET_OUT)
	{
		netName = L"NetOutSpeed";
		value = CRainmeter::GetGlobalConfig().netOutSpeed;
	}
	else
	{
		netName = L"NetTotalSpeed";
		value = CRainmeter::GetGlobalConfig().netInSpeed + CRainmeter::GetGlobalConfig().netOutSpeed;
	}

	double maxValue = parser.ReadFloat(section, L"MaxValue", -1);
	if (maxValue == -1)
	{
		maxValue = parser.ReadFloat(section, netName, -1);
		if (maxValue == -1)
		{
			maxValue = value;
		}
	}

	m_Interface = parser.ReadInt(section, L"Interface", 0);
	m_Cumulative = 0!=parser.ReadInt(section, L"Cumulative", 0);

	m_TrafficValue = parser.ReadFloat(section, L"TrafficValue", 0.0);
	m_TrafficAction = parser.ReadString(section, L"TrafficAction", L"", false);

	if (maxValue == 0)
	{
		m_MaxValue = 1;
		m_LogMaxValue = true;
	}
	else
	{
		m_MaxValue = maxValue / 8;
	}
}

/*
** UpdateStats
**
** Updates the statistics.
**
*/
void CMeasureNet::UpdateStats()
{
	if (c_Table)
	{
		size_t statsSize = c_NumOfTables * 2;

		// Fill the vectors
		while (c_StatValues.size() < statsSize)
		{
			c_StatValues.push_back(0);
		}

		while (c_OldStatValues.size() < statsSize)
		{
			c_OldStatValues.push_back(0);
		}

		for (UINT i = 0; i < c_NumOfTables; ++i)
		{
			ULONG64 in, out;

			if (c_GetIfEntry2)
			{
				in = ((PMIB_IF_ROW2)c_Table)[i].InOctets;
				out = ((PMIB_IF_ROW2)c_Table)[i].OutOctets;
			}
			else
			{
				in = ((PMIB_IFROW)c_Table)[i].dwInOctets;
				out = ((PMIB_IFROW)c_Table)[i].dwOutOctets;
			}

			if (c_OldStatValues[i * 2 + 0] != 0 && c_OldStatValues[i * 2 + 1] != 0)
			{
				if (in > c_OldStatValues[i * 2 + 0])
				{
					c_StatValues[i * 2 + 0] += in - c_OldStatValues[i * 2 + 0];
				}
				if (out > c_OldStatValues[i * 2 + 1])
				{
					c_StatValues[i * 2 + 1] += out - c_OldStatValues[i * 2 + 1];
				}
			}

			c_OldStatValues[i * 2 + 0] = in;
			c_OldStatValues[i * 2 + 1] = out;
		}
	}
}

/*
** ResetStats
**
** Resets the statistics.
**
*/
void CMeasureNet::ResetStats()
{
	c_StatValues.clear();
}

/*
** ReadStats
**
** Reads statistics.
**
*/
void CMeasureNet::ReadStats(const WCHAR* iniFile)
{
	WCHAR buffer[64];

	CConfigParser parser;
	parser.Initialize(iniFile, NULL, NULL, L"Statistics");

	int count = parser.ReadInt(L"Statistics", L"NetStatsCount", 0);

	c_StatValues.clear();

	for (int i = 1; i <= count; ++i)
	{
		ULARGE_INTEGER value;

		_snwprintf_s(buffer, _TRUNCATE, L"NetStatsInHigh%i", i);
		value.HighPart = (DWORD)parser.ReadUInt(L"Statistics", buffer, 0);

		_snwprintf_s(buffer, _TRUNCATE, L"NetStatsInLow%i", i);
		value.LowPart = (DWORD)parser.ReadUInt(L"Statistics", buffer, 0);

		c_StatValues.push_back(value.QuadPart);

		_snwprintf_s(buffer, _TRUNCATE, L"NetStatsOutHigh%i", i);
		value.HighPart = (DWORD)parser.ReadUInt(L"Statistics", buffer, 0);

		_snwprintf_s(buffer, _TRUNCATE, L"NetStatsOutLow%i", i);
		value.LowPart = (DWORD)parser.ReadUInt(L"Statistics", buffer, 0);

		c_StatValues.push_back(value.QuadPart);
	}
}

/*
** WriteStats
**
** Writes statistics.
**
*/
void CMeasureNet::WriteStats(const WCHAR* iniFile)
{
	WCHAR buffer[32];
	WCHAR buffer2[64];

	size_t statsSize = c_StatValues.size() / 2;

	_snwprintf_s(buffer, _TRUNCATE, L"%i", (int)statsSize);
	WritePrivateProfileString(L"Statistics", L"NetStatsCount", buffer, iniFile);

	for (size_t i = 0; i < statsSize; ++i)
	{
		ULARGE_INTEGER value;

		value.QuadPart = c_StatValues[i * 2];

		_snwprintf_s(buffer2, _TRUNCATE, L"NetStatsInHigh%i", (int)i + 1);
		_snwprintf_s(buffer, _TRUNCATE, L"%u", value.HighPart);
		WritePrivateProfileString(L"Statistics", buffer2, buffer, iniFile);

		_snwprintf_s(buffer2, _TRUNCATE, L"NetStatsInLow%i", (int)i + 1);
		_snwprintf_s(buffer, _TRUNCATE, L"%u", value.LowPart);
		WritePrivateProfileString(L"Statistics", buffer2, buffer, iniFile);

		value.QuadPart = c_StatValues[i * 2 + 1];

		_snwprintf_s(buffer2, _TRUNCATE, L"NetStatsOutHigh%i", (int)i + 1);
		_snwprintf_s(buffer, _TRUNCATE, L"%u", value.HighPart);
		WritePrivateProfileString(L"Statistics", buffer2, buffer, iniFile);

		_snwprintf_s(buffer2, _TRUNCATE, L"NetStatsOutLow%i", (int)i + 1);
		_snwprintf_s(buffer, _TRUNCATE, L"%u", value.LowPart);
		WritePrivateProfileString(L"Statistics", buffer2, buffer, iniFile);
	}
}

/*
** InitializeNewApi
**
** Prepares in order to use the new APIs which are available on Vista or newer.
**
*/
void CMeasureNet::InitializeNewApi()
{
	HMODULE IpHlpApi = GetModuleHandle(L"IpHlpApi");
	c_GetIfEntry2 = (FPGETIFENTRY2)GetProcAddress(IpHlpApi, "GetIfEntry2");

	if (c_GetIfEntry2)
	{
		c_NotifyIpInterfaceChange = (FPNOTIFYIPINTERFACECHANGE)GetProcAddress(IpHlpApi, "NotifyIpInterfaceChange");
		c_CancelMibChangeNotify2 = (FPCANCELMIBCHANGENOTIFY2)GetProcAddress(IpHlpApi, "CancelMibChangeNotify2");

		if (c_NotifyIpInterfaceChange && c_CancelMibChangeNotify2)
		{
			c_NotifyIpInterfaceChange(AF_UNSPEC, (PIPINTERFACE_CHANGE_CALLBACK)IpInterfaceChangeCallback, NULL, FALSE, &c_NotificationHandle);

			if (c_NotificationHandle == NULL)
			{
				c_NotifyIpInterfaceChange = NULL;
				c_CancelMibChangeNotify2 = NULL;
				Log(LOG_ERROR, L"NotifyIpInterfaceChange function failed.");
			}
		}
		else
		{
			c_NotifyIpInterfaceChange = NULL;
			c_CancelMibChangeNotify2 = NULL;
			Log(LOG_ERROR, L"NotifyIpInterfaceChange/CancelMibChangeNotify2 function not found in IpHlpApi.dll.");
		}
	}

	if (CRainmeter::GetDebug())
	{
		UpdateIFTable();
	}
}

/*
** FinalizeNewApi
**
** Frees the resources.
**
*/
void CMeasureNet::FinalizeNewApi()
{
	if (c_NotificationHandle)
	{
		c_CancelMibChangeNotify2(c_NotificationHandle);
		c_NotificationHandle = NULL;
	}

	c_GetIfEntry2 = NULL;
	c_NotifyIpInterfaceChange = NULL;
	c_CancelMibChangeNotify2 = NULL;

	DisposeBuffer();
}

/*
** DisposeBuffer
**
*/
void CMeasureNet::DisposeBuffer()
{
	delete [] c_AATable;
	c_AATable = NULL;
	c_AASize = 0;

	delete [] c_Table;
	c_Table = NULL;
	c_Size = 0;

	c_NumOfTables = 0;
}
