// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

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
};
