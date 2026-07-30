/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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
