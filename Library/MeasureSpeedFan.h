/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASURESPEEDFAN_H_
#define RM_LIBRARY_MEASURESPEEDFAN_H_

#include "Measure.h"

class MeasureSpeedFan : public Measure
{
public:
	MeasureSpeedFan(Skin* skin, const WCHAR* name);
	virtual ~MeasureSpeedFan();

	MeasureSpeedFan(const MeasureSpeedFan& other) = delete;
	MeasureSpeedFan& operator=(MeasureSpeedFan other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureSpeedFan>(); }

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	enum class SensorType
	{
		Temperature,
		Fan,
		Voltage
	};

	enum class ScaleType
	{
		Source,
		Celsius,
		Fahrenheit,
		Kelvin
	};

	void ReadSharedData(double* value);

	SensorType m_Type;
	ScaleType m_Scale;
	UINT m_Number;
};

#endif
