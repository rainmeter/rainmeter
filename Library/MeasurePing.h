// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"
#include <string>

class PingTask;

class MeasurePing : public Measure
{
public:
	MeasurePing(Skin* skin, const WCHAR* name);
	virtual ~MeasurePing();

	MeasurePing(const MeasurePing& other) = delete;
	MeasurePing& operator=(MeasurePing other) = delete;

	UINT GetTypeID() override { return TypeID<MeasurePing>(); }

	void AdvanceUpdateCounter(UINT count) override;

protected:
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	void UpdateValue() override;

private:
	std::wstring m_Destination;
	DWORD m_Timeout;
	double m_TimeoutValue;
	DWORD m_UpdateRate;
	DWORD m_UpdateRateCounter;
	std::wstring m_FinishAction;
	PingTask* m_Task;
};
