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

BYTE* CMeasureNet::c_Table = NULL;
UINT CMeasureNet::c_NumOfTables = 0;
std::vector<ULONG64> CMeasureNet::c_StatValues;
std::vector<ULONG64> CMeasureNet::c_OldStatValues;

HINSTANCE CMeasureNet::c_IpHlpApiLibrary = NULL;
FPGETIFTABLE2EX CMeasureNet::c_GetIfTable2Ex = NULL;
FPFREEMIBTABLE CMeasureNet::c_FreeMibTable = NULL;
bool CMeasureNet::c_UseNewApi = false;

extern CRainmeter* Rainmeter;

/*
** CMeasureNet
**
** The constructor. This is the base class for the net-meters. 
**
*/
CMeasureNet::CMeasureNet(CMeterWindow* meterWindow) : CMeasure(meterWindow)
{
	m_TrafficValue = 0;
	m_CurrentTraffic = 0;
	m_Interface = 0;
	m_Cumulative = false;
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
** UpdateIFTable
**
** Reads the tables for all net interfaces
**
*/
void CMeasureNet::UpdateIFTable()
{
	bool logging = false;

	if (c_UseNewApi)
	{
		if (c_Table)
		{
			c_FreeMibTable(c_Table);
			c_Table = NULL;
		}

		if (c_GetIfTable2Ex(MibIfTableNormal, (MIB_IF_TABLE2**)&c_Table) == NO_ERROR)
		{
			MIB_IF_TABLE2* ifTable = (MIB_IF_TABLE2*)c_Table;

			if (c_NumOfTables != ifTable->NumEntries)
			{
				c_NumOfTables = ifTable->NumEntries;
				logging = true;
			}

			if (CRainmeter::GetDebug() && logging)
			{
				DebugLog(L"------------------------------");
				DebugLog(L"* NETWORK-INTERFACE: Count=%i", c_NumOfTables);

				for (size_t i = 0; i < c_NumOfTables; ++i)
				{
					std::wstring type;
					switch (ifTable->Table[i].Type)
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

					DebugLog(L"%i: %s", i + 1, ifTable->Table[i].Description);
					DebugLog(L"  Type=%s(%i), Hardware=%s, Filter=%s",
						type.c_str(), ifTable->Table[i].Type,
						(ifTable->Table[i].InterfaceAndOperStatusFlags.HardwareInterface == 1) ? L"Yes" : L"No",
						(ifTable->Table[i].InterfaceAndOperStatusFlags.FilterInterface == 1) ? L"Yes" : L"No");
				}
				DebugLog(L"------------------------------");
			}
		}
		else
		{
			// Something's wrong. Unable to get the table.
			c_Table = NULL;
			c_NumOfTables = 0;
		}
	}
	else
	{
		if (c_Table == NULL)
		{
			// Gotta reserve few bytes for the tables
			DWORD value = 0;
			if (GetNumberOfInterfaces(&value) == NO_ERROR)
			{
				if (c_NumOfTables != value)
				{
					c_NumOfTables = value;
					logging = true;
				}

				if (c_NumOfTables > 0)
				{
					DWORD size = sizeof(MIB_IFTABLE) + sizeof(MIB_IFROW) * c_NumOfTables;
					c_Table = new BYTE[size];
				}
			}
		}

		if (c_Table)
		{
			DWORD ret, size = 0;

			MIB_IFTABLE* ifTable = (MIB_IFTABLE*)c_Table;

			if ((ret = GetIfTable(ifTable, &size, FALSE)) == ERROR_INSUFFICIENT_BUFFER)
			{
				delete [] c_Table;
				c_Table = new BYTE[size];

				ifTable = (MIB_IFTABLE*)c_Table;

				ret = GetIfTable(ifTable, &size, FALSE);
			}

			if (ret == NO_ERROR)
			{
				if (c_NumOfTables != ifTable->dwNumEntries)
				{
					c_NumOfTables = ifTable->dwNumEntries;
					logging = true;
				}

				if (CRainmeter::GetDebug() && logging)
				{
					DebugLog(L"------------------------------");
					DebugLog(L"* NETWORK-INTERFACE: Count=%i", c_NumOfTables);

					for (size_t i = 0; i < c_NumOfTables; ++i)
					{
						std::string desc((char*)ifTable->table[i].bDescr, ifTable->table[i].dwDescrLen);

						std::wstring type;
						switch (ifTable->table[i].dwType)
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

						DebugLog(L"%i: %s", i + 1, ConvertToWide(desc.c_str()).c_str());
						DebugLog(L"  Type=%s(%i)",
							type.c_str(), ifTable->table[i].dwType);
					}
					DebugLog(L"------------------------------");
				}
			}
			else
			{
				// Something's wrong. Unable to get the table.
				delete [] c_Table;
				c_Table = NULL;
				c_NumOfTables = 0;
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

	if (c_UseNewApi)
	{
		MIB_IF_ROW2* table = (MIB_IF_ROW2*)((MIB_IF_TABLE2*)c_Table)->Table;

		if (m_Interface == 0)
		{
			// Get all interfaces
			for (UINT i = 0; i < c_NumOfTables; ++i)
			{
				// Ignore the loopback and filter interfaces
				if (table[i].Type == IF_TYPE_SOFTWARE_LOOPBACK ||
					table[i].InterfaceAndOperStatusFlags.FilterInterface == 1) continue;

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
		MIB_IFROW* table = (MIB_IFROW*)((MIB_IFTABLE*)c_Table)->table;

		if (m_Interface == 0)
		{
			// Get all interfaces
			for (UINT i = 0; i < c_NumOfTables; ++i)
			{
				// Ignore the loopback
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

	if (m_Interface == 0)
	{
		// Get all interfaces
		for(size_t i = 0; i < c_StatValues.size() / 2; ++i)
		{
			// Ignore the loopback and filter interfaces
			if (c_NumOfTables == c_StatValues.size() / 2)
			{
				if (c_UseNewApi)
				{
					if (((MIB_IF_TABLE2*)c_Table)->Table[i].Type == IF_TYPE_SOFTWARE_LOOPBACK ||
						((MIB_IF_TABLE2*)c_Table)->Table[i].InterfaceAndOperStatusFlags.FilterInterface == 1) continue;
				}
				else
				{
					if (((MIB_IFTABLE*)c_Table)->table[i].dwType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
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
		if (m_Interface <= c_StatValues.size() / 2)
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
		// Fill the vectors
		while (c_StatValues.size() < c_NumOfTables * 2)
		{
			c_StatValues.push_back(0);
		}

		while (c_OldStatValues.size() < c_NumOfTables * 2)
		{ 
			c_OldStatValues.push_back(0);
		}

		for (UINT i = 0; i < c_NumOfTables; ++i)
		{
			ULONG64 in, out;

			if (c_UseNewApi)
			{
				in = (DWORD)((MIB_IF_TABLE2*)c_Table)->Table[i].InOctets;
				out = (DWORD)((MIB_IF_TABLE2*)c_Table)->Table[i].OutOctets;
			}
			else
			{
				in = ((MIB_IFTABLE*)c_Table)->table[i].dwInOctets;
				out = ((MIB_IFTABLE*)c_Table)->table[i].dwOutOctets;
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
	if (c_Table)
	{
		for (size_t i = 0; i < c_StatValues.size(); ++i)
		{
			c_StatValues[i] = 0;
		}
	}
}

/*
** ReadStats
**
** Reads statistics.
**
*/
void CMeasureNet::ReadStats(const std::wstring& iniFile)
{
	WCHAR buffer[64];
	int count = GetPrivateProfileInt(L"Statistics", L"NetStatsCount", 0, iniFile.c_str());

	c_StatValues.clear();

	for (int i = 0; i < count; ++i)
	{
		ULARGE_INTEGER value;

		wsprintf(buffer, L"NetStatsInHigh%i", i + 1);
		value.HighPart = (DWORD)GetPrivateProfileInt(L"Statistics", buffer, 0, iniFile.c_str());

		wsprintf(buffer, L"NetStatsInLow%i", i + 1);
		value.LowPart = (DWORD)GetPrivateProfileInt(L"Statistics", buffer, 0, iniFile.c_str());

		c_StatValues.push_back(value.QuadPart);

		wsprintf(buffer, L"NetStatsOutHigh%i", i + 1);
		value.HighPart = (DWORD)GetPrivateProfileInt(L"Statistics", buffer, 0, iniFile.c_str());

		wsprintf(buffer, L"NetStatsOutLow%i", i + 1);
		value.LowPart = (DWORD)GetPrivateProfileInt(L"Statistics", buffer, 0, iniFile.c_str());

		c_StatValues.push_back(value.QuadPart);
	}
}

/*
** WriteStats
**
** Writes statistics.
**
*/
void CMeasureNet::WriteStats(const std::wstring& iniFile)
{
	WCHAR buffer[32];
	WCHAR buffer2[64];
	
	wsprintf(buffer, L"%i", c_StatValues.size() / 2);
	WritePrivateProfileString(L"Statistics", L"NetStatsCount", buffer, iniFile.c_str());

	for (size_t i = 0; i < c_StatValues.size() / 2; ++i)
	{
		ULARGE_INTEGER value;

		value.QuadPart = c_StatValues[i * 2];

		wsprintf(buffer2, L"NetStatsInHigh%i", i + 1);
		wsprintf(buffer, L"%u", value.HighPart);
		WritePrivateProfileString(L"Statistics", buffer2, buffer, iniFile.c_str());

		wsprintf(buffer2, L"NetStatsInLow%i", i + 1);
		wsprintf(buffer, L"%u", value.LowPart);
		WritePrivateProfileString(L"Statistics", buffer2, buffer, iniFile.c_str());

		value.QuadPart = c_StatValues[i * 2 + 1];

		wsprintf(buffer2, L"NetStatsOutHigh%i", i + 1);
		wsprintf(buffer, L"%u", value.HighPart);
		WritePrivateProfileString(L"Statistics", buffer2, buffer, iniFile.c_str());

		wsprintf(buffer2, L"NetStatsOutLow%i", i + 1);
		wsprintf(buffer, L"%u", value.LowPart);
		WritePrivateProfileString(L"Statistics", buffer2, buffer, iniFile.c_str());
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
	if (c_IpHlpApiLibrary == NULL)
	{
		c_IpHlpApiLibrary = GetModuleHandle(L"IpHlpApi.dll");
		if (c_IpHlpApiLibrary)
		{
			c_GetIfTable2Ex = (FPGETIFTABLE2EX)GetProcAddress(c_IpHlpApiLibrary, "GetIfTable2Ex");
			c_FreeMibTable = (FPFREEMIBTABLE)GetProcAddress(c_IpHlpApiLibrary, "FreeMibTable");
		}

		c_UseNewApi = (c_IpHlpApiLibrary && c_GetIfTable2Ex && c_FreeMibTable);

		if (!c_UseNewApi)
		{
			if (c_IpHlpApiLibrary)
			{
				c_IpHlpApiLibrary = NULL;
			}
			c_GetIfTable2Ex = NULL;
			c_FreeMibTable = NULL;
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
	if (c_UseNewApi)
	{
		c_FreeMibTable(c_Table);

		c_IpHlpApiLibrary = NULL;
		c_GetIfTable2Ex = NULL;
		c_FreeMibTable = NULL;
	}
	else
	{
		delete [] c_Table;
	}
	c_Table = NULL;
	c_NumOfTables = 0;

	c_UseNewApi = false;
}
