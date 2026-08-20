// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

class MeasureProcess : public Measure
{
public:
	MeasureProcess(Skin* skin, const WCHAR* name);
	virtual ~MeasureProcess();

	MeasureProcess(const MeasureProcess& other) = delete;
	MeasureProcess& operator=(MeasureProcess other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureProcess>(); }

protected:
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	void UpdateValue() override;

private:
	std::wstring m_ProcessNameLowercase;
};
