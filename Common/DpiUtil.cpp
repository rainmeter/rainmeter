// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "DpiUtil.h"

SIZE DpiUtil::GetDialogBaseUnits(UINT dpi)
{
	// Measuring needs a font and a DC, so keep the last result around. Dialogs relayout on every
	// resize and only a couple of distinct DPI values are ever in play.
	static UINT s_Dpi = 0;
	static SIZE s_BaseUnits = { 0, 0 };
	if (dpi == s_Dpi) return s_BaseUnits;

	// Fall back to what the dialog font measures to on a default 96 DPI setup.
	SIZE baseUnits = { MulDiv(6, (int)dpi, USER_DEFAULT_SCREEN_DPI), MulDiv(13, (int)dpi, USER_DEFAULT_SCREEN_DPI) };

	// The font the dialog templates ask for through DS_SHELLFONT.
	LOGFONT logFont = { 0 };
	logFont.lfHeight = -MulDiv(8, (int)dpi, 72);
	logFont.lfWeight = FW_NORMAL;
	logFont.lfCharSet = DEFAULT_CHARSET;
	wcscpy_s(logFont.lfFaceName, L"MS Shell Dlg 2");

	HFONT font = CreateFontIndirect(&logFont);
	HDC dc = GetDC(nullptr);
	if (font && dc)
	{
		HGDIOBJ oldFont = SelectObject(dc, font);

		// The average character width from GetTextMetrics() is unreliable for proportional fonts, so
		// derive the horizontal unit from the alphabet like the dialog manager does.
		const WCHAR alphabet[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
		TEXTMETRIC metrics = { 0 };
		SIZE extent = { 0 };
		if (GetTextMetrics(dc, &metrics) && GetTextExtentPoint32(dc, alphabet, 52, &extent))
		{
			baseUnits.cx = (extent.cx / 26 + 1) / 2;
			baseUnits.cy = metrics.tmHeight;
		}

		SelectObject(dc, oldFont);
	}

	if (dc) ReleaseDC(nullptr, dc);
	if (font) DeleteObject(font);

	s_Dpi = dpi;
	s_BaseUnits = baseUnits;
	return baseUnits;
}

RECT DpiUtil::MapDialogUnits(const RECT& rect, UINT dpi)
{
	const SIZE baseUnits = GetDialogBaseUnits(dpi);
	return
	{
		MulDiv(rect.left, baseUnits.cx, 4),
		MulDiv(rect.top, baseUnits.cy, 8),
		MulDiv(rect.right, baseUnits.cx, 4),
		MulDiv(rect.bottom, baseUnits.cy, 8)
	};
}

SIZE DpiUtil::MapDialogUnits(const SIZE& size, UINT dpi)
{
	const SIZE baseUnits = GetDialogBaseUnits(dpi);
	return { MulDiv(size.cx, baseUnits.cx, 4), MulDiv(size.cy, baseUnits.cy, 8) };
}
