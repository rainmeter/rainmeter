/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREPOWER_H_
#define RM_LIBRARY_MEASUREPOWER_H_

#include "Measure.h"

class MeasurePower : public Measure
{
public:
	MeasurePower(Skin* skin, const WCHAR* name);
	virtual ~MeasurePower();

	MeasurePower(const MeasurePower& other) = delete;
	MeasurePower& operator=(MeasurePower other) = delete;

	UINT GetTypeID() override { return TypeID<MeasurePower>(); }

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;
	const WCHAR* GetStringValue() override;

private:
	enum class PowerState
	{
		UNKNOWN,
		ACLINE,
		STATUS,
		STATUS2,
		LIFETIME,
		PERCENT,
		MHZ,
		HZ
	};

	void LogProcessorPowerError(LONG status);
	void LogPowerStatusError();

	static UINT c_NumOfProcessors;

	PowerState m_State;
	std::wstring m_Format;
	bool m_SuppressError;
	bool m_HasBeenUpdated;
	DWORD m_CachedBatteryLifeTime;
	WCHAR m_StringValue[128];
};

#endif
