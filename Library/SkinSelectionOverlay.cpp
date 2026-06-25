/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Skin.h"
#include "SkinSelectionOverlay.h"
#include "Rainmeter.h"
#include "System.h"

const WCHAR* g_ClassName = L"RainmeterSkinSelectionOverlay";

const int g_DashLength = 6;
const int g_DashThickness = 2;
const int g_ZoomDragMinPercent = 10;
const int g_ZoomDragMaxPercent = 500;
const int g_ZoomDragEdgeSize = 10;
const int g_ZoomDragCornerSize = 20;

static bool IsLeftHit(int hit)
{
	return hit == HTLEFT || hit == HTTOPLEFT || hit == HTBOTTOMLEFT;
}

static bool IsRightHit(int hit)
{
	return hit == HTRIGHT || hit == HTTOPRIGHT || hit == HTBOTTOMRIGHT;
}

static bool IsTopHit(int hit)
{
	return hit == HTTOP || hit == HTTOPLEFT || hit == HTTOPRIGHT;
}

static bool IsBottomHit(int hit)
{
	return hit == HTBOTTOM || hit == HTBOTTOMLEFT || hit == HTBOTTOMRIGHT;
}

static HCURSOR GetZoomDragCursorForHit(int hit)
{
	switch (hit)
	{
	case HTLEFT:
	case HTRIGHT:
		return LoadCursor(nullptr, IDC_SIZEWE);

	case HTTOP:
	case HTBOTTOM:
		return LoadCursor(nullptr, IDC_SIZENS);

	case HTTOPLEFT:
	case HTBOTTOMRIGHT:
		return LoadCursor(nullptr, IDC_SIZENWSE);

	case HTTOPRIGHT:
	case HTBOTTOMLEFT:
		return LoadCursor(nullptr, IDC_SIZENESW);
	}

	return nullptr;
}

static int HitTestZoomDragRect(const RECT& rect, POINT screenPos)
{
	if (!PtInRect(&rect, screenPos)) return HTCLIENT;

	const auto w = rect.right - rect.left;
	const auto h = rect.bottom - rect.top;
	const int edgeX = min(g_ZoomDragEdgeSize, max(1, w / 2));
	const int edgeY = min(g_ZoomDragEdgeSize, max(1, h / 2));
	const int cornerX = min(g_ZoomDragCornerSize, max(1, w / 2));
	const int cornerY = min(g_ZoomDragCornerSize, max(1, h / 2));
	const int leftDistance = screenPos.x - rect.left;
	const int rightDistance = rect.right - screenPos.x - 1;
	const int topDistance = screenPos.y - rect.top;
	const int bottomDistance = rect.bottom - screenPos.y - 1;

	bool left = leftDistance < edgeX;
	bool right = rightDistance < edgeX;
	bool top = topDistance < edgeY;
	bool bottom = bottomDistance < edgeY;

	if (left && right)
	{
		if (leftDistance <= rightDistance)
		{
			right = false;
		}
		else
		{
			left = false;
		}
	}

	if (top && bottom)
	{
		if (topDistance <= bottomDistance)
		{
			bottom = false;
		}
		else
		{
			top = false;
		}
	}

	const bool nearLeftCorner = leftDistance < cornerX;
	const bool nearRightCorner = rightDistance < cornerX;
	const bool nearTopCorner = topDistance < cornerY;
	const bool nearBottomCorner = bottomDistance < cornerY;

	if ((left || top) && nearLeftCorner && nearTopCorner && leftDistance <= rightDistance && topDistance <= bottomDistance) return HTTOPLEFT;
	if ((right || top) && nearRightCorner && nearTopCorner && rightDistance < leftDistance && topDistance <= bottomDistance) return HTTOPRIGHT;
	if ((left || bottom) && nearLeftCorner && nearBottomCorner && leftDistance <= rightDistance && bottomDistance < topDistance) return HTBOTTOMLEFT;
	if ((right || bottom) && nearRightCorner && nearBottomCorner && rightDistance < leftDistance && bottomDistance < topDistance) return HTBOTTOMRIGHT;

	if (left) return HTLEFT;
	if (right) return HTRIGHT;
	if (top) return HTTOP;
	if (bottom) return HTBOTTOM;

	return HTCLIENT;
}

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

	return HitTestZoomDragRect(m_Skin->GetPhysicalWindowBounds(), screenPos);
}

bool SkinSelectionOverlay::SetZoomDragCursor(int hit)
{
	HCURSOR cursor = GetZoomDragCursorForHit(hit);
	if (!cursor) return false;

	SetCursor(cursor);
	return true;
}

void SkinSelectionOverlay::BeginZoomDrag(int hit, POINT screenPos)
{
	m_ZoomDrag = ZoomDragState{ hit, m_Skin->GetPhysicalWindowBounds(), screenPos, m_Skin->m_ZoomScale };
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

SkinSelectionOverlay::ZoomDragResult SkinSelectionOverlay::UpdateZoomDrag(POINT screenPos, int windowW, int windowH, float dpiScale, float currentZoom, POINT currentPos)
{
	const int startW = max(1, m_ZoomDrag->startRect.right - m_ZoomDrag->startRect.left);
	const int startH = max(1, m_ZoomDrag->startRect.bottom - m_ZoomDrag->startRect.top);
	const bool horizontal = IsLeftHit(m_ZoomDrag->initialHit) || IsRightHit(m_ZoomDrag->initialHit);
	const bool vertical = IsTopHit(m_ZoomDrag->initialHit) || IsBottomHit(m_ZoomDrag->initialHit);

	int draggedW = startW;
	int draggedH = startH;
	const int dx = screenPos.x - m_ZoomDrag->startPoint.x;
	const int dy = screenPos.y - m_ZoomDrag->startPoint.y;

	if (IsLeftHit(m_ZoomDrag->initialHit))
	{
		draggedW = startW - dx;
	}
	else if (IsRightHit(m_ZoomDrag->initialHit))
	{
		draggedW = startW + dx;
	}

	if (IsTopHit(m_ZoomDrag->initialHit))
	{
		draggedH = startH - dy;
	}
	else if (IsBottomHit(m_ZoomDrag->initialHit))
	{
		draggedH = startH + dy;
	}

	draggedW = max(1, draggedW);
	draggedH = max(1, draggedH);

	float zoom = m_ZoomDrag->startZoom;
	if (horizontal && vertical)
	{
		const float widthRatio = (float)draggedW / (float)startW;
		const float heightRatio = (float)draggedH / (float)startH;
		zoom *= (fabsf(widthRatio - 1.0f) >= fabsf(heightRatio - 1.0f)) ? widthRatio : heightRatio;
	}
	else if (horizontal)
	{
		zoom *= (float)draggedW / (float)startW;
	}
	else if (vertical)
	{
		zoom *= (float)draggedH / (float)startH;
	}

	zoom = max((float)g_ZoomDragMinPercent / 100.0f, min((float)g_ZoomDragMaxPercent / 100.0f, zoom));
	zoom = floorf(zoom * 100.0f + 0.5f) / 100.0f;

	int x = m_ZoomDrag->startRect.left;
	int y = m_ZoomDrag->startRect.top;

	if (IsLeftHit(m_ZoomDrag->initialHit))
	{
		const int widthAtZoom = (int)((float)windowW * dpiScale * zoom);
		x = m_ZoomDrag->startRect.right - widthAtZoom;
	}
	if (IsTopHit(m_ZoomDrag->initialHit))
	{
		const int heightAtZoom = (int)((float)windowH * dpiScale * zoom);
		y = m_ZoomDrag->startRect.bottom - heightAtZoom;
	}

	ZoomDragResult result;
	result.zoom = zoom;
	result.zoomDelta = zoom - m_ZoomDrag->startZoom;
	result.deltaX = m_ZoomDrag->startRect.left - x;
	result.deltaY = m_ZoomDrag->startRect.top - y;
	result.positionChanged = (currentPos.x + result.deltaX) != m_ZoomDrag->startRect.left || (currentPos.y + result.deltaY) != m_ZoomDrag->startRect.top;
	result.changed = fabsf(currentZoom - zoom) > 0.0001f || currentPos.x != x || currentPos.y != y;

	if (result.changed)
	{
		m_ZoomDrag->moved = true;
		m_ZoomDrag->positionChanged = m_ZoomDrag->positionChanged || result.positionChanged;
	}

	return result;
}

void SkinSelectionOverlay::ApplyZoomDrag()
{
	if (!m_ZoomDrag) return;

	POINT pos = { m_Skin->m_X.pos, m_Skin->m_Y.pos };
	const auto& result = UpdateZoomDrag(System::GetCursorPosition(), m_Skin->m_WindowW, m_Skin->m_WindowH, m_Skin->m_DpiScale, m_Skin->m_ZoomScale, pos);
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

	if (m_ZoomDrag->moved)
	{
		for (const auto& state : m_ZoomDragStartStates)
		{
			state.skin->WriteOptions(Skin::OPTION_ZOOM);

			if (m_ZoomDrag->positionChanged)
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
		SetZoomDragCursor(m_ZoomDrag->initialHit);
	}
	else
	{
		const int hit =
			(uMsg == WM_NCMOUSEMOVE && GetZoomDragCursorForHit((int)wParam)) ?
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
		(uMsg == WM_NCLBUTTONDOWN && GetZoomDragCursorForHit((int)wParam)) ?
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
