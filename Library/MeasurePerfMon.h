// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

class MeasurePerfMon : public Measure
{
public:
	MeasurePerfMon(Skin* skin, const WCHAR* name);
	virtual ~MeasurePerfMon();

	MeasurePerfMon(const MeasurePerfMon& other) = delete;
	MeasurePerfMon& operator=(MeasurePerfMon other) = delete;

	UINT GetTypeID() override { return TypeID<MeasurePerfMon>(); }

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	ULONGLONG GetPerfData(const WCHAR* objectName, const WCHAR* instanceName, const WCHAR* counterName);

	std::wstring m_ObjectName;
	std::wstring m_CounterName;
	std::wstring m_InstanceName;
	ULONGLONG m_OldValue;
	bool m_Difference;
	bool m_FirstTime;
};
