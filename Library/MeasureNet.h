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

class MeasureNet : public Measure
{
public:
	virtual UINT GetTypeID() { return TypeID<MeasureNet>(); }

	static void UpdateIFTable();

	static void UpdateStats();
	static void ResetStats();
	static void ReadStats(const std::wstring& iniFile, std::wstring& statsDate);
	static void WriteStats(const WCHAR* iniFile, const std::wstring& statsDate);

	static void InitializeStatic();
	static void FinalizeStatic();

protected:
	enum NET
	{
		NET_IN,
		NET_OUT,
		NET_TOTAL
	};

	MeasureNet(MeterWindow* meterWindow, const WCHAR* name, NET type);
	virtual ~MeasureNet();

	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	ULONG64 GetNetOctets(NET net);
	ULONG64 GetNetStatsValue(NET net);

	NET m_Net;
	UINT m_Interface;

	ULONG64 m_Octets;
	bool m_FirstTime;
	bool m_Cumulative;

	static std::vector<ULONG64> c_OldStatValues;
	static std::vector<ULONG64> c_StatValues;
	static BYTE* c_Table;
	static UINT c_NumOfTables;

	static FPGETIFTABLE2 c_GetIfTable2;
	static FPFREEMIBTABLE c_FreeMibTable;
};

#endif
