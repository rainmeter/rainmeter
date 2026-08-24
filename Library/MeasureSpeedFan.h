// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

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
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
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
