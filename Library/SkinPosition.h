/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_SKINPOSITION_H_
#define RM_LIBRARY_SKINPOSITION_H_

#include "MonitorUtil.h"

struct SkinPosition
{
	explicit SkinPosition(WCHAR oppositeChar) : oppositeChar(oppositeChar) {}

	// WindowX/WindowY and AnchorX/AnchorY are authored in 96 DPI logical coordinates. Compute
	// the referenced logical x/y point first so the caller can convert that screen logical point
	// to physical coordinates using the appropriate window or system DPI context.
	static POINT ResolveScreenLogicalPosition(SkinPosition& x, SkinPosition& y, int w, int h, float zoom, const MultiMonitorInfo& monitorsInfo);

	// Logical (96 DPI)
	std::wstring option = L"0";
	std::wstring anchorOption = L"0";
	int anchorPos = 0;

	// Physical
	int pos = 0;

	std::optional<int> monitor;
	bool fromOpposite = false;
	bool percentage = false;
	bool anchorFromOpposite = false;
	bool anchorPercentage = false;

private:
	friend class Skin;
	friend class Library_SkinPosition_Test;

	void ParseAnchorOption(int windowSize, float zoom);
	float ParseWindowOption(const std::vector<MonitorInfo>& monitors);
	void UpdateOptionValue(int logicalPos, int referenceOrigin, int referenceExtent);
	int ResolveLogicalPosition(float parsedValue, int referenceOrigin, int referenceExtent);

	const WCHAR oppositeChar;
};

#endif
