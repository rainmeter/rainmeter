/* Copyright (C) 2021 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MEASURESYSINFO_H__
#define __MEASURESYSINFO_H__

#include "Measure.h"
#include <Netlistmgr.h>

enum class SysInfoType : UINT
{
	UNKNOWN = 0U,

	// General system
	COMPUTER_NAME = 1000U,
	USER_NAME,
	USER_SID,
	OS_VERSION,
	OS_PRODUCT_NAME,
	PAGESIZE,
	OS_BITS,

	// Monitor and screen values
	NUM_MONITORS = 2000U,
	SCREEN_SIZE,
	WORK_AREA,

	SCREEN_WIDTH = 2500U,        // BLOCK 2500: Do not change the //
	SCREEN_HEIGHT,               // order of these types. They    //
	WORK_AREA_LEFT,              // used to gather monitor size   //
	WORK_AREA_TOP,               // information.                  //
	WORK_AREA_WIDTH,
	WORK_AREA_HEIGHT,
	VIRTUAL_SCREEN_LEFT,
	VIRTUAL_SCREEN_TOP,
	VIRTUAL_SCREEN_WIDTH,
	VIRTUAL_SCREEN_HEIGHT,

	// Network values
	HOST_NAME = 3000U,
	DOMAIN_NAME,
	DNS_SERVER,
	DOMAIN_WORKGROUP,

	INTERNET_CONNECTIVITY = 3250U, // BLOCK 3250: Do not change   //
	INTERNET_CONNECTIVITY_V4,      // the order of these types.   //
	INTERNET_CONNECTIVITY_V6,
	LAN_CONNECTIVITY,
	LAN_CONNECTIVITY_V4,
	LAN_CONNECTIVITY_V6,

	ADAPTER_DESCRIPTION = 3500U,   // BLOCK 3500: Do not change   //
	ADAPTER_ALIAS,                 // the order of these types.   //
	ADAPTER_GUID,                  // They are used with specific //
	ADAPTER_TYPE,                  // interface names or with the //
	ADAPTER_STATE,                 // SysInfoData=Best option.    //
	ADAPTER_STATUS,
	ADAPTER_TRANSMIT_SPEED,
	ADAPTER_RECEIVE_SPEED,
	MAC_ADDRESS,
	NET_MASK,
	IP_ADDRESS,
	GATEWAY_ADDRESS,
	GATEWAY_ADDRESS_V4,
	GATEWAY_ADDRESS_V6,

	// Timezone values
	TIMEZONE_ISDST = 4000U,
	TIMEZONE_BIAS,
	TIMEZONE_STANDARD_BIAS,
	TIMEZONE_DAYLIGHT_BIAS,
	TIMEZONE_STANDARD_NAME,
	TIMEZONE_DAYLIGHT_NAME,

	// Time values
	IDLE_TIME = 5000U,
	USER_LOGON_TIME,
	LAST_SLEEP_TIME,
	LAST_WAKE_TIME
};

class MeasureSysInfo : public Measure
{
public:
	MeasureSysInfo(Skin* skin, const WCHAR* name);
	virtual ~MeasureSysInfo();

	MeasureSysInfo(const MeasureSysInfo& other) = delete;
	MeasureSysInfo& operator=(MeasureSysInfo other) = delete;

	virtual const WCHAR* GetStringValue();

	virtual UINT GetTypeID() { return TypeID<MeasureSysInfo>(); }

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	NLM_CONNECTIVITY GetNetworkConnectivity();

	SysInfoType m_Type;
	int m_Data;

	std::wstring m_StringValue;

	bool m_SuppressError;
	bool m_HasBeenUpdated;

	static LONGLONG s_LogonTime;
};

#endif
