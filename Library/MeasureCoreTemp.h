/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASURECORETEMP_H_
#define RM_LIBRARY_MEASURECORETEMP_H_

#include "Measure.h"

class MeasureCoreTemp : public Measure
{
public:
	MeasureCoreTemp(Skin* skin, const WCHAR* name);
	virtual ~MeasureCoreTemp();

	MeasureCoreTemp(const MeasureCoreTemp& other) = delete;
	MeasureCoreTemp& operator=(MeasureCoreTemp other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureCoreTemp>(); }

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;
	const WCHAR* GetStringValue() override;

private:
	enum class Type
	{
		Temperature,
		MaxTemperature,
		TjMax,
		Load,
		Vid,
		CpuSpeed,
		BusSpeed,
		BusMultiplier,
		CpuName,
		CoreSpeed,
		CoreBusMultiplier,
		Tdp,
		Power
	};

	Type ConvertType(const WCHAR* type);
	float GetHighestTemp() const;

	Type m_Type;
	int m_Index;
	WCHAR m_StringValue[128];
};

#endif
