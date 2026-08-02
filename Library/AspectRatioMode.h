// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

enum class AspectRatioMode
{
	Stretch,
	Fit,
	Crop
};

inline AspectRatioMode ParseAspectRatioMode(int value)
{
	switch (value)
	{
	case 0:
		return AspectRatioMode::Stretch;
	case 2:
		return AspectRatioMode::Crop;
	case 1:
	default:
		return AspectRatioMode::Fit;
	}
}
