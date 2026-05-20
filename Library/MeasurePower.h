/* Copyright (C) 2002 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MEASUREPOWER_H__
#define __MEASUREPOWER_H__

#include "Measure.h"

class MeasurePower : public Measure
{
public:
	MeasurePower(Skin* skin, const WCHAR* name);
	virtual ~MeasurePower();

	MeasurePower(const MeasurePower& other) = delete;
	MeasurePower& operator=(MeasurePower other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasurePower>(); }
	virtual const WCHAR* GetStringValue();

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	enum MeasureType
	{
		POWER_UNKNOWN,
		POWER_ACLINE,
		POWER_STATUS,
		POWER_STATUS2,
		POWER_LIFETIME,
		POWER_PERCENT,
		POWER_MHZ,
		POWER_HZ
	};

	MeasureType m_Type;
	std::wstring m_Format;
	bool m_SuppressError;
	bool m_Updated;
	DWORD m_CachedBatteryLifeTime;
};

#endif
