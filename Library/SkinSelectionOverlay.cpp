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
#include "MonitorUtil.h"
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

static void DrawHorizontalDash(DWORD* pixels, int width, int height, int y, DWORD color)
{
	for (int x = 0; x < width; x += g_DashLength + g_DashGap)
	{
		for (int dashX = x; dashX < min(width, x + g_DashLength); ++dashX)
		{
			for (int offset = 0; offset < min(g_DashThickness, height); ++offset)
			{
				pixels[(y + offset) * width + dashX] = color;
			}
		}
	}
}

static void DrawVerticalDash(DWORD* pixels, int width, int height, int x, DWORD color)
{
	for (int y = 0; y < height; y += g_DashLength + g_DashGap)
	{
		for (int dashY = y; dashY < min(height, y + g_DashLength); ++dashY)
		{
			for (int offset = 0; offset < min(g_DashThickness, width); ++offset)
			{
				pixels[dashY * width + x + offset] = color;
			}
		}
	}
}

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
	const DWORD fillColor = MakePixel(m_Skin->m_SelectedColor, ColorToByte(m_Skin->m_SelectedColor.a));
	std::fill(pixels, pixels + (width * height), fillColor);

	const DWORD dashColor = MakePixel(D2D1::ColorF(D2D1::ColorF::DarkSlateGray), 255);
	DrawHorizontalDash(pixels, width, height, 0, dashColor);
	DrawHorizontalDash(pixels, width, height, max(0, height - g_DashThickness), dashColor);
	DrawVerticalDash(pixels, width, height, 0, dashColor);
	DrawVerticalDash(pixels, width, height, max(0, width - g_DashThickness), dashColor);

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
	BeginPaint(m_Window, &ps);
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
	ApplyCommandToSelection(LOWORD(wParam));
	return 0;
}

void SkinSelectionOverlay::ApplyCommandToSelection(UINT_PTR command)
{
	if (command == IDM_SKIN_SELECTALL)
	{
		for (const auto& skins : GetRainmeter().GetAllSkins())
		{
			skins.second->Select();
		}
		return;
	}

	std::vector<Skin*> selectedSkins;
	for (const auto& skins : GetRainmeter().GetAllSkins())
	{
		Skin* skin = skins.second;
		if (skin->IsSelected())
		{
			selectedSkins.push_back(skin);
		}
	}

	if (selectedSkins.empty()) return;

	auto allMatch = [&](auto getter)
	{
		const auto value = getter(selectedSkins[0]);
		for (size_t i = 1; i < selectedSkins.size(); ++i)
		{
			if (getter(selectedSkins[i]) != value)
			{
				return false;
			}
		}
		return true;
	};

	const bool boolValue =
		command == IDM_SKIN_FROMRIGHT ? !(allMatch([](Skin* skin) { return skin->m_X.fromOpposite; }) && selectedSkins[0]->m_X.fromOpposite) :
		command == IDM_SKIN_FROMBOTTOM ? !(allMatch([](Skin* skin) { return skin->m_Y.fromOpposite; }) && selectedSkins[0]->m_Y.fromOpposite) :
		command == IDM_SKIN_XPERCENTAGE ? !(allMatch([](Skin* skin) { return skin->m_X.percentage; }) && selectedSkins[0]->m_X.percentage) :
		command == IDM_SKIN_YPERCENTAGE ? !(allMatch([](Skin* skin) { return skin->m_Y.percentage; }) && selectedSkins[0]->m_Y.percentage) :
		command == IDM_SKIN_MONITOR_AUTOSELECT ? !(allMatch([](Skin* skin) { return skin->m_AutoSelectScreen; }) && selectedSkins[0]->m_AutoSelectScreen) :
		command == IDM_SKIN_CLICKTHROUGH ? !(allMatch([](Skin* skin) { return skin->m_OldClickThrough; }) && selectedSkins[0]->m_OldClickThrough) :
		command == IDM_SKIN_DRAGGABLE ? !(allMatch([](Skin* skin) { return skin->m_OldWindowDraggable; }) && selectedSkins[0]->m_OldWindowDraggable) :
		command == IDM_SKIN_KEEPONSCREEN ? !(allMatch([](Skin* skin) { return skin->m_OldKeepOnScreen; }) && selectedSkins[0]->m_OldKeepOnScreen) :
		command == IDM_SKIN_REMEMBERPOSITION ? !(allMatch([](Skin* skin) { return skin->m_SavePosition; }) && selectedSkins[0]->m_SavePosition) :
		command == IDM_SKIN_SNAPTOEDGES ? !(allMatch([](Skin* skin) { return skin->m_SnapEdges; }) && selectedSkins[0]->m_SnapEdges) :
		command == IDM_SKIN_FAVORITE ? !(allMatch([](Skin* skin) { return skin->m_Favorite; }) && selectedSkins[0]->m_Favorite) :
		false;

	for (Skin* skin : selectedSkins)
	{
		switch (command)
		{
		case IDM_SKIN_REFRESH:
			skin->Refresh(false);
			break;

		case IDM_CLOSESKIN:
			if (skin->m_State != Skin::STATE_CLOSING)
			{
				GetRainmeter().DeactivateSkin(skin, -1);
			}
			break;

		case IDM_SKIN_VERYTOPMOST:
			skin->SetWindowZPosition(ZPOSITION_ONTOPMOST);
			break;

		case IDM_SKIN_TOPMOST:
			skin->SetWindowZPosition(ZPOSITION_ONTOP);
			break;

		case IDM_SKIN_NORMAL:
			skin->SetWindowZPosition(ZPOSITION_NORMAL);
			break;

		case IDM_SKIN_BOTTOM:
			skin->SetWindowZPosition(ZPOSITION_ONBOTTOM);
			break;

		case IDM_SKIN_ONDESKTOP:
			skin->SetWindowZPosition(ZPOSITION_ONDESKTOP);
			break;

		case IDM_SKIN_FROMRIGHT:
			skin->m_X.fromOpposite = boolValue;
			skin->SavePositionIfAppropriate();
			break;

		case IDM_SKIN_FROMBOTTOM:
			skin->m_Y.fromOpposite = boolValue;
			skin->SavePositionIfAppropriate();
			break;

		case IDM_SKIN_XPERCENTAGE:
			skin->m_X.percentage = boolValue;
			skin->SavePositionIfAppropriate();
			break;

		case IDM_SKIN_YPERCENTAGE:
			skin->m_Y.percentage = boolValue;
			skin->SavePositionIfAppropriate();
			break;

		case IDM_SKIN_MONITOR_AUTOSELECT:
			skin->m_AutoSelectScreen = boolValue;
			skin->WriteOptions(Skin::OPTION_POSITION | Skin::OPTION_AUTOSELECTSCREEN);
			break;

		case IDM_SKIN_CLICKTHROUGH:
			skin->SetClickThrough(boolValue);
			skin->m_OldClickThrough = boolValue;
			break;

		case IDM_SKIN_DRAGGABLE:
			skin->SetWindowDraggable(boolValue);
			skin->m_OldWindowDraggable = boolValue;
			break;

		case IDM_SKIN_KEEPONSCREEN:
			skin->SetKeepOnScreen(boolValue);
			skin->m_OldKeepOnScreen = boolValue;
			break;

		case IDM_SKIN_REMEMBERPOSITION:
			skin->SetSavePosition(boolValue);
			break;

		case IDM_SKIN_SNAPTOEDGES:
			skin->SetSnapEdges(boolValue);
			break;

		case IDM_SKIN_FAVORITE:
			skin->SetFavorite(boolValue);
			break;

		case IDM_SKIN_HIDEONMOUSE_NONE:
			skin->SetWindowHide(HIDEMODE_NONE);
			break;

		case IDM_SKIN_HIDEONMOUSE:
			skin->SetWindowHide(HIDEMODE_HIDE);
			break;

		case IDM_SKIN_TRANSPARENCY_FADEIN:
			skin->SetWindowHide(HIDEMODE_FADEIN);
			break;

		case IDM_SKIN_TRANSPARENCY_FADEOUT:
			skin->SetWindowHide(HIDEMODE_FADEOUT);
			break;

		default:
			if (command >= IDM_SKIN_TRANSPARENCY_0 && command <= IDM_SKIN_TRANSPARENCY_100)
			{
				skin->m_AlphaValue = command == IDM_SKIN_TRANSPARENCY_100 ?
					1 :
					(int)(255.0 - (command - IDM_SKIN_TRANSPARENCY_0) * (230.0 / (IDM_SKIN_TRANSPARENCY_90 - IDM_SKIN_TRANSPARENCY_0)));

				skin->UpdateWindowTransparency(skin->m_AlphaValue);
				skin->WriteOptions(Skin::OPTION_ALPHAVALUE);
			}
			else if (command >= IDM_SKIN_ZOOM_80 && command <= IDM_SKIN_ZOOM_150)
			{
				static const float c_Zooms[] = { 0.8f, 0.9f, 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f };
				skin->SetZoom(c_Zooms[command - IDM_SKIN_ZOOM_80]);
			}
			else if (command == IDM_SKIN_MONITOR_PRIMARY || command >= ID_MONITOR_FIRST && command <= ID_MONITOR_LAST)
			{
				const auto& monitorsInfo = MonitorUtil::GetMultiMonitorInfo();
				const std::vector<MonitorInfo>& monitors = monitorsInfo.monitors;

				int monitor = 0;
				bool monitorDefined = false;
				if (command == IDM_SKIN_MONITOR_PRIMARY)
				{
					monitor = monitorsInfo.primary;
				}
				else
				{
					monitor = (command & 0x0ffff) - ID_MONITOR_FIRST;
					monitorDefined = true;
				}

				const int monitorIndex = monitor - 1;
				if (monitor >= 0 && (monitor == 0 || monitor <= (int)monitors.size() && monitors[monitorIndex].active))
				{
					skin->m_AutoSelectScreen = false;
					skin->m_X.monitor = skin->m_Y.monitor = monitorDefined ? std::optional<int>{ monitor } : std::nullopt;
					skin->WriteOptions(Skin::OPTION_POSITION | Skin::OPTION_AUTOSELECTSCREEN);
				}
			}
			break;
		}
	}
}

LRESULT SkinSelectionOverlay::ForwardMessageToSkin(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SendMessage(m_Skin->m_Window, uMsg, wParam, lParam);
	return 0;
}
