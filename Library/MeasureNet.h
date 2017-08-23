/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MEASURENET_H__
#define __MEASURENET_H__

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Iphlpapi.h>
#include "Measure.h"

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

	MeasureNet(Skin* skin, const WCHAR* name, NET type);
	virtual ~MeasureNet();

	MeasureNet(const MeasureNet& other) = delete;
	MeasureNet& operator=(MeasureNet other) = delete;

	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	ULONG64 GetNetOctets(NET net);
	ULONG64 GetNetStatsValue(NET net);
	UINT GetBestInterfaceOrByName(const WCHAR* iface);

	NET m_Net;
	UINT m_Interface;

	ULONG64 m_Octets;
	bool m_FirstTime;
	bool m_Cumulative;
	bool m_UseBits;

	static std::vector<ULONG64> c_OldStatValues;
	static std::vector<ULONG64> c_StatValues;
	static MIB_IF_TABLE2* c_Table;
	static UINT c_NumOfTables;
};

#endif
