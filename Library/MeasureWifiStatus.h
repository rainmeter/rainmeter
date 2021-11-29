/* Copyright (C) 2021 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREWIFISTATUS_H_
#define RM_LIBRARY_MEASUREWIFISTATUS_H_

#include "Measure.h"
#include <wlanapi.h>

class MeasureWifiStatus : public Measure
{
public:
	MeasureWifiStatus(Skin* skin, const WCHAR* name);
	virtual ~MeasureWifiStatus();

	MeasureWifiStatus(const MeasureWifiStatus& other) = delete;
	MeasureWifiStatus& operator=(MeasureWifiStatus other) = delete;

	virtual const WCHAR* GetStringValue();

	UINT GetTypeID() override { return TypeID<MeasureWifiStatus>(); }

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	enum class MeasureType : UINT
	{
		UNINITIALIZED = 0U,
		SSID = 100U,
		ENCRYPTION,
		AUTH,
		LIST,
		PHY,

		QUALITY = 200U,
		TXRATE,
		RXRATE,

		UNKNOWN = 9999U
	};

	void FinalizeHandle();

	const WCHAR* GetCipherAlgorithmString(DOT11_CIPHER_ALGORITHM value);
	const WCHAR* GetAuthAlgorithmString(DOT11_AUTH_ALGORITHM value);
	const WCHAR* GetInterfaceStateString(WLAN_INTERFACE_STATE value);
	const WCHAR* GetPHYString(DOT11_PHY_TYPE value);

	const WCHAR* GetErrorString(DWORD value);

	MeasureType m_Type;
	UINT m_ListStyle;
	UINT m_ListMax;
	std::wstring m_StatusString;

	static UINT s_Instances;
	static HANDLE s_Client;
	static PWLAN_INTERFACE_INFO s_Interface;
	static PWLAN_INTERFACE_INFO_LIST s_InterfaceList;
};

#endif
