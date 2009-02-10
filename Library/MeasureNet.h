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
/*
  $Header: /home/cvsroot/Rainmeter/Library/MeasureNet.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeasureNet.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.8  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.7  2003/02/10 18:13:34  rainy
  Changed the way stats are gathered.

  Revision 1.6  2002/12/23 14:26:07  rainy
  Added cumulative statistics measuring.

  Revision 1.5  2002/07/01 15:34:23  rainy
  Now it is possible to select the NIC.

  Revision 1.4  2002/04/26 18:24:16  rainy
  Modified the Update method to support disabled measures.

  Revision 1.3  2002/04/01 15:38:25  rainy
  Some on the implementation from NetIn/Out is moved here.
  Added TrafficAction and TrafficValue.

  Revision 1.2  2001/08/12 15:45:34  Rainy
  Changed UpdateTable() to be a class method.

  Revision 1.1.1.1  2001/08/11 10:58:19  Rainy
  Added to CVS.

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
