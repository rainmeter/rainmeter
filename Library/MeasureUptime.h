// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

class MeasureUptime : public Measure
{
public:
	MeasureUptime(Skin* skin, const WCHAR* name);
	virtual ~MeasureUptime();

	MeasureUptime(const MeasureUptime& other) = delete;
	MeasureUptime& operator=(MeasureUptime other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasureUptime>(); }

	virtual const WCHAR* GetStringValue();

protected:
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	virtual void UpdateValue();

private:
	bool m_AddDaysToHours;
	std::wstring m_Format;
	double m_Seconds;
	bool m_SecondsDefined;
};
