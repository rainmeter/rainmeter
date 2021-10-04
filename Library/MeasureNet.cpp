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
#include "../Common/NetworkUtil.h"

std::vector<ULONG64> MeasureNet::c_StatValues;
std::vector<ULONG64> MeasureNet::c_OldStatValues;

MeasureNet::MeasureNet(Skin* skin, const WCHAR* name, NET type) : Measure(skin, name),
	m_Net(type),
	m_Interface(0U),
	m_Octets(0Ui64),
	m_FirstTime(true),
	m_Cumulative(false),
	m_UseBits(false)
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
	const ULONG oldCount = NetworkUtil::GetInterfaceCount();

	if (!NetworkUtil::UpdateInterfaceTable()) return;

	MIB_IF_ROW2* table = NetworkUtil::GetInterfaceTable();
	const ULONG newCount = NetworkUtil::GetInterfaceCount();
	if (table && GetRainmeter().GetDebug() && oldCount != newCount)
	{
		LogDebug(L"------------------------------");
		LogDebugF(L"* NETWORK-INTERFACE: Count=%i", newCount);

		for (size_t i = 0ULL; i < newCount; ++i)
		{
			LPCWSTR type = NetworkUtil::GetInterfaceTypeString(table[i].Type);
			LPCWSTR state = NetworkUtil::GetInterfaceMediaConnectionString(table[i].MediaConnectState);
			LPCWSTR status = NetworkUtil::GetInterfaceOperStatusString(table[i].OperStatus);

			LogDebugF(L"%3i: Name: %s", (int)i + 1, table[i].Description);
			LogDebugF(L"     Alias: %s", table[i].Alias);

			WCHAR guid[64] = { 0 };
			if (StringFromGUID2(table[i].InterfaceGuid, guid, 64) > 0)
			{
				LogDebugF(L"     GUID: %s", guid);
			}

			LogDebugF(L"     Type=%s(%i), Hardware=%s, Filter=%s",
				type, table[i].Type,
				(table[i].InterfaceAndOperStatusFlags.HardwareInterface == 1) ? L"Yes" : L"No",
				(table[i].InterfaceAndOperStatusFlags.FilterInterface == 1) ? L"Yes" : L"No");
			LogDebugF(L"     IfIndex=%i, State=%s, Status=%s(%i)",
				table[i].InterfaceIndex,
				state,
				status, table[i].OperStatus);
		}
		LogDebug(L"------------------------------");
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
	MIB_IF_ROW2* table = NetworkUtil::GetInterfaceTable();
	if (!table) return value;

	const ULONG interfaceCount = NetworkUtil::GetInterfaceCount();
	if (m_Interface == 0)
	{
		// Get all interfaces
		for (ULONG i = 0UL; i < interfaceCount; ++i)
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
		if (m_Interface <= interfaceCount)
		{
			ULONG index = NetworkUtil::GetIndexFromIfIndex(m_Interface);
			switch (net)
			{
			case NET_IN:
				value += table[index].InOctets;
				break;

			case NET_OUT:
				value += table[index].OutOctets;
				break;

			case NET_TOTAL:
				value += table[index].InOctets;
				value += table[index].OutOctets;
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

	MIB_IF_ROW2* table = NetworkUtil::GetInterfaceTable();
	if (!table) return value;

	const ULONG interfaceCount = NetworkUtil::GetInterfaceCount();
	if (m_Interface == 0UL)
	{
		// Get all interfaces
		for (size_t i = 0ULL; i < statsSize; ++i)
		{
			// Ignore the loopback and filter interfaces
			if (interfaceCount == statsSize)
			{
				if (table[i].Type == IF_TYPE_SOFTWARE_LOOPBACK ||
					table[i].InterfaceAndOperStatusFlags.FilterInterface == 1) continue;
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
			ULONG index = NetworkUtil::GetIndexFromIfIndex(m_Interface);
			switch (net)
			{
			case NET_IN:
				value += c_StatValues[index * 2 + 0];
				break;

			case NET_OUT:
				value += c_StatValues[index * 2 + 1];
				break;

			case NET_TOTAL:
				value += c_StatValues[index * 2 + 0];
				value += c_StatValues[index * 2 + 1];
				break;
			}
		}
	}

	return value;
}

void MeasureNet::UpdateValue()
{
	if (!NetworkUtil::GetInterfaceTable()) return;

	const ULONG64 bits = m_UseBits ? 8ULL : 1ULL;

	if (m_Cumulative)
	{
		m_Value = (double)(__int64)(GetNetStatsValue(m_Net) * bits);
	}
	else
	{
		ULONG64 value = 0ULL;

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
				value = 0ULL;
			}
		}
		else
		{
			m_Octets = GetNetOctets(m_Net);
			m_FirstTime = false;
		}

		m_Value = (double)(__int64)(value * bits);
	}
}

void MeasureNet::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	double value = 0.0;
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

	double maxValue = parser.ReadFloat(section, L"MaxValue", -1.0);
	if (maxValue == -1.0)
	{
		maxValue = parser.ReadFloat(section, netName, -1.0);
		if (maxValue == -1.0)
		{
			maxValue = value;
		}
	}

	// Option 'Interface' represents either the number of the interface in the 'iftable',
	// or the name of the interface (ie. its Description). Optionally, if 'Interface=Best',
	// there will be an attempt to find the best interface.
	std::wstring iface = parser.ReadString(section, L"Interface", L"BEST");
	if (!iface.empty() && !std::all_of(iface.begin(), iface.end(), iswdigit))
	{
		m_Interface = NetworkUtil::FindBestInterface(iface.c_str());
		if (GetRainmeter().GetDebug())
		{
			MIB_IF_ROW2* table = NetworkUtil::GetInterfaceTable();
			for (size_t i = 0ULL; i < NetworkUtil::GetInterfaceCount(); ++i)
			{
				if (table[i].InterfaceIndex == m_Interface)
				{
					LogDebugF(this, L"Using network interface: %s (IfIndex=%i)", table[i].Description, m_Interface);
					break;
				}
			}
		}
	}
	else
	{
		m_Interface = parser.ReadUInt(section, L"Interface", 0U);
	}

	m_Cumulative = parser.ReadBool(section, L"Cumulative", false);
	if (m_Cumulative)
	{
		GetRainmeter().SetNetworkStatisticsTimer();
	}

	m_UseBits = parser.ReadBool(section, L"UseBits", false);

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
		m_MaxValue = maxValue / (m_UseBits ? 1.0 : 8.0);
		m_LogMaxValue = false;
	}
}

void MeasureNet::UpdateStats()
{
	MIB_IF_ROW2* table = NetworkUtil::GetInterfaceTable();
	if (!table) return;

	const ULONG interfaceCount = NetworkUtil::GetInterfaceCount();
	size_t statsSize = interfaceCount * 2;

	// Fill the vectors
	if (c_StatValues.size() < statsSize)
	{
		c_StatValues.resize(statsSize, 0ULL);
	}

	if (c_OldStatValues.size() < statsSize)
	{
		c_OldStatValues.resize(statsSize, 0ULL);
	}

	for (size_t i = 0ULL; i < interfaceCount; ++i)
	{
		ULONG64 in = table[i].InOctets;
		ULONG64 out = table[i].OutOctets;
		if (c_OldStatValues[i * 2 + 0] != 0ULL)
		{
			if (in > c_OldStatValues[i * 2 + 0])
			{
				c_StatValues[i * 2 + 0] += in - c_OldStatValues[i * 2 + 0];
			}
		}

		if (c_OldStatValues[i * 2 + 1] != 0ULL)
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

	uint32_t count = parser.ReadUInt(L"Statistics", L"Count", 0U);
	if (parser.GetLastDefaultUsed())
	{
		count = parser.ReadUInt(L"Statistics", L"NetStatsCount", 0U);
	}

	c_StatValues.clear();
	c_StatValues.reserve(count * 2);

	for (uint32_t i = 1U; i <= count; ++i)
	{
		ULARGE_INTEGER value = { 0 };

		_snwprintf_s(buffer, _TRUNCATE, L"In%u", i);
		value.QuadPart = parser.ReadUInt64(L"Statistics", buffer, 0Ui64);
		if (parser.GetLastDefaultUsed())
		{
			_snwprintf_s(buffer, _TRUNCATE, L"NetStatsInHigh%u", i);
			value.HighPart = parser.ReadUInt(L"Statistics", buffer, 0U);

			_snwprintf_s(buffer, _TRUNCATE, L"NetStatsInLow%u", i);
			value.LowPart = parser.ReadUInt(L"Statistics", buffer, 0U);
		}
		c_StatValues.push_back(value.QuadPart);

		_snwprintf_s(buffer, _TRUNCATE, L"Out%u", i);
		value.QuadPart = parser.ReadUInt64(L"Statistics", buffer, 0Ui64);
		if (parser.GetLastDefaultUsed())
		{
			_snwprintf_s(buffer, _TRUNCATE, L"NetStatsOutHigh%u", i);
			value.HighPart = parser.ReadUInt(L"Statistics", buffer, 0U);

			_snwprintf_s(buffer, _TRUNCATE, L"NetStatsOutLow%u", i);
			value.LowPart = parser.ReadUInt(L"Statistics", buffer, 0U);
		}
		c_StatValues.push_back(value.QuadPart);
	}
}

void MeasureNet::WriteStats(const WCHAR* iniFile, const std::wstring& statsDate)
{
	WCHAR buffer[48];
	int len = 0;

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
	for (uint32_t i = 0U; i < count; ++i)
	{
		if (c_StatValues[i * 2] > 0ULL)
		{
			len  = _snwprintf_s(buffer, _TRUNCATE, L"In%u=%llu", i + 1U, c_StatValues[i * 2]);
			appendStatsValue();
		}

		if (c_StatValues[i * 2 + 1] > 0ULL)
		{
			len  = _snwprintf_s(buffer, _TRUNCATE, L"Out%u=%llu", i + 1U, c_StatValues[i * 2 + 1]);
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
	NetworkUtil::Finalize();
}
