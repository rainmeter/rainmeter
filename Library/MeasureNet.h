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

#ifndef __MEASURENET_H__
#define __MEASURENET_H__

#include "Measure.h"
#include <Iphlpapi.h>

class CMeasureNet : public CMeasure
{
public:
	enum NET {
		NET_IN,
		NET_OUT,
		NET_TOTAL
	};

	CMeasureNet(CMeterWindow* meterWindow);
	virtual ~CMeasureNet();
	
	virtual bool Update();

	static void UpdateIFTable();

	static void UpdateStats();
	static void ResetStats();
	static void ReadStats(const std::wstring& iniFile);
	static void WriteStats(const std::wstring& iniFile);

protected:
	void ReadConfig(CConfigParser& parser, const WCHAR* section, CMeasureNet::NET net);
	DWORD GetNetOctets(NET net);
	LARGE_INTEGER GetNetStatsValue(NET net);

	double m_CurrentTraffic;
	double m_TrafficValue;
	UINT m_Interface;
	bool m_Cumulative;
	std::wstring m_TrafficAction;

	static std::vector<DWORD> c_OldStatValues;
	static std::vector<LARGE_INTEGER> c_StatValues;
	static MIB_IFTABLE* c_Table;
	static UINT c_NumOfTables;
};

#endif
