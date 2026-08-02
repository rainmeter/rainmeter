// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

class MeasureMediaKey : public Measure
{
public:
	MeasureMediaKey(Skin* skin, const WCHAR* name);
	virtual ~MeasureMediaKey();

	MeasureMediaKey(const MeasureMediaKey& other) = delete;
	MeasureMediaKey& operator=(MeasureMediaKey other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureMediaKey>(); }

protected:
	void UpdateValue() override {};
	void Command(const std::wstring& command) override;
};
