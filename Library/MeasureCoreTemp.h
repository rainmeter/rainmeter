// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

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
};
