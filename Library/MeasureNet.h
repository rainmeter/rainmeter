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

#ifndef __MEASURENET_H__
#define __MEASURENET_H__

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Iphlpapi.h>
#include "Measure.h"

typedef NETIO_STATUS (NETIOAPI_API_ * FPGETIFTABLE2)(PMIB_IF_TABLE2* Table);
typedef VOID (NETIOAPI_API_ * FPFREEMIBTABLE)(PVOID Memory);

class CMeasureNet : public CMeasure
{
public:
	enum NET
	{
		NET_IN,
		NET_OUT,
		NET_TOTAL
	};

	CMeasureNet(CMeterWindow* meterWindow, const WCHAR* name);
	virtual ~CMeasureNet();
	
	virtual bool Update();

	static void UpdateIFTable();

	static void UpdateStats();
	static void ResetStats();
	static void ReadStats(const WCHAR* iniFile, std::wstring& statsDate);
	static void WriteStats(const WCHAR* iniFile, const std::wstring& statsDate);

	static void InitializeNewApi();
	static void FinalizeNewApi();

protected:
	void ReadConfig(CConfigParser& parser, const WCHAR* section, CMeasureNet::NET net);
	ULONG64 GetNetOctets(NET net);
	ULONG64 GetNetStatsValue(NET net);

	double m_CurrentTraffic;
	double m_TrafficValue;
	UINT m_Interface;
	bool m_Cumulative;
	std::wstring m_TrafficAction;

	static std::vector<ULONG64> c_OldStatValues;
	static std::vector<ULONG64> c_StatValues;
	static BYTE* c_Table;
	static UINT c_NumOfTables;

	static FPGETIFTABLE2 c_GetIfTable2;
	static FPFREEMIBTABLE c_FreeMibTable;
};

#endif
