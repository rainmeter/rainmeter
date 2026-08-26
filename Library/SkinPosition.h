// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "MonitorUtil.h"

class SkinPositionOption
{
public:
	explicit SkinPositionOption(WCHAR oppositeChar) : oppositeChar(oppositeChar) {}

	// The window option to save. The authored option takes precedence over the value derived from
	// the actual position, so that a position we were forced into, such as one clamped or resolved
	// against another monitor while the intended monitor was unavailable, is not written over it.
	const std::wstring& GetWindowOptionToSave() const { return authoredWindowOption ? *authoredWindowOption : windowOption; }

	const std::wstring& GetAnchorOption() const { return anchorOption; }
	int GetAnchorPos() const { return anchorPos; }

	const std::optional<int>& GetMonitor() const { return monitor; }
	bool IsFromOpposite() const { return fromOpposite; }
	bool IsPercentage() const { return percentage; }

	// Sets the option as authored by a skin option, or by a bang that takes the same syntax.
	void SetAuthoredWindowOption(std::wstring option);

	// Sets the option to an explicit position, superseding any authored option.
	void SetWindowOption(std::wstring option) { windowOption = std::move(option); authoredWindowOption.reset(); }

	void SetAnchorOption(std::wstring option) { anchorOption = std::move(option); }

	// Changes how the position is expressed. Both modifiers become part of the window option, so
	// the authored option no longer applies once either of them is changed.
	void SetFromOpposite(bool b) { fromOpposite = b; authoredWindowOption.reset(); }
	void SetPercentage(bool b) { percentage = b; authoredWindowOption.reset(); }

	// Rewrites the window option to express |logicalPos| with the current modifiers.
	void UpdateOptionValue(int logicalPos, int referenceOrigin, int referenceExtent);

private:
	friend class SkinPosition;
	friend class Library_SkinPosition_Test;

	void ParseAnchorOption(int windowSize, float zoom);
	float ParseWindowOption(const std::vector<MonitorInfo>& monitors);
	int ResolveLogicalPosition(float parsedValue, int referenceOrigin, int referenceExtent);

	// Logical (96 DPI)
	std::wstring windowOption = L"0";
	std::wstring anchorOption = L"0";
	int anchorPos = 0;

	std::optional<int> monitor;
	bool fromOpposite = false;
	bool percentage = false;
	bool anchorFromOpposite = false;
	bool anchorPercentage = false;

	// The window option as authored by a skin option or a bang, kept for as long as the position
	// is derived from it (see SkinPositionOrigin). |windowOption| is rewritten from the actual
	// position whenever the position is saved, which loses the authored value if the position was
	// clamped or resolved against another monitor while the intended monitor was unavailable.
	// Keeping the authored value around allows restoring it once the display topology changes.
	std::optional<std::wstring> authoredWindowOption;

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

// Whether a position supersedes the authored WindowX/WindowY options or is merely derived from
// them. Resolving the options and fitting the result within the screen area keep the authored
// options intact, while a move that puts the skin somewhere else replaces them.
enum class SkinPositionOrigin : BYTE
{
	Move,
	Options
};

class SkinPosition
{
public:
	SkinPosition();

	POINT AsPhysical(SIZE windowSize) const;
	POINT AsVirtualized(HMONITOR monitor) const;
	void SetPhysical(POINT position, SkinPositionOrigin origin = SkinPositionOrigin::Move);
	void SetVirtualized(POINT position, SkinPositionOrigin origin = SkinPositionOrigin::Move);

	bool IsVirtualized() const { return m_Space == SkinPositionSpace::Virtualized; }
	SkinPositionSpace GetSpace() const { return m_Space; }

	void ResetCache() { m_ConvertedPos.reset(); }

	void SetMonitor(std::optional<int> monitor, SkinPositionOrigin origin = SkinPositionOrigin::Move);

	void RestoreAuthoredWindowOptions();

	SkinPositionOption& GetX() { return m_X; }
	const SkinPositionOption& GetX() const { return m_X; }

	SkinPositionOption& GetY() { return m_Y; }
	const SkinPositionOption& GetY() const { return m_Y; }

	POINT ResolveVirtualizedPosition(int w, int h, float zoom, const MultiMonitorInfo& monitorsInfo);

private:
	void ClearAuthoredWindowOptions();

	SkinPositionOption m_X;
	SkinPositionOption m_Y;
	POINT m_Pos = {};

	// Cached representation of m_Pos in the other coordinate space.
	mutable std::optional<POINT> m_ConvertedPos;

	SkinPositionSpace m_Space = SkinPositionSpace::Virtualized;
};
