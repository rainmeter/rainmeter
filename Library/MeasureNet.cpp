/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureNet.h"
#include "Rainmeter.h"
#include "System.h"

MIB_IF_TABLE2* MeasureNet::c_Table = nullptr;
UINT MeasureNet::c_NumOfTables = 0;
std::vector<ULONG64> MeasureNet::c_StatValues;
std::vector<ULONG64> MeasureNet::c_OldStatValues;

MeasureNet::MeasureNet(Skin* skin, const WCHAR* name, NET type) : Measure(skin, name),
	m_Net(type),
	m_Interface(),
	m_Octets(),
	m_FirstTime(true),
	m_Cumulative(false)
{
}

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

	if (c_Table)
	{
		FreeMibTable(c_Table);
		c_Table = nullptr;
	}

	if (GetIfTable2(&c_Table) == NO_ERROR)
	{
		if (c_NumOfTables != c_Table->NumEntries)
		{
			c_NumOfTables = c_Table->NumEntries;
			logging = true;
		}

		if (GetRainmeter().GetDebug() && logging)
		{
			LogDebug(L"------------------------------");
			LogDebugF(L"* NETWORK-INTERFACE: Count=%i", c_NumOfTables);

			for (size_t i = 0; i < c_NumOfTables; ++i)
			{
				const WCHAR* type = L"Other";
				switch (c_Table->Table[i].Type)
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

				LogDebugF(L"%i: %s", (int)i + 1, c_Table->Table[i].Description);
				LogDebugF(L"  Alias: %s", c_Table->Table[i].Alias);
				LogDebugF(L"  Type=%s(%i), Hardware=%s, Filter=%s",
					type, c_Table->Table[i].Type,
					(c_Table->Table[i].InterfaceAndOperStatusFlags.HardwareInterface == 1) ? L"Yes" : L"No",
					(c_Table->Table[i].InterfaceAndOperStatusFlags.FilterInterface == 1) ? L"Yes" : L"No");
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

/*
** Reads the amount of octets. This is the same for in, out and total.
** the net-parameter informs which inherited class called this method.
**
*/
ULONG64 MeasureNet::GetNetOctets(NET net)
{
	ULONG64 value = 0;
	MIB_IF_ROW2* table = (MIB_IF_ROW2*)c_Table->Table;
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
				if (c_Table->Table[i].Type == IF_TYPE_SOFTWARE_LOOPBACK ||
					c_Table->Table[i].InterfaceAndOperStatusFlags.FilterInterface == 1) continue;
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

	// Option 'Interface' represents either the number of the interface in the 'iftable',
	// or the name of the interface (ie. its Description). Optionally, if 'Interface=Best',
	// there will be an attempt to find the best interface.
	std::wstring iface = parser.ReadString(section, L"Interface", L"");
	if (!iface.empty() && !std::all_of(iface.begin(), iface.end(), iswdigit))
	{
		m_Interface = GetBestInterfaceOrByName(iface.c_str());
	}
	else
	{
		m_Interface = parser.ReadInt(section, L"Interface", 0);
	}

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

UINT MeasureNet::GetBestInterfaceOrByName(const WCHAR* iface)
{
	if (c_Table == nullptr) return 0;

	if (_wcsicmp(iface, L"BEST") == 0)
	{
		DWORD dwBestIndex;
		if (NO_ERROR == GetBestInterface(INADDR_ANY, &dwBestIndex))
		{
			MIB_IF_ROW2* table = (MIB_IF_ROW2*)c_Table->Table;
			for (size_t i = 0; i < c_NumOfTables; ++i)
			{
				if (table[i].InterfaceIndex == (NET_IFINDEX)dwBestIndex)
				{
					if (GetRainmeter().GetDebug())
					{
						LogDebugF(this, L"Using network interface: Number=(%i), Name=\"%s\"", i + 1, table[i].Description);
					}

					return (UINT)(i + 1);
				}
			}
		}
	}
	else
	{
		MIB_IF_ROW2* table = (MIB_IF_ROW2*)c_Table->Table;
		for (size_t i = 0; i < c_NumOfTables; ++i)
		{
			if (_wcsicmp(iface, table[i].Description) == 0)
			{
				return (UINT)(i + 1);
			}
		}
	}

	LogErrorF(this, L"Cannot find interface: \"%s\"", iface);
	return 0;
}

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
			ULONG64 in = c_Table->Table[i].InOctets;
			ULONG64 out = c_Table->Table[i].OutOctets;
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

void MeasureNet::ResetStats()
{
	c_StatValues.clear();
}

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

void MeasureNet::WriteStats(const WCHAR* iniFile, const std::wstring& statsDate)
{
	WCHAR buffer[48];
	int len;

	uint32_t count = (uint32_t)c_StatValues.size() / 2;

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

void MeasureNet::InitializeStatic()
{
	if (GetRainmeter().GetDebug())
	{
		UpdateIFTable();
	}
}

void MeasureNet::FinalizeStatic()
{
	if (c_Table)
	{
		FreeMibTable(c_Table);
	}
	c_Table = nullptr;
	c_NumOfTables = 0;
}
