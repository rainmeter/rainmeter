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

SkinSelectionOverlay::SkinSelectionOverlay(Skin* skin) :
	m_Skin(skin),
	m_Window(),
	m_ZoomDrag()
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
		MESSAGE(OnPaint, WM_PAINT)
		MESSAGE(OnSetCursor, WM_SETCURSOR)
		MESSAGE(OnMouseMove, WM_NCMOUSEMOVE)
		MESSAGE(OnMouseMove, WM_MOUSEMOVE)
		MESSAGE(OnLeftButtonDown, WM_NCLBUTTONDOWN)
		MESSAGE(OnLeftButtonDown, WM_LBUTTONDOWN)
		MESSAGE(OnLeftButtonUp, WM_LBUTTONUP)
		MESSAGE(OnLeftButtonUp, WM_CAPTURECHANGED)
		MESSAGE(ForwardMessageToSkin, WM_CONTEXTMENU)
		MESSAGE(ForwardMessageToSkin, WM_RBUTTONUP)
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

	const BYTE alpha = (BYTE)max(0, min(255, (int)roundf(m_Skin->m_SelectedColor.a * 255.0f)));
	const COLORREF color = RGB(
		max(0, min(255, (int)roundf(m_Skin->m_SelectedColor.r * 255.0f))),
		max(0, min(255, (int)roundf(m_Skin->m_SelectedColor.g * 255.0f))),
		max(0, min(255, (int)roundf(m_Skin->m_SelectedColor.b * 255.0f))));

	SetLayeredWindowAttributes(m_Window, color, alpha, LWA_ALPHA);
	SetWindowPos(m_Window, HWND_TOP, 0, 0, m_Skin->GetPhysicalWindowW(), m_Skin->GetPhysicalWindowH(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
	InvalidateRect(m_Window, nullptr, TRUE);
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
}

void SkinSelectionOverlay::ApplyZoomDrag()
{
	if (!m_ZoomDrag) return;

	POINT pos = { m_Skin->m_X.pos, m_Skin->m_Y.pos };
	const auto& result = m_ZoomDrag->Update(System::GetCursorPosition(), m_Skin->m_WindowW, m_Skin->m_WindowH, m_Skin->m_DpiScale, m_Skin->m_ZoomScale, pos);
	if (!result.changed) return;

	for (const auto& skins : GetRainmeter().GetAllSkins())
	{
		Skin* skin = skins.second;
		if (skin->IsSelected())
		{
			skin->m_X.pos += result.deltaX;
			skin->m_Y.pos += result.deltaY;
			skin->ApplyZoom(result.zoom, false);
		}
	}
}

void SkinSelectionOverlay::CommitZoomDrag()
{
	if (!m_ZoomDrag) return;

	if (m_ZoomDrag->HasMoved())
	{
		m_Skin->WriteOptions(Skin::OPTION_ZOOM);

		if (m_ZoomDrag->HasPositionChanged())
		{
			m_Skin->SavePositionIfAppropriate();
		}
	}

	m_ZoomDrag.reset();

	if (GetCapture() == m_Window)
	{
		ReleaseCapture();
	}
}

LRESULT SkinSelectionOverlay::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return HitTestZoomDrag({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
}

LRESULT SkinSelectionOverlay::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_Window, &ps);
	RECT rc;
	GetClientRect(m_Window, &rc);
	const COLORREF color = RGB(
		max(0, min(255, (int)roundf(m_Skin->m_SelectedColor.r * 255.0f))),
		max(0, min(255, (int)roundf(m_Skin->m_SelectedColor.g * 255.0f))),
		max(0, min(255, (int)roundf(m_Skin->m_SelectedColor.b * 255.0f))));
	HBRUSH brush = CreateSolidBrush(color);
	FillRect(hdc, &rc, brush);
	DeleteObject(brush);
	EndPaint(m_Window, &ps);
	return 0;
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

LRESULT SkinSelectionOverlay::ForwardMessageToSkin(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SendMessage(m_Skin->m_Window, uMsg, wParam, lParam);
	return 0;
}
