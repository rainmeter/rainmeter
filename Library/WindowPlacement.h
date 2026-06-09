/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_WINDOWPLACEMENT_H_
#define RM_LIBRARY_WINDOWPLACEMENT_H_

#include <string>
#include "System.h"

namespace WindowPlacement {

struct Input
{
	std::wstring windowX;
	std::wstring windowY;
	std::wstring anchorX;
	std::wstring anchorY;
	int windowW;
	int windowH;
	float scale;
	float oldScale = 0.0f;
	bool anchorXDefined = true;
	bool anchorYDefined = true;
};

struct AxisResult
{
	int screen;
	bool screenDefined;
	bool fromFarEdge;
	bool percentage;
	int anchorScreen;
	bool anchorFromFarEdge;
	bool anchorPercentage;
	int coordinate;
};

struct Result
{
	AxisResult x;
	AxisResult y;
};

Result WindowToScreen(const Input& input, const MultiMonitorInfo& monitorsInfo);

}  // namespace WindowPlacement

#endif
