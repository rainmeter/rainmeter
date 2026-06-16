/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_SKINPOSITION_H_
#define RM_LIBRARY_SKINPOSITION_H_

#include <optional>
#include <string>
#include "MonitorUtil.h"

struct SkinPosition
{
	explicit SkinPosition(WCHAR oppositeChar) : oppositeChar(oppositeChar) {}

	static UINT ComputePositionFromOptions(SkinPosition& x, SkinPosition& y, int w, int h, float zoom, const MultiMonitorInfo& monitorsInfo);

	// Logical.
	std::wstring option = L"0";
	std::wstring anchorOption = L"0";

	// Physical pixels.
	int pos = 0;

	// 96 DPI pixels.
	int anchorPos = 0;

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
	int ComputePosition(float parsedValue, int referenceOrigin, int referenceExtent);
	void UpdateOptionValue(int logicalPos, int referenceOrigin, int referenceExtent);

	const WCHAR oppositeChar;
};

#endif
