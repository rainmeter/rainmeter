// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "MeasureNet.h"

class MeasureNetIn : public MeasureNet
{
public:
	MeasureNetIn(Skin* skin, const WCHAR* name);
	virtual ~MeasureNetIn();

	MeasureNetIn(const MeasureNetIn& other) = delete;
	MeasureNetIn& operator=(MeasureNetIn other) = delete;
};
