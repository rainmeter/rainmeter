// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>

namespace DpiUtil {

class DpiUnawareScope
{
public:
	DpiUnawareScope() : m_PreviousContext(SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE)) {}
	DpiUnawareScope(const DpiUnawareScope&) = delete;
	DpiUnawareScope& operator=(const DpiUnawareScope&) = delete;

	~DpiUnawareScope()
	{
		SetThreadDpiAwarenessContext(m_PreviousContext);
	}

private:
	DPI_AWARENESS_CONTEXT m_PreviousContext;
};

// The dialog manager bakes the base units into a dialog when it is created, so MapDialogRect() keeps
// mapping for the DPI the dialog happened to be created at even after it has moved to a monitor with
// a different DPI. These measure the dialog template font at any DPI instead.
SIZE GetDialogBaseUnits(UINT dpi);

// Both map left/right (and cx) horizontally and top/bottom (and cy) vertically, so the RECT overload
// works for a plain rectangle as well as for a { x, y, w, h } tuple.
RECT MapDialogUnits(const RECT& rect, UINT dpi);
SIZE MapDialogUnits(const SIZE& size, UINT dpi);

}  // namespace DpiUtil
