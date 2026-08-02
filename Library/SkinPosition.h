/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "MonitorUtil.h"

class Skin;

struct SkinPositionOption
{
	explicit SkinPositionOption(WCHAR oppositeChar) : oppositeChar(oppositeChar) {}

	// Logical (96 DPI)
	std::wstring windowOption = L"0";
	std::wstring anchorOption = L"0";
	int anchorPos = 0;

	std::optional<int> monitor;
	bool fromOpposite = false;
	bool percentage = false;
	bool anchorFromOpposite = false;
	bool anchorPercentage = false;

private:
	friend class Skin;
	friend class SkinPosition;
	friend class Library_SkinPosition_Test;

	void ParseAnchorOption(int windowSize, float zoom);
	float ParseWindowOption(const std::vector<MonitorInfo>& monitors);
	void UpdateOptionValue(int logicalPos, int referenceOrigin, int referenceExtent);
	int ResolveLogicalPosition(float parsedValue, int referenceOrigin, int referenceExtent);

	const WCHAR oppositeChar;
};

// Identifies the coordinate space used by a skin position. Physical coordinates are actual
// desktop pixels and are authoritative after Windows moves the skin, such as during dragging.
// Virtualized coordinates are the 96-DPI screen coordinates exposed to a DPI-unaware window
// on the current monitor and are authoritative when a position option or bang is resolved.
// Rainmeter versions before 5.0 were DPI-unaware, so values such as WindowX/WindowY and must
// coordinates passed to bangs were written with these virtualized coordinates in mind. We must
// continue tracking that coordinate space to interpret existing skins and settings as their
// authors intended, even though Rainmeter now positions its windows in physical pixels. These
// spaces do not have a one-to-one mapping across monitors with different DPI settings, so
// SkinPosition::m_Pos retains whichever representation was last set instead of repeatedly
// converting it. SkinPosition::m_ConvertedPos, when present, is only a cached representation in
// the opposite space.
enum class SkinPositionSpace : BYTE
{
	Physical,
	Virtualized
};

class SkinPosition
{
public:
	SkinPosition();

	POINT AsPhysical(SIZE windowSize) const;
	POINT AsVirtualized(HMONITOR monitor) const;
	void SetPhysical(POINT position);
	void SetVirtualized(POINT position);

	bool IsVirtualized() const { return m_Space == SkinPositionSpace::Virtualized; }
	SkinPositionSpace GetSpace() const { return m_Space; }

	void ResetCache() { m_ConvertedPos.reset(); }

	SkinPositionOption& GetX() { return m_X; }
	const SkinPositionOption& GetX() const { return m_X; }

	SkinPositionOption& GetY() { return m_Y; }
	const SkinPositionOption& GetY() const { return m_Y; }

	POINT ResolveVirtualizedPosition(int w, int h, float zoom, const MultiMonitorInfo& monitorsInfo);

private:
	SkinPositionOption m_X;
	SkinPositionOption m_Y;
	POINT m_Pos = {};

	// Cached representation of m_Pos in the other coordinate space.
	mutable std::optional<POINT> m_ConvertedPos;

	SkinPositionSpace m_Space = SkinPositionSpace::Virtualized;
};
