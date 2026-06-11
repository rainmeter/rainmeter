/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "ControlTemplate.h"

namespace {

BOOL GetNonClientMetricsForDpi(NONCLIENTMETRICS& metrics, UINT dpi)
{
	typedef BOOL (WINAPI* SystemParametersInfoForDpiProc)(UINT, UINT, PVOID, UINT, UINT);
	static auto s_SystemParametersInfoForDpi = (SystemParametersInfoForDpiProc)GetProcAddress(
		GetModuleHandle(L"user32"), "SystemParametersInfoForDpi");
	if (s_SystemParametersInfoForDpi)
	{
		return s_SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0, dpi);
	}

	if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0)) return FALSE;

	HDC dc = GetDC(nullptr);
	const int systemDpi = dc ? GetDeviceCaps(dc, LOGPIXELSX) : 96;
	if (dc) ReleaseDC(nullptr, dc);
	if (systemDpi > 0 && (UINT)systemDpi != dpi)
	{
		metrics.lfMenuFont.lfHeight = MulDiv(metrics.lfMenuFont.lfHeight, (int)dpi, systemDpi);
		metrics.lfMenuFont.lfWidth = MulDiv(metrics.lfMenuFont.lfWidth, (int)dpi, systemDpi);
	}

	return TRUE;
}

}  // namespace

ControlTemplate::ControlTemplate() :
	m_Parent(),
	m_InitialParentSize(),
	m_InitialDpi(),
	m_CurrentDpi(),
	m_Font(),
	m_FontBold()
{
}

ControlTemplate::~ControlTemplate()
{
	if (m_Font) DeleteObject(m_Font);
	if (m_FontBold) DeleteObject(m_FontBold);
}

void ControlTemplate::UpdateFonts(UINT dpi)
{
	NONCLIENTMETRICS metrics = { 0 };
	metrics.cbSize = sizeof(metrics);
	if (!GetNonClientMetricsForDpi(metrics, dpi)) return;

	HFONT oldFont = m_Font;
	HFONT oldFontBold = m_FontBold;

	m_Font = CreateFontIndirect(&metrics.lfMenuFont);
	metrics.lfMenuFont.lfWeight = FW_BOLD;
	metrics.lfMenuFont.lfHeight -= MulDiv(2, (int)dpi, 96);
	m_FontBold = CreateFontIndirect(&metrics.lfMenuFont);

	SendMessage(m_Parent, WM_SETFONT, (WPARAM)m_Font, FALSE);
	for (const auto& createdControl : m_Controls)
	{
		HFONT controlFont = (createdControl.control.options & Control::BOLD_FONT) ? m_FontBold : m_Font;
		SendMessage(createdControl.window, WM_SETFONT, (WPARAM)controlFont, FALSE);
	}

	if (oldFont) DeleteObject(oldFont);
	if (oldFontBold) DeleteObject(oldFontBold);
}

void ControlTemplate::Initialize(const Control* cts, UINT ctCount, HWND parent, UINT dpi, GetStringFunc getString)
{
	assert(!m_Parent);
	if (m_Parent) return;

	m_Parent = parent;
	m_InitialDpi = dpi;
	m_CurrentDpi = dpi;
	UpdateFonts(dpi);

	RECT parentRect;
	GetClientRect(parent, &parentRect);
	m_InitialParentSize.cx = parentRect.right;
	m_InitialParentSize.cy = parentRect.bottom;
	m_Controls.reserve(ctCount);

	for (UINT i = 0; i < ctCount; ++i)
	{
		const Control& ct = cts[i];
		WCHAR* text = ct.textId ? getString(ct.textId) : nullptr;

		RECT r = { ct.x, ct.y, ct.w, ct.h };
		MapDialogRect(parent, &r);

		HWND wnd = CreateWindowEx(
			ct.exStyle,
			ct.name,
			text,
			ct.style,
			r.left, r.top, r.right, r.bottom,
			parent,
			(HMENU)ct.id,
			nullptr,
			nullptr);

		HFONT font = (ct.options & Control::BOLD_FONT) ? m_FontBold : m_Font;
		SendMessage(wnd, WM_SETFONT, (WPARAM)font, FALSE);

		m_Controls.push_back({ ct, wnd, r });
	}
}

void ControlTemplate::Relayout(UINT dpi)
{
	if (m_Controls.empty() || !m_InitialDpi) return;

	if (dpi != m_CurrentDpi)
	{
		m_CurrentDpi = dpi;
		UpdateFonts(dpi);
	}

	RECT clientRect;
	GetClientRect(m_Parent, &clientRect);

	const int scaledDesignWidth = MulDiv(m_InitialParentSize.cx, (int)dpi, (int)m_InitialDpi);
	const int scaledDesignHeight = MulDiv(m_InitialParentSize.cy, (int)dpi, (int)m_InitialDpi);
	const int deltaX = clientRect.right - scaledDesignWidth;
	const int deltaY = clientRect.bottom - scaledDesignHeight;

	HDWP dwp = BeginDeferWindowPos((int)m_Controls.size());
	for (const auto& createdControl : m_Controls)
	{
		const Control& ct = createdControl.control;
		RECT rect =
		{
			MulDiv(createdControl.initialRect.left, (int)dpi, (int)m_InitialDpi),
			MulDiv(createdControl.initialRect.top, (int)dpi, (int)m_InitialDpi),
			MulDiv(createdControl.initialRect.right, (int)dpi, (int)m_InitialDpi),
			MulDiv(createdControl.initialRect.bottom, (int)dpi, (int)m_InitialDpi)
		};

		const bool left = (ct.options & Control::ANCHOR_LEFT) != 0;
		const bool top = (ct.options & Control::ANCHOR_TOP) != 0;
		const bool right = (ct.options & Control::ANCHOR_RIGHT) != 0;
		const bool bottom = (ct.options & Control::ANCHOR_BOTTOM) != 0;

		if (right)
		{
			if (left) rect.right += deltaX;
			else rect.left += deltaX;
		}
		if (bottom)
		{
			if (top) rect.bottom += deltaY;
			else rect.top += deltaY;
		}

		if (dwp)
		{
			dwp = DeferWindowPos(dwp, createdControl.window, nullptr, rect.left, rect.top,
				rect.right, rect.bottom,
				SWP_NOACTIVATE | SWP_NOZORDER);
		}
		else
		{
			SetWindowPos(createdControl.window, nullptr, rect.left, rect.top,
				rect.right, rect.bottom,
				SWP_NOACTIVATE | SWP_NOZORDER);
		}
	}

	if (dwp) EndDeferWindowPos(dwp);
}

UINT ControlTemplate::ScaleDialogUnits(int value) const
{
	return MulDiv(value, m_CurrentDpi, 96);
}
