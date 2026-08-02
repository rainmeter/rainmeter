// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

class MeasurePhysicalMemory : public Measure
{
public:
	MeasurePhysicalMemory(Skin* skin, const WCHAR* name);
	virtual ~MeasurePhysicalMemory();

	MeasurePhysicalMemory(const MeasurePhysicalMemory& other) = delete;
	MeasurePhysicalMemory& operator=(MeasurePhysicalMemory other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasurePhysicalMemory>(); }

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	bool m_Total;
};
