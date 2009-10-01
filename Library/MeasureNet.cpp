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

#pragma warning(disable: 4996)

#include "MeasureNet.h"
#include "Rainmeter.h"

MIB_IFTABLE* CMeasureNet::c_Table = NULL;
UINT CMeasureNet::c_NumOfTables = 0;
std::vector<LARGE_INTEGER> CMeasureNet::c_StatValues;
std::vector<DWORD> CMeasureNet::c_OldStatValues;

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
	delete [] c_Table;
	c_Table = NULL;
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
	if(c_Table == NULL)
	{
		// Gotta reserve few bytes for the tables
		DWORD value;
		if(GetNumberOfInterfaces(&value) == NO_ERROR)
		{
			c_NumOfTables = value;

			if(c_NumOfTables > 0)
			{
				DWORD size = sizeof(MIB_IFTABLE) + sizeof(MIB_IFROW) * c_NumOfTables;
				c_Table = (MIB_IFTABLE*)new char[size];
			}
		}
	}

	if(c_Table)
	{
		DWORD size = sizeof(MIB_IFTABLE) + sizeof(MIB_IFROW) * c_NumOfTables;
		if(GetIfTable(c_Table, &size, false) != NO_ERROR)
		{
			delete [] c_Table;
			c_Table = (MIB_IFTABLE*)new char[size];
			if(GetIfTable(c_Table, &size, false) != NO_ERROR)
			{
				// Something's wrong. Unable to get the table.
				delete [] c_Table;
				c_Table = NULL;
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
DWORD CMeasureNet::GetNetOctets(NET net)
{
	DWORD value = 0;

	if (m_Interface == 0)
	{
		// Get all interfaces
		for(UINT i = 0; i < c_NumOfTables; i++)
		{
			// Ignore the loopback
			if(strcmp((char*)c_Table->table[i].bDescr, "MS TCP Loopback interface") != 0)
			{
				switch (net)
				{
				case NET_IN:
					value += c_Table->table[i].dwInOctets;
					break;

				case NET_OUT:
					value += c_Table->table[i].dwOutOctets;
					break;

				case NET_TOTAL:
					value += c_Table->table[i].dwInOctets;
					value += c_Table->table[i].dwOutOctets;
					break;
				}
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
				value += c_Table->table[m_Interface - 1].dwInOctets;
				break;

			case NET_OUT:
				value += c_Table->table[m_Interface - 1].dwOutOctets;
				break;

			case NET_TOTAL:
				value += c_Table->table[m_Interface - 1].dwInOctets;
				value += c_Table->table[m_Interface - 1].dwOutOctets;
				break;
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
LARGE_INTEGER CMeasureNet::GetNetStatsValue(NET net)
{
	LARGE_INTEGER value;
	value.QuadPart = 0;

	if (m_Interface == 0)
	{
		// Get all interfaces
		for(size_t i = 0; i < c_StatValues.size() / 2; i++)
		{
			switch (net)
			{
			case NET_IN:
				value.QuadPart += c_StatValues[i * 2 + 0].QuadPart;
				break;

			case NET_OUT:
				value.QuadPart += c_StatValues[i * 2 + 1].QuadPart;
				break;

			case NET_TOTAL:
				value.QuadPart += c_StatValues[i * 2 + 0].QuadPart;
				value.QuadPart += c_StatValues[i * 2 + 1].QuadPart;
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
				value.QuadPart += c_StatValues[m_Interface * 2 + 0].QuadPart;
				break;

			case NET_OUT:
				value.QuadPart += c_StatValues[m_Interface * 2 + 1].QuadPart;
				break;

			case NET_TOTAL:
				value.QuadPart += c_StatValues[m_Interface * 2 + 0].QuadPart;
				value.QuadPart += c_StatValues[m_Interface * 2 + 1].QuadPart;
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
		while (c_StatValues.size() < c_Table->dwNumEntries * 2)
		{
			LARGE_INTEGER value;
			value.QuadPart = 0;
			c_StatValues.push_back(value);
		}

		while (c_OldStatValues.size() < c_Table->dwNumEntries * 2)
		{ 
			c_OldStatValues.push_back(0);
		}

		for (UINT i = 0; i < c_Table->dwNumEntries; i++)
		{
			DWORD in, out;

			in = c_Table->table[i].dwInOctets;
			out = c_Table->table[i].dwOutOctets;

			if (c_OldStatValues[i * 2 + 0] != 0 && c_OldStatValues[i * 2 + 1] != 0)
			{
				if (in > c_OldStatValues[i * 2 + 0])
				{
					c_StatValues[i * 2 + 0].QuadPart += in - c_OldStatValues[i * 2 + 0];
				}
				if (out > c_OldStatValues[i * 2 + 1])
				{
					c_StatValues[i * 2 + 1].QuadPart += out - c_OldStatValues[i * 2 + 1];
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
		for (size_t i = 0; i < c_StatValues.size(); i++)
		{
			LARGE_INTEGER value;
			value.QuadPart = 0;
			c_StatValues[i] = value;
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
	WCHAR buffer[256];
	int count = GetPrivateProfileInt(L"Statistics", L"NetStatsCount", 0, iniFile.c_str());

	c_StatValues.clear();

	for (int i = 0; i < count; i++)
	{
		LARGE_INTEGER value;

		wsprintf(buffer, L"NetStatsInHigh%i", i + 1);
		value.HighPart = (DWORD)GetPrivateProfileInt(L"Statistics", buffer, 0, iniFile.c_str());
		wsprintf(buffer, L"NetStatsInLow%i", i + 1);
		value.LowPart = (DWORD)GetPrivateProfileInt(L"Statistics", buffer, 0, iniFile.c_str());
		c_StatValues.push_back(value);

		wsprintf(buffer, L"NetStatsOutHigh%i", i + 1);
		value.HighPart = (DWORD)GetPrivateProfileInt(L"Statistics", buffer, 0, iniFile.c_str());
		wsprintf(buffer, L"NetStatsOutLow%i", i + 1);
		value.LowPart = (DWORD)GetPrivateProfileInt(L"Statistics", buffer, 0, iniFile.c_str());
		c_StatValues.push_back(value);
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
	WCHAR buffer[256];
	WCHAR buffer2[256];
	
	wsprintf(buffer, L"%i", c_StatValues.size() / 2);
	WritePrivateProfileString(L"Statistics", L"NetStatsCount", buffer, iniFile.c_str());

	for (size_t i = 0; i < c_StatValues.size() / 2; i++)
	{
		wsprintf(buffer2, L"NetStatsInHigh%i", i + 1);
		wsprintf(buffer, L"%u", c_StatValues[i * 2].HighPart);
		WritePrivateProfileString(L"Statistics", buffer2, buffer, iniFile.c_str());

		wsprintf(buffer2, L"NetStatsInLow%i", i + 1);
		wsprintf(buffer, L"%u", c_StatValues[i * 2].LowPart);
		WritePrivateProfileString(L"Statistics", buffer2, buffer, iniFile.c_str());

		wsprintf(buffer2, L"NetStatsOutHigh%i", i + 1);
		wsprintf(buffer, L"%u", c_StatValues[i * 2 + 1].HighPart);
		WritePrivateProfileString(L"Statistics", buffer2, buffer, iniFile.c_str());

		wsprintf(buffer2, L"NetStatsOutLow%i", i + 1);
		wsprintf(buffer, L"%u", c_StatValues[i * 2 + 1].LowPart);
		WritePrivateProfileString(L"Statistics", buffer2, buffer, iniFile.c_str());
	}
}

