// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"
#include <pdh.h>

class MeasurePerfMon : public Measure
{
public:
	MeasurePerfMon(Skin* skin, const WCHAR* name);
	virtual ~MeasurePerfMon();

	MeasurePerfMon(const MeasurePerfMon& other) = delete;
	MeasurePerfMon& operator=(MeasurePerfMon other) = delete;

	UINT GetTypeID() override { return TypeID<MeasurePerfMon>(); }

protected:
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	void UpdateValue() override;

private:
	bool OpenQuery();
	void CloseQuery();
	bool GetRawValue(ULONGLONG& value);

	std::wstring m_ObjectName;
	std::wstring m_CounterName;
	std::wstring m_InstanceName;

	PDH_HQUERY m_Query;
	PDH_HCOUNTER m_Counter;

	// Set when the object turned out to have instances but none was named, in which case the first
	// one is read, as it always has been
	bool m_FirstInstance;

	// Only needed to read the first instance, and kept between updates so that doing so does not
	// have to go back to the heap every time
	std::vector<BYTE> m_Buffer;

	ULONGLONG m_OldValue;
	bool m_Difference;
	bool m_FirstTime;
};
