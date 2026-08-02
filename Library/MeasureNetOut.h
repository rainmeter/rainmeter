// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "MeasureNet.h"

class MeasureNetOut : public MeasureNet
{
public:
	MeasureNetOut(Skin* skin, const WCHAR* name);
	virtual ~MeasureNetOut();

	MeasureNetOut(const MeasureNetOut& other) = delete;
	MeasureNetOut& operator=(MeasureNetOut other) = delete;
};
