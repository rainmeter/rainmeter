// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

class MeasureMemory : public Measure
{
public:
	MeasureMemory(Skin* skin, const WCHAR* name);
	virtual ~MeasureMemory();

	MeasureMemory(const MeasureMemory& other) = delete;
	MeasureMemory& operator=(MeasureMemory other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasureMemory>(); }

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	bool m_Total;
};
