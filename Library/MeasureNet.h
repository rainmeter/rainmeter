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

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Iphlpapi.h>
#include "Measure.h"

typedef NETIO_STATUS (NETIOAPI_API_ * FPGETIFENTRY2)(PMIB_IF_ROW2 Row);
typedef NETIO_STATUS (NETIOAPI_API_ * FPNOTIFYIPINTERFACECHANGE)(ADDRESS_FAMILY Family, PIPINTERFACE_CHANGE_CALLBACK Callback, PVOID CallerContext, BOOLEAN InitialNotification, HANDLE* NotificationHandle);
typedef NETIO_STATUS (NETIOAPI_API_ * FPCANCELMIBCHANGENOTIFY2)(HANDLE NotificationHandle);

class CMeasureNet : public CMeasure
{
public:
	enum NET {
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
	static void ReadStats(const WCHAR* iniFile);
	static void WriteStats(const WCHAR* iniFile);

	static void InitializeNewApi();
	static void FinalizeNewApi();

protected:
	void ReadConfig(CConfigParser& parser, const WCHAR* section, CMeasureNet::NET net);
	ULONG64 GetNetOctets(NET net);
	ULONG64 GetNetStatsValue(NET net);

	static void DisposeBuffer();
	static VOID NETIOAPI_API_ IpInterfaceChangeCallback(PVOID CallerContext, PMIB_IPINTERFACE_ROW Row, MIB_NOTIFICATION_TYPE NotificationType);

	double m_CurrentTraffic;
	double m_TrafficValue;
	UINT m_Interface;
	bool m_Cumulative;
	std::wstring m_TrafficAction;

	static std::vector<ULONG64> c_OldStatValues;
	static std::vector<ULONG64> c_StatValues;
	static BYTE* c_Table;
	static ULONG c_Size;
	static UINT c_NumOfTables;
	static BYTE* c_AATable;
	static ULONG c_AASize;

	static bool c_IpInterfaceChanged;
	static HANDLE c_NotificationHandle;

	static FPGETIFENTRY2 c_GetIfEntry2;
	static FPNOTIFYIPINTERFACECHANGE c_NotifyIpInterfaceChange;
	static FPCANCELMIBCHANGENOTIFY2 c_CancelMibChangeNotify2;
};

#endif
