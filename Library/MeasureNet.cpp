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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "MeasureNet.h"
#include "Rainmeter.h"
#include "System.h"

BYTE* MeasureNet::c_Table = nullptr;
UINT MeasureNet::c_NumOfTables = 0;
std::vector<ULONG64> MeasureNet::c_StatValues;
std::vector<ULONG64> MeasureNet::c_OldStatValues;

FPGETIFTABLE2 MeasureNet::c_GetIfTable2 = nullptr;
FPFREEMIBTABLE MeasureNet::c_FreeMibTable = nullptr;

/*
** The constructor. This is the base class for the net-meters.
**
*/
MeasureNet::MeasureNet(MeterWindow* meterWindow, const WCHAR* name, NET type) : Measure(meterWindow, name),
	m_Net(type),
	m_Interface(),
	m_Octets(),
	m_FirstTime(true),
	m_Cumulative(false)
{
}

/*
** The destructor
**
*/
MeasureNet::~MeasureNet()
{
}

/*
** Reads the tables for all net interfaces
**
*/
void MeasureNet::UpdateIFTable()
{
	bool logging = false;

	if (c_GetIfTable2)
	{
		if (c_Table)
		{
			c_FreeMibTable(c_Table);
			c_Table = nullptr;
		}

		if (c_GetIfTable2((MIB_IF_TABLE2**)&c_Table) == NO_ERROR)
		{
			MIB_IF_TABLE2* ifTable = (MIB_IF_TABLE2*)c_Table;

			if (c_NumOfTables != ifTable->NumEntries)
			{
				c_NumOfTables = ifTable->NumEntries;
				logging = true;
			}

			if (GetRainmeter().GetDebug() && logging)
			{
				LogDebug(L"------------------------------");
				LogDebugF(L"* NETWORK-INTERFACE: Count=%i", c_NumOfTables);

				for (size_t i = 0; i < c_NumOfTables; ++i)
				{
					const WCHAR* type = L"Other";
					switch (ifTable->Table[i].Type)
					{
					case IF_TYPE_ETHERNET_CSMACD:
						type = L"Ethernet";
						break;
					case IF_TYPE_PPP:
						type = L"PPP";
						break;
					case IF_TYPE_SOFTWARE_LOOPBACK:
						type = L"Loopback";
						break;
					case IF_TYPE_IEEE80211:
						type = L"IEEE802.11";
						break;
					case IF_TYPE_TUNNEL:
						type = L"Tunnel";
						break;
					case IF_TYPE_IEEE1394:
						type = L"IEEE1394";
						break;
					}

					LogDebugF(L"%i: %s", (int)i + 1, ifTable->Table[i].Description);
					LogDebugF(L"  Alias: %s", ifTable->Table[i].Alias);
					LogDebugF(L"  Type=%s(%i), Hardware=%s, Filter=%s",
						type, ifTable->Table[i].Type,
						(ifTable->Table[i].InterfaceAndOperStatusFlags.HardwareInterface == 1) ? L"Yes" : L"No",
						(ifTable->Table[i].InterfaceAndOperStatusFlags.FilterInterface == 1) ? L"Yes" : L"No");
				}
				LogDebug(L"------------------------------");
			}
		}
		else
		{
			// Something's wrong. Unable to get the table.
			c_Table = nullptr;
			c_NumOfTables = 0;
		}
	}
	else
	{
		if (c_Table == nullptr)
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

				if (GetRainmeter().GetDebug() && logging)
				{
					LogDebug(L"------------------------------");
					LogDebugF(L"* NETWORK-INTERFACE: Count=%i", c_NumOfTables);

					for (size_t i = 0; i < c_NumOfTables; ++i)
					{
						const WCHAR* type = L"";
						switch (ifTable->table[i].dwType)
						{
						case IF_TYPE_ETHERNET_CSMACD:
							type = L"Ethernet";
							break;
						case IF_TYPE_PPP:
							type = L"PPP";
							break;
						case IF_TYPE_SOFTWARE_LOOPBACK:
							type = L"Loopback";
							break;
						case IF_TYPE_IEEE80211:
							type = L"IEEE802.11";
							break;
						case IF_TYPE_TUNNEL:
							type = L"Tunnel";
							break;
						case IF_TYPE_IEEE1394:
							type = L"IEEE1394";
							break;
						default:
							type = L"Other";
							break;
						}

						LogDebugF(L"%i: %.*S", (int)i + 1, ifTable->table[i].dwDescrLen, (char*)ifTable->table[i].bDescr);
						LogDebugF(L"  Type=%s(%i)", type, ifTable->table[i].dwType);
					}
					LogDebug(L"------------------------------");
				}
			}
			else
			{
				// Something's wrong. Unable to get the table.
				delete [] c_Table;
				c_Table = nullptr;
				c_NumOfTables = 0;
			}
		}
	}
}

/*
** Reads the amount of octets. This is the same for in, out and total.
** the net-parameter informs which inherited class called this method.
**
*/
ULONG64 MeasureNet::GetNetOctets(NET net)
{
	ULONG64 value = 0;

	if (c_GetIfTable2)
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
** Returns the stats value of the interface
**
*/
ULONG64 MeasureNet::GetNetStatsValue(NET net)
{
	ULONG64 value = 0;
	size_t statsSize = c_StatValues.size() / 2;

	if (m_Interface == 0)
	{
		// Get all interfaces
		for (size_t i = 0; i < statsSize; ++i)
		{
			// Ignore the loopback and filter interfaces
			if (c_NumOfTables == statsSize)
			{
				if (c_GetIfTable2)
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
** Updates the current value.
**
*/
void MeasureNet::UpdateValue()
{
	if (c_Table == nullptr) return;

	if (m_Cumulative)
	{
		m_Value = (double)(__int64)GetNetStatsValue(m_Net);
	}
	else
	{
		ULONG64 value = 0;

		if (!m_FirstTime)
		{
			value = GetNetOctets(m_Net);
			if (value > m_Octets)
			{
				ULONG64 tmpValue = value;
				value -= m_Octets;
				m_Octets = tmpValue;
			}
			else
			{
				m_Octets = value;
				value = 0;
			}
		}
		else
		{
			m_Octets = GetNetOctets(m_Net);
			m_FirstTime = false;
		}

		m_Value = (double)(__int64)value;
	}
}

/*
** Read the options specified in the ini file.
**
*/
void MeasureNet::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	double value;
	const WCHAR* netName = nullptr;

	if (m_Net == NET_IN)
	{
		netName = L"NetInSpeed";
		value = GetRainmeter().GetGlobalOptions().netInSpeed;
	}
	else if (m_Net == NET_OUT)
	{
		netName = L"NetOutSpeed";
		value = GetRainmeter().GetGlobalOptions().netOutSpeed;
	}
	else // if (m_Net == NET_TOTAL)
	{
		netName = L"NetTotalSpeed";
		value = GetRainmeter().GetGlobalOptions().netInSpeed + GetRainmeter().GetGlobalOptions().netOutSpeed;
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

	m_Cumulative = parser.ReadBool(section, L"Cumulative", false);
	if (m_Cumulative)
	{
		GetRainmeter().SetNetworkStatisticsTimer();
	}

	if (maxValue == 0.0)
	{
		if (!m_LogMaxValue)
		{
			m_MaxValue = 1.0;
			m_LogMaxValue = true;
			m_MedianValues.clear();
		}
	}
	else
	{
		m_MaxValue = maxValue / 8;
		m_LogMaxValue = false;
	}
}

/*
** Updates the statistics.
**
*/
void MeasureNet::UpdateStats()
{
	if (c_Table)
	{
		size_t statsSize = c_NumOfTables * 2;

		// Fill the vectors
		if (c_StatValues.size() < statsSize)
		{
			c_StatValues.resize(statsSize, 0);
		}

		if (c_OldStatValues.size() < statsSize)
		{
			c_OldStatValues.resize(statsSize, 0);
		}

		for (UINT i = 0; i < c_NumOfTables; ++i)
		{
			ULONG64 in, out;

			if (c_GetIfTable2)
			{
				in = ((MIB_IF_TABLE2*)c_Table)->Table[i].InOctets;
				out = ((MIB_IF_TABLE2*)c_Table)->Table[i].OutOctets;
			}
			else
			{
				in = ((MIB_IFTABLE*)c_Table)->table[i].dwInOctets;
				out = ((MIB_IFTABLE*)c_Table)->table[i].dwOutOctets;
			}

			if (c_OldStatValues[i * 2 + 0] != 0)
			{
				if (in > c_OldStatValues[i * 2 + 0])
				{
					c_StatValues[i * 2 + 0] += in - c_OldStatValues[i * 2 + 0];
				}
			}

			if (c_OldStatValues[i * 2 + 1] != 0)
			{
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
** Resets the statistics.
**
*/
void MeasureNet::ResetStats()
{
	c_StatValues.clear();
}

/*
** Reads statistics.
**
*/
void MeasureNet::ReadStats(const std::wstring& iniFile, std::wstring& statsDate)
{
	WCHAR buffer[48];

	ConfigParser parser;
	parser.Initialize(iniFile, nullptr, L"Statistics");

	const std::wstring& date = parser.ReadString(L"Statistics", L"Since", L"", false);
	if (!date.empty())
	{
		statsDate = date;
	}

	uint32_t count = parser.ReadUInt(L"Statistics", L"Count", 0);
	if (parser.GetLastDefaultUsed())
	{
		count = parser.ReadUInt(L"Statistics", L"NetStatsCount", 0);
	}

	c_StatValues.clear();
	c_StatValues.reserve(count * 2);

	for (uint32_t i = 1; i <= count; ++i)
	{
		ULARGE_INTEGER value;

		_snwprintf_s(buffer, _TRUNCATE, L"In%u", i);
		value.QuadPart = parser.ReadUInt64(L"Statistics", buffer, 0);
		if (parser.GetLastDefaultUsed())
		{
			_snwprintf_s(buffer, _TRUNCATE, L"NetStatsInHigh%u", i);
			value.HighPart = parser.ReadUInt(L"Statistics", buffer, 0);

			_snwprintf_s(buffer, _TRUNCATE, L"NetStatsInLow%u", i);
			value.LowPart = parser.ReadUInt(L"Statistics", buffer, 0);
		}
		c_StatValues.push_back(value.QuadPart);

		_snwprintf_s(buffer, _TRUNCATE, L"Out%u", i);
		value.QuadPart = parser.ReadUInt64(L"Statistics", buffer, 0);
		if (parser.GetLastDefaultUsed())
		{
			_snwprintf_s(buffer, _TRUNCATE, L"NetStatsOutHigh%u", i);
			value.HighPart = parser.ReadUInt(L"Statistics", buffer, 0);

			_snwprintf_s(buffer, _TRUNCATE, L"NetStatsOutLow%u", i);
			value.LowPart = parser.ReadUInt(L"Statistics", buffer, 0);
		}
		c_StatValues.push_back(value.QuadPart);
	}
}

/*
** Writes statistics.
**
*/
void MeasureNet::WriteStats(const WCHAR* iniFile, const std::wstring& statsDate)
{
	WCHAR buffer[48];
	int len;

	uint32_t count = c_StatValues.size() / 2;

	// Reserve sufficient buffer for statistics
	std::wstring data;
	data.reserve(48 * (2 + count));

	// Add date
	data = L"Since=";
	data += statsDate;
	data += L'\0';

	auto appendStatsValue = [&]()
	{
		data.append(buffer, len);
		data += L'\0';
	};

	// Add stats count
	len = _snwprintf_s(buffer, _TRUNCATE, L"Count=%u", count);
	appendStatsValue();

	// Add stats
	for (uint32_t i = 0; i < count; ++i)
	{
		if (c_StatValues[i * 2] > 0)
		{
			len  = _snwprintf_s(buffer, _TRUNCATE, L"In%u=%llu", i + 1, c_StatValues[i * 2]);
			appendStatsValue();
		}

		if (c_StatValues[i * 2 + 1] > 0)
		{
			len  = _snwprintf_s(buffer, _TRUNCATE, L"Out%u=%llu", i + 1, c_StatValues[i * 2 + 1]);
			appendStatsValue();
		}
	}

	// Write statistics
	WritePrivateProfileSection(L"Statistics", data.c_str(), iniFile);
}

/*
** Prepares in order to use the new APIs which are available on Vista or newer.
**
*/
void MeasureNet::InitializeStatic()
{
	if (Platform::IsAtLeastWinVista())
	{
		HMODULE IpHlpApiLibrary = GetModuleHandle(L"IpHlpApi.dll");
		if (IpHlpApiLibrary)
		{
			c_GetIfTable2 = (FPGETIFTABLE2)GetProcAddress(IpHlpApiLibrary, "GetIfTable2");
			c_FreeMibTable = (FPFREEMIBTABLE)GetProcAddress(IpHlpApiLibrary, "FreeMibTable");
		}

		if (!c_GetIfTable2 || !c_FreeMibTable)
		{
			c_GetIfTable2 = nullptr;
			c_FreeMibTable = nullptr;
		}
	}

	if (GetRainmeter().GetDebug())
	{
		UpdateIFTable();
	}
}

/*
** Frees the resources.
**
*/
void MeasureNet::FinalizeStatic()
{
	if (c_GetIfTable2)
	{
		if (c_Table)
		{
			c_FreeMibTable(c_Table);
		}

		c_GetIfTable2 = nullptr;
		c_FreeMibTable = nullptr;
	}
	else
	{
		delete [] c_Table;
	}
	c_Table = nullptr;
	c_NumOfTables = 0;
}
