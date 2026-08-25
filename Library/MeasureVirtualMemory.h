// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

class MeasureVirtualMemory : public Measure
{
public:
	MeasureVirtualMemory(Skin* skin, const WCHAR* name);
	virtual ~MeasureVirtualMemory();

	MeasureVirtualMemory(const MeasureVirtualMemory& other) = delete;
	MeasureVirtualMemory& operator=(MeasureVirtualMemory other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasureVirtualMemory>(); }

protected:
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	virtual void UpdateValue();

private:
	bool m_Total;
};
