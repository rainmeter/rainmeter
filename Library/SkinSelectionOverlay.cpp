/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Skin.h"
#include "SkinSelectionOverlay.h"
#include "SkinZoomDrag.h"
#include "Rainmeter.h"
#include "System.h"

const WCHAR* g_ClassName = L"RainmeterSkinSelectionOverlay";

const int g_DashLength = 6;
const int g_DashGap = 4;
const int g_DashThickness = 2;

static BYTE ColorToByte(float value)
{
	return (BYTE)max(0, min(255, (int)roundf(value * 255.0f)));
}

static DWORD MakePixel(const D2D1_COLOR_F& color, BYTE alpha)
{
	const float alphaScale = (float)alpha / 255.0f;
	const BYTE r = ColorToByte(color.r * alphaScale);
	const BYTE g = ColorToByte(color.g * alphaScale);
	const BYTE b = ColorToByte(color.b * alphaScale);
	return ((DWORD)alpha << 24) | ((DWORD)r << 16) | ((DWORD)g << 8) | b;
}

static void DrawHorizontalDash(DWORD* pixels, int width, int height, int y, DWORD color, DWORD alternateColor)
{
	for (int x = 0, dash = 0; x < width; x += g_DashLength, ++dash)
	{
		const DWORD dashColor = (dash % 2 == 0) ? color : alternateColor;
		for (int dashX = x; dashX < min(width, x + g_DashLength); ++dashX)
		{
			for (int offset = 0; offset < min(g_DashThickness, height); ++offset)
			{
				pixels[(y + offset) * width + dashX] = dashColor;
			}
		}
	}
}

static void DrawVerticalDash(DWORD* pixels, int width, int height, int x, DWORD color, DWORD alternateColor)
{
	for (int y = 0, dash = 0; y < height; y += g_DashLength, ++dash)
	{
		const DWORD dashColor = (dash % 2 == 0) ? color : alternateColor;
		for (int dashY = y; dashY < min(height, y + g_DashLength); ++dashY)
		{
			for (int offset = 0; offset < min(g_DashThickness, width); ++offset)
			{
				pixels[dashY * width + x + offset] = dashColor;
			}
		}
	}
}

SkinSelectionOverlay::SkinSelectionOverlay(Skin* skin) :
	m_Skin(skin),
	m_Window(),
	m_ZoomDrag(),
	m_ZoomDragStartStates()
{
	WNDCLASS wc = { 0 };
	wc.hInstance = GetRainmeter().GetModuleInstance();
	wc.hCursor = nullptr;
	wc.lpszClassName = g_ClassName;
	wc.lpfnWndProc = [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT
	{
		auto instance = (SkinSelectionOverlay*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

		if (uMsg == WM_NCCREATE)
		{
			instance = (SkinSelectionOverlay*)((CREATESTRUCT*)lParam)->lpCreateParams;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)instance);
		}

		BEGIN_MESSAGEPROC
		MESSAGE(OnNcHitTest, WM_NCHITTEST)
		MESSAGE(OnSetCursor, WM_SETCURSOR)
		MESSAGE(OnMouseMove, WM_NCMOUSEMOVE)
		MESSAGE(OnMouseMove, WM_MOUSEMOVE)
		MESSAGE(OnLeftButtonDown, WM_NCLBUTTONDOWN)
		MESSAGE(OnLeftButtonDown, WM_LBUTTONDOWN)
		MESSAGE(OnLeftButtonUp, WM_LBUTTONUP)
		MESSAGE(OnLeftButtonUp, WM_CAPTURECHANGED)
		MESSAGE(OnContextMenu, WM_CONTEXTMENU)
		MESSAGE(OnCommand, WM_COMMAND)
		END_MESSAGEPROC
	};
	static auto s_ClassAtom = RegisterClass(&wc);

	Create();
}

SkinSelectionOverlay::~SkinSelectionOverlay()
{
	Destroy();
}

void SkinSelectionOverlay::Create()
{
	if (m_Window || !m_Skin->m_Window) return;

	m_Window = CreateWindowEx(WS_EX_LAYERED, g_ClassName, nullptr, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_Skin->m_Window, nullptr, GetRainmeter().GetModuleInstance(), this);

	Update();
}

void SkinSelectionOverlay::Destroy()
{
	CommitZoomDrag();

	if (m_Window)
	{
		DestroyWindow(m_Window);
		m_Window = nullptr;
	}
}

void SkinSelectionOverlay::Update()
{
	if (!m_Window) return;

	const int width = m_Skin->GetPhysicalWindowW();
	const int height = m_Skin->GetPhysicalWindowH();
	if (width <= 0 || height <= 0) return;

	SetWindowPos(m_Window, HWND_TOP, 0, 0, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);

	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	void* bits = nullptr;
	HDC screenDC = GetDC(nullptr);
	HDC memoryDC = CreateCompatibleDC(screenDC);
	HBITMAP bitmap = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
	if (!memoryDC || !bitmap || !bits)
	{
		if (bitmap) DeleteObject(bitmap);
		if (memoryDC) DeleteDC(memoryDC);
		if (screenDC) ReleaseDC(nullptr, screenDC);
		return;
	}

	HGDIOBJ oldBitmap = SelectObject(memoryDC, bitmap);

	DWORD* pixels = (DWORD*)bits;
	const DWORD fillColor = MakePixel(m_Skin->m_SelectedColor, 80);
	std::fill(pixels, pixels + (width * height), fillColor);

	const DWORD dashColor = MakePixel(m_Skin->m_SelectedColor, 240);
	const DWORD alternateDashColor = MakePixel(D2D1::ColorF(D2D1::ColorF::Black), 240);
	DrawHorizontalDash(pixels, width, height, 0, dashColor, alternateDashColor);
	DrawHorizontalDash(pixels, width, height, max(0, height - g_DashThickness), dashColor, alternateDashColor);
	DrawVerticalDash(pixels, width, height, 0, dashColor, alternateDashColor);
	DrawVerticalDash(pixels, width, height, max(0, width - g_DashThickness), dashColor, alternateDashColor);

	POINT dst = { 0 };
	POINT src = { 0 };
	SIZE size = { width, height };
	BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
	UpdateLayeredWindow(m_Window, screenDC, &dst, &size, memoryDC, &src, 0, &blend, ULW_ALPHA);

	SelectObject(memoryDC, oldBitmap);
	DeleteObject(bitmap);
	DeleteDC(memoryDC);
	ReleaseDC(nullptr, screenDC);
}

int SkinSelectionOverlay::HitTestZoomDrag(POINT screenPos) const
{
	if (!m_Skin->m_WindowDraggable || GetRainmeter().GetDisableDragging())
	{
		return HTCLIENT;
	}

	return SkinZoomDrag::HitTest(m_Skin->GetPhysicalWindowBounds(), screenPos);
}

bool SkinSelectionOverlay::SetZoomDragCursor(int hit)
{
	HCURSOR cursor = SkinZoomDrag::GetCursorForHit(hit);
	if (!cursor) return false;

	SetCursor(cursor);
	return true;
}

void SkinSelectionOverlay::BeginZoomDrag(int hit, POINT screenPos)
{
	m_ZoomDrag = std::make_unique<SkinZoomDrag>(hit, m_Skin->GetPhysicalWindowBounds(), screenPos, m_Skin->m_ZoomScale);
	m_ZoomDragStartStates.clear();

	for (const auto& skins : GetRainmeter().GetAllSkins())
	{
		Skin* skin = skins.second;
		if (skin->IsSelected())
		{
			m_ZoomDragStartStates.push_back({ skin, { skin->m_X.pos, skin->m_Y.pos }, skin->m_ZoomScale });
		}
	}
}

void SkinSelectionOverlay::ApplyZoomDrag()
{
	if (!m_ZoomDrag) return;

	POINT pos = { m_Skin->m_X.pos, m_Skin->m_Y.pos };
	const auto& result = m_ZoomDrag->Update(System::GetCursorPosition(), m_Skin->m_WindowW, m_Skin->m_WindowH, m_Skin->m_DpiScale, m_Skin->m_ZoomScale, pos);
	if (!result.changed) return;

	for (const auto& state : m_ZoomDragStartStates)
	{
		state.skin->m_X.pos = state.pos.x + result.deltaX;
		state.skin->m_Y.pos = state.pos.y + result.deltaY;
		state.skin->ApplyZoom(state.zoom + result.zoomDelta, false);
	}
}

void SkinSelectionOverlay::CommitZoomDrag()
{
	if (!m_ZoomDrag) return;

	if (m_ZoomDrag->HasMoved())
	{
		for (const auto& state : m_ZoomDragStartStates)
		{
			state.skin->WriteOptions(Skin::OPTION_ZOOM);

			if (m_ZoomDrag->HasPositionChanged())
			{
				state.skin->SavePositionIfAppropriate();
			}
		}
	}

	m_ZoomDrag.reset();
	m_ZoomDragStartStates.clear();

	if (GetCapture() == m_Window)
	{
		ReleaseCapture();
	}
}

LRESULT SkinSelectionOverlay::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return HitTestZoomDrag({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
}

LRESULT SkinSelectionOverlay::OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int hit = LOWORD(lParam);
	if (hit == HTCLIENT)
	{
		hit = HitTestZoomDrag(System::GetCursorPosition());
	}

	if (SetZoomDragCursor(hit))
	{
		return TRUE;
	}

	SetCursor(LoadCursor(nullptr, IDC_ARROW));
	return TRUE;
}

LRESULT SkinSelectionOverlay::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_ZoomDrag)
	{
		ApplyZoomDrag();
		SetZoomDragCursor(m_ZoomDrag->GetInitialHit());
	}
	else
	{
		const int hit =
			(uMsg == WM_NCMOUSEMOVE && SkinZoomDrag::GetCursorForHit((int)wParam)) ?
			(int)wParam :
			HitTestZoomDrag(System::GetCursorPosition());
		if (!SetZoomDragCursor(hit))
		{
			SetCursor(LoadCursor(nullptr, IDC_ARROW));
		}
	}
	return 0;
}

LRESULT SkinSelectionOverlay::OnLeftButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT screenPos = System::GetCursorPosition();
	const int zoomDragHitTest =
		(uMsg == WM_NCLBUTTONDOWN && SkinZoomDrag::GetCursorForHit((int)wParam)) ?
		(int)wParam :
		HitTestZoomDrag(screenPos);
	if (zoomDragHitTest != HTCLIENT)
	{
		BeginZoomDrag(zoomDragHitTest, screenPos);
		ApplyZoomDrag();
		SetCapture(m_Window);
		SetZoomDragCursor(zoomDragHitTest);
		return 0;
	}

	return ForwardMessageToSkin(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(screenPos.x, screenPos.y));
}

LRESULT SkinSelectionOverlay::OnLeftButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_ZoomDrag) CommitZoomDrag();
	return 0;
}

LRESULT SkinSelectionOverlay::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos = { 0 };
	if ((lParam & 0xFFFFFFFF) == 0xFFFFFFFF)
	{
		RECT rect = { 0 };
		GetWindowRect(m_Window, &rect);
		pos.x = rect.left;
		pos.y = rect.top;
	}
	else
	{
		pos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	}

	GetRainmeter().ShowSkinSelectionContextMenu(pos, m_Skin, m_Window);
	return 0;
}

LRESULT SkinSelectionOverlay::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	for (const auto& [path, skin] : GetRainmeter().GetAllSkins())
	{
		if (wParam == IDM_SKIN_SELECT || skin->IsSelected())
		{
			SendMessage(skin->GetWindow(), WM_COMMAND, wParam, lParam);
		}
	}

	return 0;
}

LRESULT SkinSelectionOverlay::ForwardMessageToSkin(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SendMessage(m_Skin->m_Window, uMsg, wParam, lParam);
	return 0;
}
