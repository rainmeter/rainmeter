// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "MeasureNet.h"

class MeasureNetTotal : public MeasureNet
{
public:
	MeasureNetTotal(Skin* skin, const WCHAR* name);
	virtual ~MeasureNetTotal();

	MeasureNetTotal(const MeasureNetTotal& other) = delete;
	MeasureNetTotal& operator=(MeasureNetTotal other) = delete;
};
