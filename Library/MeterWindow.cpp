/*
  Copyright (C) 2001 Kimmo Pekkola

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "MeterWindow.h"
#include "Rainmeter.h"
#include "TrayWindow.h"
#include "System.h"
#include "Error.h"
#include "Meter.h"
#include "Measure.h"
#include "DialogAbout.h"
#include "DialogManage.h"
#include "resource.h"
#include "Litestep.h"
#include "MeasureCalc.h"
#include "MeasureNet.h"
#include "MeasurePlugin.h"
#include "MeterButton.h"
#include "MeterString.h"
#include "TintedImage.h"
#include "MeasureScript.h"
#include "../Version.h"
#include "../Common/PathUtil.h"
#include "../Common/Gfx/Canvas.h"

using namespace Gdiplus;

#define SNAPDISTANCE 10

#define ZPOS_FLAGS	(SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING)

enum TIMER
{
	TIMER_METER      = 1,
	TIMER_MOUSE      = 2,
	TIMER_FADE       = 3,
	TIMER_TRANSITION = 4,
	TIMER_DEACTIVATE = 5
};
enum INTERVAL
{
	INTERVAL_METER      = 1000,
	INTERVAL_MOUSE      = 500,
	INTERVAL_FADE       = 10,
	INTERVAL_TRANSITION = 100
};

int MeterWindow::c_InstanceCount = 0;

HINSTANCE MeterWindow::c_DwmInstance = nullptr;
FPDWMENABLEBLURBEHINDWINDOW MeterWindow::c_DwmEnableBlurBehindWindow = nullptr;
FPDWMGETCOLORIZATIONCOLOR MeterWindow::c_DwmGetColorizationColor = nullptr;
FPDWMSETWINDOWATTRIBUTE MeterWindow::c_DwmSetWindowAttribute = nullptr;
FPDWMISCOMPOSITIONENABLED MeterWindow::c_DwmIsCompositionEnabled = nullptr;

/*
** Constructor
**
*/
MeterWindow::MeterWindow(const std::wstring& folderPath, const std::wstring& file) : m_FolderPath(folderPath), m_FileName(file),
	m_Canvas(),
	m_Background(),
	m_BackgroundSize(),
	m_Window(),
	m_Mouse(this),
	m_MouseOver(false),
	m_MouseInputRegistered(false),
	m_HasMouseScrollAction(false),
	m_BackgroundMargins(),
	m_DragMargins(),
	m_WindowX(1, L'0'),
	m_WindowY(1, L'0'),
	m_WindowXScreen(1),
	m_WindowYScreen(1),
	m_WindowXScreenDefined(false),
	m_WindowYScreenDefined(false),
	m_WindowXFromRight(false),
	m_WindowYFromBottom(false),
	m_WindowXPercentage(false),
	m_WindowYPercentage(false),
	m_WindowW(),
	m_WindowH(),
	m_ScreenX(),
	m_ScreenY(),
	m_AnchorXFromRight(false),
	m_AnchorYFromBottom(false),
	m_AnchorXPercentage(false),
	m_AnchorYPercentage(false),
	m_AnchorScreenX(),
	m_AnchorScreenY(),
	m_WindowDraggable(true),
	m_WindowUpdate(INTERVAL_METER),
	m_TransitionUpdate(INTERVAL_TRANSITION),
	m_ActiveTransition(false),
	m_HasNetMeasures(false),
	m_HasButtons(false),
	m_WindowHide(HIDEMODE_NONE),
	m_WindowStartHidden(false),
	m_SavePosition(false),			// Must be false
	m_SnapEdges(true),
	m_AlphaValue(255),
	m_FadeDuration(250),
	m_WindowZPosition(ZPOSITION_NORMAL),
	m_DynamicWindowSize(false),
	m_ClickThrough(false),
	m_KeepOnScreen(true),
	m_AutoSelectScreen(false),
	m_Dragging(false),
	m_Dragged(false),
	m_BackgroundMode(BGMODE_IMAGE),
	m_SolidAngle(),
	m_SolidBevel(BEVELTYPE_NONE),
	m_Blur(false),
	m_BlurMode(BLURMODE_NONE),
	m_BlurRegion(),
	m_FadeStartTime(),
	m_FadeStartValue(),
	m_FadeEndValue(),
	m_TransparencyValue(),
	m_State(STATE_INITIALIZING),
	m_Hidden(false),
	m_ResizeWindow(RESIZEMODE_NONE),
	m_UpdateCounter(),
	m_MouseMoveCounter(),
	m_FontCollection(),
	m_ToolTipHidden(false)
{
	if (!c_DwmInstance && Platform::IsAtLeastWinVista())
	{
		c_DwmInstance = System::RmLoadLibrary(L"dwmapi.dll");
		if (c_DwmInstance)
		{
			c_DwmEnableBlurBehindWindow = (FPDWMENABLEBLURBEHINDWINDOW)GetProcAddress(c_DwmInstance, "DwmEnableBlurBehindWindow");
			c_DwmGetColorizationColor = (FPDWMGETCOLORIZATIONCOLOR)GetProcAddress(c_DwmInstance, "DwmGetColorizationColor");
			c_DwmSetWindowAttribute = (FPDWMSETWINDOWATTRIBUTE)GetProcAddress(c_DwmInstance, "DwmSetWindowAttribute");
			c_DwmIsCompositionEnabled = (FPDWMISCOMPOSITIONENABLED)GetProcAddress(c_DwmInstance, "DwmIsCompositionEnabled");
		}
	}

	if (c_InstanceCount == 0)
	{
		WNDCLASSEX wc = {sizeof(WNDCLASSEX)};
		wc.style = CS_NOCLOSE | CS_DBLCLKS;
		wc.lpfnWndProc = InitialWndProc;
		wc.hInstance = GetRainmeter().GetModuleInstance();
		wc.hCursor = nullptr;  // The cursor should be controlled by using SetCursor() when needed.
		wc.lpszClassName = METERWINDOW_CLASS_NAME;
		RegisterClassEx(&wc);
	}

	++c_InstanceCount;
}

/*
** Destructor
**
*/
MeterWindow::~MeterWindow()
{
	m_State = STATE_CLOSING;

	if (!m_OnCloseAction.empty())
	{
		GetRainmeter().ExecuteCommand(m_OnCloseAction.c_str(), this);
	}

	Dispose(false);

	--c_InstanceCount;

	if (c_InstanceCount == 0)
	{
		UnregisterClass(METERWINDOW_CLASS_NAME, GetRainmeter().GetModuleInstance());

		if (c_DwmInstance)
		{
			FreeLibrary(c_DwmInstance);
			c_DwmInstance = nullptr;

			c_DwmEnableBlurBehindWindow = nullptr;
			c_DwmGetColorizationColor = nullptr;
			c_DwmSetWindowAttribute = nullptr;
			c_DwmIsCompositionEnabled = nullptr;
		}
	}
}

/*
** Kills timers/hooks and disposes buffers
**
*/
void MeterWindow::Dispose(bool refresh)
{
	// Kill the timer/hook
	KillTimer(m_Window, TIMER_METER);
	KillTimer(m_Window, TIMER_MOUSE);
	KillTimer(m_Window, TIMER_FADE);
	KillTimer(m_Window, TIMER_TRANSITION);

	m_FadeStartTime = 0;

	UnregisterMouseInput();
	m_HasMouseScrollAction = false;

	m_ActiveTransition = false;

	m_MouseOver = false;
	SetMouseLeaveEvent(true);

	// Destroy the meters
	for (auto j = m_Meters.begin(); j != m_Meters.end(); ++j)
	{
		delete (*j);
	}
	m_Meters.clear();

	// Destroy the measures
	for (auto i = m_Measures.begin(); i != m_Measures.end(); ++i)
	{
		delete (*i);
	}
	m_Measures.clear();

	delete m_Background;
	m_Background = nullptr;

	m_BackgroundSize.cx = m_BackgroundSize.cy = 0;
	m_BackgroundName.clear();

	if (m_BlurRegion)
	{
		DeleteObject(m_BlurRegion);
		m_BlurRegion = nullptr;
	}

	if (m_FontCollection)
	{
		delete m_FontCollection;
		m_FontCollection = nullptr;
	}

	if (!refresh)
	{
		if (m_Window)
		{
			DestroyWindow(m_Window);
			m_Window = nullptr;
		}
	}

	delete m_Canvas;
	m_Canvas = nullptr;
}

/*
** Initializes the window, creates the class and the window.
**
*/
void MeterWindow::Initialize()
{
	m_Window = CreateWindowEx(
		WS_EX_TOOLWINDOW,
		METERWINDOW_CLASS_NAME,
		nullptr,
		WS_POPUP,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		nullptr,
		nullptr,
		GetRainmeter().GetModuleInstance(),
		this);

	setlocale(LC_NUMERIC, "C");

	// Mark the window to ignore the Aero peek
	IgnoreAeroPeek();

	Refresh(true, true);
	if (!m_WindowStartHidden)
	{
		if (m_WindowHide == HIDEMODE_FADEOUT)
		{
			FadeWindow(0, 255);
		}
		else
		{
			FadeWindow(0, m_AlphaValue);
		}
	}
}

/*
** Excludes this window from the Aero Peek.
**
*/
void MeterWindow::IgnoreAeroPeek()
{
	if (c_DwmSetWindowAttribute)
	{
		BOOL bValue = TRUE;
		c_DwmSetWindowAttribute(m_Window, DWMWA_EXCLUDED_FROM_PEEK, &bValue, sizeof(bValue));
	}
}

/*
** Registers to receive WM_INPUT for the mouse events.
**
*/
void MeterWindow::RegisterMouseInput()
{
	if (!m_MouseInputRegistered && m_HasMouseScrollAction)
	{
		RAWINPUTDEVICE rid;
		rid.usUsagePage = 0x01;
		rid.usUsage = 0x02;  // HID mouse
		rid.dwFlags = RIDEV_INPUTSINK;
		rid.hwndTarget = m_Window;
		if (RegisterRawInputDevices(&rid, 1, sizeof(rid)))
		{
			m_MouseInputRegistered = true;
		}
	}
}

void MeterWindow::UnregisterMouseInput()
{
	if (m_MouseInputRegistered)
	{
		RAWINPUTDEVICE rid;
		rid.usUsagePage = 0x01;
		rid.usUsage = 0x02;  // HID mouse
		rid.dwFlags = RIDEV_REMOVE;
		rid.hwndTarget = m_Window;
		RegisterRawInputDevices(&rid, 1, sizeof(rid));
		m_MouseInputRegistered = false;
	}
}

void MeterWindow::AddWindowExStyle(LONG_PTR flag)
{
	LONG_PTR style = GetWindowLongPtr(m_Window, GWL_EXSTYLE);
	if ((style & flag) == 0)
	{
		SetWindowLongPtr(m_Window, GWL_EXSTYLE, style | flag);
	}
}

void MeterWindow::RemoveWindowExStyle(LONG_PTR flag)
{
	LONG_PTR style = GetWindowLongPtr(m_Window, GWL_EXSTYLE);
	if ((style & flag) != 0)
	{
		SetWindowLongPtr(m_Window, GWL_EXSTYLE, style & ~flag);
	}
}

/*
** Unloads the skin with delay to avoid crash (and for fade to complete).
**
*/
void MeterWindow::Deactivate()
{
	if (m_State == STATE_CLOSING) return;
	m_State = STATE_CLOSING;

	GetRainmeter().RemoveMeterWindow(this);
	GetRainmeter().AddUnmanagedMeterWindow(this);

	HideFade();
	SetTimer(m_Window, TIMER_DEACTIVATE, m_FadeDuration + 50, nullptr);
}

/*
** Rebuilds the skin.
**
*/
void MeterWindow::Refresh(bool init, bool all)
{
	if (m_State == STATE_CLOSING) return;
	m_State = STATE_REFRESHING;

	GetRainmeter().SetCurrentParser(&m_Parser);
	
	LogNoticeF(this, L"Refreshing skin");

	SetResizeWindowMode(RESIZEMODE_RESET);

	if (!init)
	{
		Dispose(true);
	}

	ZPOSITION oldZPos = m_WindowZPosition;

	if (!ReadSkin())
	{
		GetRainmeter().DeactivateSkin(this, -1);
		return;
	}

	// Remove transparent flag
	RemoveWindowExStyle(WS_EX_TRANSPARENT);

	m_Hidden = m_WindowStartHidden;

	// Set the window region
	UpdateWindow(m_AlphaValue, true);  // Add/Remove layered flag
	Update(true);

	if (m_BlurMode == BLURMODE_NONE)
	{
		HideBlur();
	}
	else
	{
		ShowBlur();
	}

	if (m_KeepOnScreen)
	{
		MapCoordsToScreen(m_ScreenX, m_ScreenY, m_WindowW, m_WindowH);
	}

	SetWindowPos(m_Window, nullptr, m_ScreenX, m_ScreenY, m_WindowW, m_WindowH, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

	ScreenToWindow();

	if (init)
	{
		ChangeSingleZPos(m_WindowZPosition, all);
	}
	else if (all || oldZPos != m_WindowZPosition)
	{
		ChangeZPos(m_WindowZPosition, all);
	}

	// Start the timers
	if (m_WindowUpdate >= 0)
	{
		SetTimer(m_Window, TIMER_METER, m_WindowUpdate, nullptr);
	}

	SetTimer(m_Window, TIMER_MOUSE, INTERVAL_MOUSE, nullptr);

	GetRainmeter().SetCurrentParser(nullptr);

	m_State = STATE_RUNNING;

	if (!m_OnRefreshAction.empty())
	{
		GetRainmeter().ExecuteCommand(m_OnRefreshAction.c_str(), this);
	}
}

void MeterWindow::SetMouseLeaveEvent(bool cancel)
{
	if (!cancel && (!m_MouseOver || m_ClickThrough)) return;

	// Check whether the mouse event is set
	TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT)};
	tme.hwndTrack = m_Window;
	tme.dwFlags = TME_QUERY;

	if (TrackMouseEvent(&tme) != 0)
	{
		if (cancel)
		{
			if (tme.dwFlags == 0) return;
		}
		else
		{
			if (m_WindowDraggable)
			{
				if (tme.dwFlags == (TME_LEAVE | TME_NONCLIENT)) return;
			}
			else
			{
				if (tme.dwFlags == TME_LEAVE) return;
			}
		}
	}

	tme.cbSize = sizeof(TRACKMOUSEEVENT);
	tme.hwndTrack = m_Window;

	// Cancel the mouse event set before
	tme.dwFlags |= TME_CANCEL;
	TrackMouseEvent(&tme);

	if (cancel) return;

	// Set the mouse event
	tme.dwFlags = TME_LEAVE;
	if (m_WindowDraggable)
	{
		tme.dwFlags |= TME_NONCLIENT;
	}
	TrackMouseEvent(&tme);
}

void MeterWindow::MapCoordsToScreen(int& x, int& y, int w, int h)
{
	const size_t numOfMonitors = System::GetMonitorCount();  // intentional
	const std::vector<MonitorInfo>& monitors = System::GetMultiMonitorInfo().monitors;

	// Check that the window is inside the screen area
	POINT pt = {x + w / 2, y + h / 2};
	for (int i = 0; i < 5; ++i)
	{
		switch (i)
		{
		case 0:
			// Use initial value
			break;

		case 1:
			pt.x = x;
			pt.y = y;
			break;

		case 2:
			pt.x = x + w;
			pt.y = y + h;
			break;

		case 3:
			pt.x = x;
			pt.y = y + h;
			break;

		case 4:
			pt.x = x + w;
			pt.y = y;
			break;
		}

		for (auto iter = monitors.cbegin(); iter != monitors.cend(); ++iter)
		{
			if (!(*iter).active) continue;

			const RECT r = (*iter).screen;
			if (pt.x >= r.left && pt.x < r.right && pt.y >= r.top && pt.y < r.bottom)
			{
				x = min(x, r.right - w);
				x = max(x, r.left);
				y = min(y, r.bottom - h);
				y = max(y, r.top);
				return;
			}
		}
	}

	// No monitor found for the window -> Use the default work area
	const RECT r = monitors[System::GetMultiMonitorInfo().primary - 1].work;
	x = min(x, r.right - w);
	x = max(x, r.left);
	y = min(y, r.bottom - h);
	y = max(y, r.top);
}

/*
** Moves the window to a new place (on the virtual screen)
**
*/
void MeterWindow::MoveWindow(int x, int y)
{
	SetWindowPos(m_Window, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

	SavePositionIfAppropriate();
}

/*
** Sets the window's z-position
**
*/
void MeterWindow::ChangeZPos(ZPOSITION zPos, bool all)
{
	HWND winPos = HWND_NOTOPMOST;
	m_WindowZPosition = zPos;

	switch (zPos)
	{
	case ZPOSITION_ONTOPMOST:
	case ZPOSITION_ONTOP:
		winPos = HWND_TOPMOST;
		break;

	case ZPOSITION_ONBOTTOM:
		if (all)
		{
			if (System::GetShowDesktop())
			{
				// Insert after the system window temporarily to keep order
				winPos = System::GetWindow();
			}
			else
			{
				// Insert after the helper window
				winPos = System::GetHelperWindow();
			}
		}
		else
		{
			winPos = HWND_BOTTOM;
		}
		break;

	case ZPOSITION_NORMAL:
		if (all || !GetRainmeter().IsNormalStayDesktop()) break;
	case ZPOSITION_ONDESKTOP:
		if (System::GetShowDesktop())
		{
			winPos = System::GetHelperWindow();

			if (all)
			{
				// Insert after the helper window
			}
			else
			{
				// Find the "backmost" topmost window
				while (winPos = ::GetNextWindow(winPos, GW_HWNDPREV))
				{
					if (GetWindowLongPtr(winPos, GWL_EXSTYLE) & WS_EX_TOPMOST)
					{
						// Insert after the found window
						if (0 != SetWindowPos(m_Window, winPos, 0, 0, 0, 0, ZPOS_FLAGS))
						{
							break;
						}
					}
				}
				return;
			}
		}
		else
		{
			if (all)
			{
				// Insert after the helper window
				winPos = System::GetHelperWindow();
			}
			else
			{
				winPos = HWND_BOTTOM;
			}
		}
		break;
	}

	SetWindowPos(m_Window, winPos, 0, 0, 0, 0, ZPOS_FLAGS);
}

/*
** Sets the window's z-position in proper order.
**
*/
void MeterWindow::ChangeSingleZPos(ZPOSITION zPos, bool all)
{
	if (zPos == ZPOSITION_NORMAL && GetRainmeter().IsNormalStayDesktop() && (!all || System::GetShowDesktop()))
	{
		m_WindowZPosition = zPos;

		// Set window on top of all other ZPOSITION_ONDESKTOP, ZPOSITION_BOTTOM, and ZPOSITION_NORMAL windows
		SetWindowPos(m_Window, System::GetBackmostTopWindow(), 0, 0, 0, 0, ZPOS_FLAGS);

		// Bring window on top of other application windows
		BringWindowToTop(m_Window);
	}
	else
	{
		ChangeZPos(zPos, all);
	}
}

/*
** Runs the bang command with the given arguments.
** Correct number of arguments must be passed (or use Rainmeter::ExecuteBang).
*/
void MeterWindow::DoBang(Bang bang, const std::vector<std::wstring>& args)
{
	switch (bang)
	{
	case Bang::Refresh:
		// Refresh needs to be delayed since it crashes if done during Update()
		PostMessage(m_Window, WM_METERWINDOW_DELAYED_REFRESH, (WPARAM)nullptr, (LPARAM)nullptr);
		break;

	case Bang::Redraw:
		Redraw();
		break;

	case Bang::Update:
		KillTimer(m_Window, TIMER_METER);  // Kill timer temporarily
		Update(false);
		if (m_WindowUpdate >= 0)
		{
			SetTimer(m_Window, TIMER_METER, m_WindowUpdate, nullptr);
		}
		break;

	case Bang::ShowBlur:
		ShowBlur();
		break;

	case Bang::HideBlur:
		HideBlur();
		break;

	case Bang::ToggleBlur:
		DoBang(IsBlur() ? Bang::HideBlur : Bang::ShowBlur, args);
		break;

	case Bang::AddBlur:
		ResizeBlur(args[0], RGN_OR);
		if (IsBlur()) ShowBlur();
		break;

	case Bang::RemoveBlur:
		ResizeBlur(args[0], RGN_DIFF);
		if (IsBlur()) ShowBlur();
		break;

	case Bang::ToggleMeter:
		ToggleMeter(args[0]);
		break;

	case Bang::ShowMeter:
		ShowMeter(args[0]);
		break;

	case Bang::HideMeter:
		HideMeter(args[0]);
		break;

	case Bang::UpdateMeter:
		UpdateMeter(args[0]);
		break;

	case Bang::ToggleMeterGroup:
		ToggleMeter(args[0], true);
		break;

	case Bang::ShowMeterGroup:
		ShowMeter(args[0], true);
		break;

	case Bang::HideMeterGroup:
		HideMeter(args[0], true);
		break;

	case Bang::UpdateMeterGroup:
		UpdateMeter(args[0], true);
		break;

	case Bang::ToggleMeasure:
		ToggleMeasure(args[0]);
		break;

	case Bang::EnableMeasure:
		EnableMeasure(args[0]);
		break;

	case Bang::DisableMeasure:
		DisableMeasure(args[0]);
		break;

	case Bang::PauseMeasure:
		PauseMeasure(args[0]);
		break;

	case Bang::UnpauseMeasure:
		UnpauseMeasure(args[0]);
		break;

	case Bang::TogglePauseMeasure:
		TogglePauseMeasure(args[0]);
		break;

	case Bang::UpdateMeasure:
		UpdateMeasure(args[0]);
		DialogAbout::UpdateMeasures(this);
		break;

	case Bang::DisableMeasureGroup:
		DisableMeasure(args[0], true);
		break;

	case Bang::ToggleMeasureGroup:
		ToggleMeasure(args[0], true);
		break;

	case Bang::EnableMeasureGroup:
		EnableMeasure(args[0], true);
		break;

	case Bang::PauseMeasureGroup:
		PauseMeasure(args[0], true);
		break;

	case Bang::UnpauseMeasureGroup:
		UnpauseMeasure(args[0], true);
		break;

	case Bang::TogglePauseMeasureGroup:
		TogglePauseMeasure(args[0], true);
		break;

	case Bang::UpdateMeasureGroup:
		UpdateMeasure(args[0], true);
		DialogAbout::UpdateMeasures(this);
		break;

	case Bang::Show:
		m_Hidden = false;
		ShowWindow(m_Window, SW_SHOWNOACTIVATE);
		UpdateWindowTransparency((m_WindowHide == HIDEMODE_FADEOUT) ? 255 : m_AlphaValue);
		break;

	case Bang::Hide:
		m_Hidden = true;
		ShowWindow(m_Window, SW_HIDE);
		break;

	case Bang::Toggle:
		DoBang(m_Hidden ? Bang::Show : Bang::Hide, args);
		break;

	case Bang::ShowFade:
		ShowFade();
		break;

	case Bang::HideFade:
		HideFade();
		break;

	case Bang::ToggleFade:
		DoBang(m_Hidden ? Bang::ShowFade : Bang::HideFade, args);
		break;

	case Bang::Move:
		{
			int x = m_Parser.ParseInt(args[0].c_str(), 0);
			int y = m_Parser.ParseInt(args[1].c_str(), 0);
			MoveWindow(x, y);
		}
		break;

	case Bang::ZPos:
		SetWindowZPosition((ZPOSITION)m_Parser.ParseInt(args[0].c_str(), 0));
		break;

	case Bang::ClickThrough:
		{
			int f = m_Parser.ParseInt(args[0].c_str(), 0);
			SetClickThrough((f == -1) ? !m_ClickThrough : f);
		}
		break;

	case Bang::Draggable:
		{
			int f = m_Parser.ParseInt(args[0].c_str(), 0);
			SetWindowDraggable((f == -1) ? !m_WindowDraggable : f);
		}
		break;

	case Bang::SnapEdges:
		{
			int f = m_Parser.ParseInt(args[0].c_str(), 0);
			SetSnapEdges((f == -1) ? !m_SnapEdges : f);
		}
		break;

	case Bang::KeepOnScreen:
		{
			int f = m_Parser.ParseInt(args[0].c_str(), 0);
			SetKeepOnScreen((f == -1) ? !m_KeepOnScreen : f);
		}
		break;

	case Bang::SetTransparency:
		{
			const std::wstring& arg = args[0];
			m_AlphaValue = ConfigParser::ParseInt(arg.c_str(), 255);
			m_AlphaValue = max(m_AlphaValue, 0);
			m_AlphaValue = min(m_AlphaValue, 255);
			UpdateWindowTransparency(m_AlphaValue);
		}
		break;

	case Bang::MoveMeter:
		{
			int x = m_Parser.ParseInt(args[0].c_str(), 0);
			int y = m_Parser.ParseInt(args[1].c_str(), 0);
			MoveMeter(args[2], x, y);
		}
		break;

	case Bang::CommandMeasure:
		{
			const std::wstring& measure = args[0];
			Measure* m = GetMeasure(measure);
			if (m)
			{
				m->Command(args[1]);
			}
			else
			{
				LogWarningF(this, L"!CommandMeasure: [%s] not found", measure.c_str());
			}
		}
		break;

	case Bang::PluginBang:
		{
			std::wstring arg = args[0];
			std::wstring::size_type pos;
			while ((pos = arg.find(L'"')) != std::wstring::npos)
			{
				arg.erase(pos, 1);
			}

			std::wstring measure;
			pos = arg.find(L' ');
			if (pos != std::wstring::npos)
			{
				measure.assign(arg, 0, pos);
				++pos;
			}
			else
			{
				measure = arg;
			}
			arg.erase(0, pos);

			if (!measure.empty())
			{
				Measure* m = GetMeasure(measure);
				if (m)
				{
					m->Command(arg);
					return;
				}

				LogWarningF(this, L"!PluginBang: [%s] not found", measure.c_str());
			}
			else
			{
				LogErrorF(this, L"!PluginBang: Invalid parameters");
			}
		}
		break;

	case Bang::SetVariable:
		SetVariable(args[0], args[1]);
		break;

	case Bang::SetOption:
		SetOption(args[0], args[1], args[2], false);
		break;

	case Bang::SetOptionGroup:
		SetOption(args[0], args[1], args[2], true);
		break;
	}
}

/*
** Enables blurring of the window background (using Aero)
**
*/
void MeterWindow::ShowBlur()
{
	if (c_DwmGetColorizationColor && c_DwmIsCompositionEnabled && c_DwmEnableBlurBehindWindow)
	{
		SetBlur(true);

		// Check that Aero and transparency is enabled
		DWORD color;
		BOOL opaque, enabled;
		if (c_DwmGetColorizationColor(&color, &opaque) != S_OK)
		{
			opaque = TRUE;
		}
		if (c_DwmIsCompositionEnabled(&enabled) != S_OK)
		{
			enabled = FALSE;
		}
		if (opaque || !enabled) return;

		if (m_BlurMode == BLURMODE_FULL)
		{
			if (m_BlurRegion) DeleteObject(m_BlurRegion);
			m_BlurRegion = CreateRectRgn(0, 0, GetW(), GetH());
		}

		BlurBehindWindow(TRUE);
	}
}

/*
** Disables Aero blur
**
*/
void MeterWindow::HideBlur()
{
	if (c_DwmEnableBlurBehindWindow)
	{
		SetBlur(false);

		BlurBehindWindow(FALSE);
	}
}

/*
** Adds to or removes from blur region
**
*/
void MeterWindow::ResizeBlur(const std::wstring& arg, int mode)
{
	if (Platform::IsAtLeastWinVista())
	{
		WCHAR* parseSz = _wcsdup(arg.c_str());
		int type, x, y, w = 0, h = 0;

		WCHAR* token = wcstok(parseSz, L",");
		if (token)
		{
			while (token[0] == L' ') ++token;
			type = m_Parser.ParseInt(token, 0);

			token = wcstok(nullptr, L",");
			if (token)
			{
				while (token[0] == L' ') ++token;
				x = m_Parser.ParseInt(token, 0);

				token = wcstok(nullptr, L",");
				if (token)
				{
					while (token[0] == L' ') ++token;
					y = m_Parser.ParseInt(token, 0);

					token = wcstok(nullptr, L",");
					if (token)
					{
						while (token[0] == L' ') ++token;
						w = m_Parser.ParseInt(token, 0);

						token = wcstok(nullptr, L",");
						if (token)
						{
							while (token[0] == L' ') ++token;
							h = m_Parser.ParseInt(token, 0);
						}
					}
				}
			}
		}

		if (w && h)
		{
			HRGN tempRegion;

			switch (type)
			{
			case 1:
				tempRegion = CreateRectRgn(x, y, w, h);
				break;

			case 2:
				token = wcstok(nullptr, L",");
				if (token)
				{
					while (token[0] == L' ') ++token;
					int r =  m_Parser.ParseInt(token, 0);
					tempRegion = CreateRoundRectRgn(x, y, w, h, r, r);
				}
				break;

			case 3:
				tempRegion = CreateEllipticRgn(x, y, w, h);
				break;
	
			default:  // Unknown type
				free(parseSz);
				return;
			}

			CombineRgn(m_BlurRegion, m_BlurRegion, tempRegion, mode);
			DeleteObject(tempRegion);
		}
		free(parseSz);
	}
}

/*
** Helper function that compares the given name to section's name.
**
*/
bool CompareName(const Section* section, const WCHAR* name, bool group)
{
	return (group) ? section->BelongsToGroup(name) : (_wcsicmp(section->GetName(), name) == 0);
}

/*
** Shows the given meter
**
*/
void MeterWindow::ShowMeter(const std::wstring& name, bool group)
{
	const WCHAR* meter = name.c_str();

	std::vector<Meter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		if (CompareName((*j), meter, group))
		{
			(*j)->Show();
			SetResizeWindowMode(RESIZEMODE_CHECK);	// Need to recalculate the window size
			if (!group) return;
		}
	}

	if (!group) LogErrorF(this, L"!ShowMeter: [%s] not found", meter);
}

/*
** Hides the given meter
**
*/
void MeterWindow::HideMeter(const std::wstring& name, bool group)
{
	const WCHAR* meter = name.c_str();

	std::vector<Meter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		if (CompareName((*j), meter, group))
		{
			(*j)->Hide();
			SetResizeWindowMode(RESIZEMODE_CHECK);	// Need to recalculate the window size
			if (!group) return;
		}
	}

	if (!group) LogErrorF(this, L"!HideMeter: [%s] not found", meter);
}

/*
** Toggles the given meter
**
*/
void MeterWindow::ToggleMeter(const std::wstring& name, bool group)
{
	const WCHAR* meter = name.c_str();

	std::vector<Meter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		if (CompareName((*j), meter, group))
		{
			if ((*j)->IsHidden())
			{
				(*j)->Show();
			}
			else
			{
				(*j)->Hide();
			}
			SetResizeWindowMode(RESIZEMODE_CHECK);	// Need to recalculate the window size
			if (!group) return;
		}
	}

	if (!group) LogErrorF(this, L"!ToggleMeter: [%s] not found", meter);
}

/*
** Moves the given meter
**
*/
void MeterWindow::MoveMeter(const std::wstring& name, int x, int y)
{
	const WCHAR* meter = name.c_str();

	std::vector<Meter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		if (CompareName((*j), meter, false))
		{
			(*j)->SetX(x);
			(*j)->SetY(y);
			SetResizeWindowMode(RESIZEMODE_CHECK);	// Need to recalculate the window size
			return;
		}
	}

	LogErrorF(this, L"!MoveMeter: [%s] not found", meter);
}

/*
** Updates the given meter
**
*/
void MeterWindow::UpdateMeter(const std::wstring& name, bool group)
{
	const WCHAR* meter = name.c_str();
	bool all = false;

	if (!group && meter[0] == L'*' && meter[1] == L'\0')  // Allow [!UpdateMeter *]
	{
		all = true;
		group = true;
	}

	bool bActiveTransition = false;
	bool bContinue = true;
	for (auto j = m_Meters.cbegin(); j != m_Meters.cend(); ++j)
	{
		if (all || (bContinue && CompareName((*j), meter, group)))
		{
			if (UpdateMeter((*j), bActiveTransition, true))
			{
				(*j)->DoUpdateAction();
			}

			SetResizeWindowMode(RESIZEMODE_CHECK);	// Need to recalculate the window size
			if (!group)
			{
				bContinue = false;
				if (bActiveTransition) break;
			}
		}
		else
		{
			// Check for transitions
			if (!bActiveTransition && (*j)->HasActiveTransition())
			{
				bActiveTransition = true;
				if (!group && !bContinue) break;
			}
		}
	}

	// Post-updates
	PostUpdate(bActiveTransition);

	if (!group && bContinue) LogErrorF(this, L"!UpdateMeter: [%s] not found", meter);
}

/*
** Enables the given measure
**
*/
void MeterWindow::EnableMeasure(const std::wstring& name, bool group)
{
	const WCHAR* measure = name.c_str();

	std::vector<Measure*>::const_iterator i = m_Measures.begin();
	for ( ; i != m_Measures.end(); ++i)
	{
		if (CompareName((*i), measure, group))
		{
			(*i)->Enable();
			if (!group) return;
		}
	}

	if (!group) LogErrorF(this, L"!EnableMeasure: [%s] not found", measure);
}

/*
** Disables the given measure
**
*/
void MeterWindow::DisableMeasure(const std::wstring& name, bool group)
{
	const WCHAR* measure = name.c_str();

	std::vector<Measure*>::const_iterator i = m_Measures.begin();
	for ( ; i != m_Measures.end(); ++i)
	{
		if (CompareName((*i), measure, group))
		{
			(*i)->Disable();
			if (!group) return;
		}
	}

	if (!group) LogErrorF(this, L"!DisableMeasure: [%s] not found", measure);
}

/*
** Toggles the given measure
**
*/
void MeterWindow::ToggleMeasure(const std::wstring& name, bool group)
{
	const WCHAR* measure = name.c_str();

	std::vector<Measure*>::const_iterator i = m_Measures.begin();
	for ( ; i != m_Measures.end(); ++i)
	{
		if (CompareName((*i), measure, group))
		{
			if ((*i)->IsDisabled())
			{
				(*i)->Enable();
			}
			else
			{
				(*i)->Disable();
			}
			if (!group) return;
		}
	}

	if (!group) LogErrorF(this, L"!ToggleMeasure: [%s] not found", measure);
}

/*
** Pauses the given measure
**
*/
void MeterWindow::PauseMeasure(const std::wstring& name, bool group)
{
	const WCHAR* measure = name.c_str();

	std::vector<Measure*>::const_iterator i = m_Measures.begin();
	for ( ; i != m_Measures.end(); ++i)
	{
		if (CompareName((*i), measure, group))
		{
			(*i)->Pause();
			if (!group) return;
		}
	}

	if (!group) LogErrorF(this, L"!PauseMeasure: [%s] not found", measure);
}

/*
** Unpauses the given measure
**
*/
void MeterWindow::UnpauseMeasure(const std::wstring& name, bool group)
{
	const WCHAR* measure = name.c_str();

	std::vector<Measure*>::const_iterator i = m_Measures.begin();
	for ( ; i != m_Measures.end(); ++i)
	{
		if (CompareName((*i), measure, group))
		{
			(*i)->Unpause();
			if (!group) return;
		}
	}

	if (!group) LogErrorF(this, L"!UnpauseMeasure: [%s] not found", measure);
}

/*
** Toggles the pause state of the given measure
**
*/
void MeterWindow::TogglePauseMeasure(const std::wstring& name, bool group)
{
	const WCHAR* measure = name.c_str();

	std::vector<Measure*>::const_iterator i = m_Measures.begin();
	for ( ; i != m_Measures.end(); ++i)
	{
		if (CompareName((*i), measure, group))
		{
			if ((*i)->IsPaused())
			{
				(*i)->Unpause();
			}
			else
			{
				(*i)->Pause();
			}
			if (!group) return;
		}
	}

	if (!group) LogErrorF(this, L"!TogglePauseMeasure: [%s] not found", measure);
}

/*
** Updates the given measure
**
*/
void MeterWindow::UpdateMeasure(const std::wstring& name, bool group)
{
	const WCHAR* measure = name.c_str();
	bool all = false;

	if (!group && measure[0] == L'*' && measure[1] == L'\0')  // Allow [!UpdateMeasure *]
	{
		all = true;
		group = true;
	}

	bool bNetStats = m_HasNetMeasures;
	for (auto i = m_Measures.cbegin(); i != m_Measures.cend(); ++i)
	{
		if (all || CompareName((*i), measure, group))
		{
			if (bNetStats && (*i)->GetTypeID() == TypeID<MeasureNet>())
			{
				MeasureNet::UpdateIFTable();
				MeasureNet::UpdateStats();
				bNetStats = false;
			}

			if (UpdateMeasure((*i), true))
			{
				(*i)->DoUpdateAction();
				(*i)->DoChangeAction();
			}

			if (!group) return;
		}
	}

	if (!group) LogErrorF(this, L"!UpdateMeasure: [%s] not found", measure);
}

/*
** Sets variable to given value.
**
*/
void MeterWindow::SetVariable(const std::wstring& variable, const std::wstring& value)
{
	double result;
	if (m_Parser.ParseFormula(value, &result))
	{
		WCHAR buffer[256];
		int len = _snwprintf_s(buffer, _TRUNCATE, L"%.5f", result);
		Measure::RemoveTrailingZero(buffer, len);

		const std::wstring& resultString = buffer;

		m_Parser.SetVariable(variable, resultString);
	}
	else
	{
		m_Parser.SetVariable(variable, value);
	}
}

/*
** Changes the property of a meter or measure.
**
*/
void MeterWindow::SetOption(const std::wstring& section, const std::wstring& option, const std::wstring& value, bool group)
{
	auto setValue = [&](Section* section, const std::wstring& option, const std::wstring& value)
	{
		// Force DynamicVariables temporarily (until next ReadOptions()).
		section->SetDynamicVariables(true);

		if (value.empty())
		{
			m_Parser.DeleteValue(section->GetOriginalName(), option);
		}
		else
		{
			m_Parser.SetValue(section->GetOriginalName(), option, value);
		}
	};

	if (group)
	{
		for (auto j = m_Meters.begin(); j != m_Meters.end(); ++j)
		{
			if ((*j)->BelongsToGroup(section))
			{
				setValue(*j, option, value);
			}
		}

		for (auto i = m_Measures.begin(); i != m_Measures.end(); ++i)
		{
			if ((*i)->BelongsToGroup(section))
			{
				setValue(*i, option, value);
			}
		}
	}
	else
	{
		Meter* meter = GetMeter(section);
		if (meter)
		{
			setValue(meter, option, value);
			return;
		}

		Measure* measure = GetMeasure(section);
		if (measure)
		{
			setValue(measure, option, value);
			return;
		}

		// ContextTitle and ContextAction in [Rainmeter] are dynamic
		if ((_wcsicmp(section.c_str(), L"Rainmeter") == 0) && (wcsnicmp(option.c_str(), L"Context", 7) == 0))
		{
			if (value.empty())
			{
				m_Parser.DeleteValue(section, option);
			}
			else
			{
				m_Parser.SetValue(section, option, value);
			}
		}

		// Is it a style?
	}
}

/*
** Calculates the screen cordinates from the WindowX/Y options
**
*/
void MeterWindow::WindowToScreen()
{
	std::wstring::size_type index, index2;
	int pixel = 0;
	float num;
	int screenx, screeny, screenh, screenw;

	const int numOfMonitors = (int)System::GetMonitorCount();
	const MultiMonitorInfo& monitorsInfo = System::GetMultiMonitorInfo();
	const std::vector<MonitorInfo>& monitors = monitorsInfo.monitors;

	// Clear position flags
	m_WindowXScreen = m_WindowYScreen = monitorsInfo.primary; // Default to primary screen
	m_WindowXScreenDefined = m_WindowYScreenDefined = false;
	m_WindowXFromRight = m_WindowYFromBottom = false; // Default to from left/top
	m_WindowXPercentage = m_WindowYPercentage = false; // Default to pixels
	m_AnchorXFromRight = m_AnchorYFromBottom = false;
	m_AnchorXPercentage = m_AnchorYPercentage = false;

	// --- Calculate AnchorScreenX ---

	index = m_AnchorX.find_first_not_of(L"0123456789.");
	num = (float)_wtof(m_AnchorX.substr(0,index).c_str());
	index = m_AnchorX.find_last_of(L'%');
	if (index != std::wstring::npos) m_AnchorXPercentage = true;
	index = m_AnchorX.find_last_of(L'R');
	if (index != std::wstring::npos) m_AnchorXFromRight = true;
	if (m_AnchorXPercentage) //is a percentage
	{
		pixel = (int)(m_WindowW * num / 100.0f);
	}
	else
	{
		pixel = (int)num;
	}
	if (m_AnchorXFromRight) //measure from right
	{
		pixel = m_WindowW - pixel;
	}
	else
	{
		//pixel = pixel;
	}
	m_AnchorScreenX = pixel;

	// --- Calculate AnchorScreenY ---

	index = m_AnchorY.find_first_not_of(L"0123456789.");
	num = (float)_wtof(m_AnchorY.substr(0,index).c_str());
	index = m_AnchorY.find_last_of(L'%');
	if (index != std::wstring::npos) m_AnchorYPercentage = true;
	index = m_AnchorY.find_last_of(L'R');
	if (index != std::wstring::npos) m_AnchorYFromBottom = true;
	if (m_AnchorYPercentage) //is a percentage
	{
		pixel = (int)(m_WindowH * num / 100.0f);
	}
	else
	{
		pixel = (int)num;
	}
	if (m_AnchorYFromBottom) //measure from bottom
	{
		pixel = m_WindowH - pixel;
	}
	else
	{
		//pixel = pixel;
	}
	m_AnchorScreenY = pixel;

	// --- Calculate ScreenX ---

	index = m_WindowX.find_first_not_of(L"-0123456789.");
	num = (float)_wtof(m_WindowX.substr(0,index).c_str());
	index = m_WindowX.find_last_of(L'%');
	index2 = m_WindowX.find_last_of(L'#');  // for ignoring the non-replaced variables such as "#WORKAREAX@n#"
	if (index != std::wstring::npos && (index2 == std::wstring::npos || index2 < index))
	{
		m_WindowXPercentage = true;
	}
	index = m_WindowX.find_last_of(L'R');
	if (index != std::wstring::npos && (index2 == std::wstring::npos || index2 < index))
	{
		m_WindowXFromRight = true;
	}
	index = m_WindowX.find_last_of(L'@');
	if (index != std::wstring::npos && (index2 == std::wstring::npos || index2 < index))
	{
		index = index + 1;
		index2 = m_WindowX.find_first_not_of(L"0123456789", index);

		std::wstring screenStr = m_WindowX.substr(index, (index2 != std::wstring::npos) ? index2 - index : std::wstring::npos);
		if (!screenStr.empty())
		{
			int screenIndex = _wtoi(screenStr.c_str());
			if (screenIndex >= 0 && (screenIndex == 0 || screenIndex <= numOfMonitors && monitors[screenIndex - 1].active))
			{
				m_WindowXScreen = screenIndex;
				m_WindowXScreenDefined = true;
				m_WindowYScreen = m_WindowXScreen; //Default to X and Y on same screen if not overridden on WindowY
				m_WindowYScreenDefined = true;
			}
		}
	}
	if (m_WindowXScreen == 0)
	{
		screenx = monitorsInfo.vsL;
		screenw = monitorsInfo.vsW;
	}
	else
	{
		screenx = monitors[m_WindowXScreen - 1].screen.left;
		screenw = monitors[m_WindowXScreen - 1].screen.right - monitors[m_WindowXScreen - 1].screen.left;
	}
	if (m_WindowXPercentage) //is a percentage
	{
		pixel = (int)(screenw * num / 100.0f);
	}
	else
	{
		pixel = (int)num;
	}
	if (m_WindowXFromRight) //measure from right
	{
		pixel = screenx + (screenw - pixel);
	}
	else
	{
		pixel = screenx + pixel;
	}
	m_ScreenX = pixel - m_AnchorScreenX;

	// --- Calculate ScreenY ---

	index = m_WindowY.find_first_not_of(L"-0123456789.");
	num = (float)_wtof(m_WindowY.substr(0,index).c_str());
	index = m_WindowY.find_last_of(L'%');
	index2 = m_WindowX.find_last_of(L'#');  // for ignoring the non-replaced variables such as "#WORKAREAY@n#"
	if (index != std::wstring::npos && (index2 == std::wstring::npos || index2 < index))
	{
		m_WindowYPercentage = true;
	}
	index = m_WindowY.find_last_of(L'B');
	if (index != std::wstring::npos && (index2 == std::wstring::npos || index2 < index))
	{
		m_WindowYFromBottom = true;
	}
	index = m_WindowY.find_last_of(L'@');
	if (index != std::wstring::npos && (index2 == std::wstring::npos || index2 < index))
	{
		index = index + 1;
		index2 = m_WindowY.find_first_not_of(L"0123456789", index);

		std::wstring screenStr = m_WindowY.substr(index, (index2 != std::wstring::npos) ? index2 - index : std::wstring::npos);
		if (!screenStr.empty())
		{
			int screenIndex = _wtoi(screenStr.c_str());
			if (screenIndex >= 0 && (screenIndex == 0 || screenIndex <= numOfMonitors && monitors[screenIndex - 1].active))
			{
				m_WindowYScreen = screenIndex;
				m_WindowYScreenDefined = true;
			}
		}
	}
	if (m_WindowYScreen == 0)
	{
		screeny = monitorsInfo.vsT;
		screenh = monitorsInfo.vsH;
	}
	else
	{
		screeny = monitors[m_WindowYScreen - 1].screen.top;
		screenh = monitors[m_WindowYScreen - 1].screen.bottom - monitors[m_WindowYScreen - 1].screen.top;
	}
	if (m_WindowYPercentage) //is a percentage
	{
		pixel = (int)(screenh * num / 100.0f);
	}
	else
	{
		pixel = (int)num;
	}
	if (m_WindowYFromBottom) //measure from right
	{
		pixel = screeny + (screenh - pixel);
	}
	else
	{
		pixel = screeny + pixel;
	}
	m_ScreenY = pixel - m_AnchorScreenY;
}

/* ScreenToWindow
**
** Calculates the WindowX/Y cordinates from the ScreenX/Y
**
*/
void MeterWindow::ScreenToWindow()
{
	WCHAR buffer[256];
	int pixel = 0;
	float num;
	int screenx, screeny, screenh, screenw;

	const size_t numOfMonitors = System::GetMonitorCount();
	const MultiMonitorInfo& monitorsInfo = System::GetMultiMonitorInfo();
	const std::vector<MonitorInfo>& monitors = monitorsInfo.monitors;

	// Correct to auto-selected screen
	if (m_AutoSelectScreen)
	{
		RECT rect = {m_ScreenX, m_ScreenY, m_ScreenX + m_WindowW, m_ScreenY + m_WindowH};
		HMONITOR hMonitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);

		if (hMonitor != nullptr)
		{
			int screenIndex = 1;
			for (auto iter = monitors.cbegin(); iter != monitors.cend(); ++iter, ++screenIndex)
			{
				if ((*iter).active && (*iter).handle == hMonitor)
				{
					bool reset = (!m_WindowXScreenDefined || !m_WindowYScreenDefined ||
						m_WindowXScreen != screenIndex || m_WindowYScreen != screenIndex);

					m_WindowXScreen = m_WindowYScreen = screenIndex;
					m_WindowXScreenDefined = m_WindowYScreenDefined = true;

					if (reset)
					{
						m_Parser.ResetMonitorVariables(this);  // Set present monitor variables
					}
					break;
				}
			}
		}
	}

	// --- Calculate WindowX ---

	if (m_WindowXScreen == 0)
	{
		screenx = monitorsInfo.vsL;
		screenw = monitorsInfo.vsW;
	}
	else
	{
		screenx = monitors[m_WindowXScreen - 1].screen.left;
		screenw = monitors[m_WindowXScreen - 1].screen.right - monitors[m_WindowXScreen - 1].screen.left;
	}
	if (m_WindowXFromRight)
	{
		pixel = (screenx + screenw) - m_ScreenX;
		pixel -= m_AnchorScreenX;
	}
	else
	{
		pixel = m_ScreenX - screenx;
		pixel += m_AnchorScreenX;
	}
	if (m_WindowXPercentage)
	{
		num = 100.0f * (float)pixel / (float)screenw;
		_snwprintf_s(buffer, _TRUNCATE, L"%.5f%%", num);
	}
	else
	{
		_itow_s(pixel, buffer, 10);
	}
	if (m_WindowXFromRight)
	{
		wcscat_s(buffer, L"R");
	}
	if (m_WindowXScreenDefined)
	{
		_snwprintf_s(buffer, _TRUNCATE, L"%s@%i", buffer, m_WindowXScreen);
	}
	m_WindowX = buffer;

	// --- Calculate WindowY ---

	if (m_WindowYScreen == 0)
	{
		screeny = monitorsInfo.vsT;
		screenh = monitorsInfo.vsH;
	}
	else
	{
		screeny = monitors[m_WindowYScreen - 1].screen.top;
		screenh = monitors[m_WindowYScreen - 1].screen.bottom - monitors[m_WindowYScreen - 1].screen.top;
	}
	if (m_WindowYFromBottom)
	{
		pixel = (screeny + screenh) - m_ScreenY;
		pixel -= m_AnchorScreenY;
	}
	else
	{
		pixel = m_ScreenY - screeny;
		pixel += m_AnchorScreenY;
	}
	if (m_WindowYPercentage)
	{
		num = 100.0f * (float)pixel / (float)screenh;
		_snwprintf_s(buffer, _TRUNCATE, L"%.5f%%", num);
	}
	else
	{
		_itow_s(pixel, buffer, 10);
	}
	if (m_WindowYFromBottom)
	{
		wcscat_s(buffer, L"B");
	}
	if (m_WindowYScreenDefined)
	{
		_snwprintf_s(buffer, _TRUNCATE, L"%s@%i", buffer, m_WindowYScreen);
	}
	m_WindowY = buffer;
}

/*
** Reads the skin options from Rainmeter.ini
**
*/
void MeterWindow::ReadOptions()
{
	WCHAR buffer[32];

	const WCHAR* section = m_FolderPath.c_str();
	ConfigParser parser;
	parser.Initialize(GetRainmeter().GetIniFile(), nullptr, section);

	INT writeFlags = 0;
	auto addWriteFlag = [&](INT flag)
	{
		if (parser.GetLastDefaultUsed())
		{
			writeFlags |= flag;
		}
	};

	// Check if the window position should be read as a formula
	double value;
	m_WindowX = parser.ReadString(section, L"WindowX", L"0");
	addWriteFlag(OPTION_POSITION);
	if (parser.ParseFormula(m_WindowX, &value))
	{
		_itow_s((int)value, buffer, 10);
		m_WindowX = buffer;
	}
	m_WindowY = parser.ReadString(section, L"WindowY", L"0");
	addWriteFlag(OPTION_POSITION);
	if (parser.ParseFormula(m_WindowY, &value))
	{
		_itow_s((int)value, buffer, 10);
		m_WindowY = buffer;
	}

	m_AnchorX = parser.ReadString(section, L"AnchorX", L"0");
	m_AnchorY = parser.ReadString(section, L"AnchorY", L"0");

	int zPos = parser.ReadInt(section, L"AlwaysOnTop", ZPOSITION_NORMAL);
	addWriteFlag(OPTION_ALWAYSONTOP);
	m_WindowZPosition = (zPos >= ZPOSITION_ONDESKTOP && zPos <= ZPOSITION_ONTOPMOST) ? (ZPOSITION)zPos : ZPOSITION_NORMAL;

	int hideMode = parser.ReadInt(section, L"HideOnMouseOver", HIDEMODE_NONE);
	m_WindowHide = (hideMode >= HIDEMODE_NONE && hideMode <= HIDEMODE_FADEOUT) ? (HIDEMODE)hideMode : HIDEMODE_NONE;

	m_WindowDraggable = parser.ReadBool(section, L"Draggable", true);
	addWriteFlag(OPTION_DRAGGABLE);

	m_SnapEdges = parser.ReadBool(section, L"SnapEdges", true);
	addWriteFlag(OPTION_SNAPEDGES);

	m_ClickThrough = parser.ReadBool(section, L"ClickThrough", false);
	addWriteFlag(OPTION_CLICKTHROUGH);

	m_KeepOnScreen = parser.ReadBool(section, L"KeepOnScreen", true);
	addWriteFlag(OPTION_KEEPONSCREEN);

	m_SavePosition = parser.ReadBool(section, L"SavePosition", true);
	m_WindowStartHidden = parser.ReadBool(section, L"StartHidden", false);
	m_AutoSelectScreen = parser.ReadBool(section, L"AutoSelectScreen", false);

	m_AlphaValue = parser.ReadInt(section, L"AlphaValue", 255);
	m_AlphaValue = max(m_AlphaValue, 0);
	m_AlphaValue = min(m_AlphaValue, 255);

	m_FadeDuration = parser.ReadInt(section, L"FadeDuration", 250);

	std::wstring skinGroup = parser.ReadString(section, L"Group", L"");
	const std::wstring& group = m_Parser.ReadString(L"Rainmeter", L"Group", L"");
	if (!group.empty())
	{
		skinGroup += L'|';
		skinGroup += group;
	}
	InitializeGroup(skinGroup);

	if (writeFlags != 0)
	{
		WriteOptions(writeFlags);
	}

	// Set WindowXScreen/WindowYScreen temporarily
	WindowToScreen();
}

/*
** Writes the specified options to Rainmeter.ini
**
*/
void MeterWindow::WriteOptions(INT setting)
{
	const WCHAR* iniFile = GetRainmeter().GetIniFile().c_str();

	if (*iniFile)
	{
		WCHAR buffer[32];
		const WCHAR* section = m_FolderPath.c_str();

		if (setting != OPTION_ALL)
		{
			DialogManage::UpdateSkins(this);
		}

		if (setting & OPTION_POSITION)
		{
			ScreenToWindow();

			// If position needs to be save, do so.
			if (m_SavePosition)
			{
				WritePrivateProfileString(section, L"WindowX", m_WindowX.c_str(), iniFile);
				WritePrivateProfileString(section, L"WindowY", m_WindowY.c_str(), iniFile);
			}

			if (setting == OPTION_POSITION) return;
		}

		if (setting & OPTION_ALPHAVALUE)
		{
			_itow_s(m_AlphaValue, buffer, 10);
			WritePrivateProfileString(section, L"AlphaValue", buffer, iniFile);
		}

		if (setting & OPTION_FADEDURATION)
		{
			_itow_s(m_FadeDuration, buffer, 10);
			WritePrivateProfileString(section, L"FadeDuration", buffer, iniFile);
		}

		if (setting & OPTION_CLICKTHROUGH)
		{
			WritePrivateProfileString(section, L"ClickThrough", m_ClickThrough ? L"1" : L"0", iniFile);
		}

		if (setting & OPTION_DRAGGABLE)
		{
			WritePrivateProfileString(section, L"Draggable", m_WindowDraggable ? L"1" : L"0", iniFile);
		}

		if (setting & OPTION_HIDEONMOUSEOVER)
		{
			_itow_s(m_WindowHide, buffer, 10);
			WritePrivateProfileString(section, L"HideOnMouseOver", buffer, iniFile);
		}

		if (setting & OPTION_SAVEPOSITION)
		{
			WritePrivateProfileString(section, L"SavePosition", m_SavePosition ? L"1" : L"0", iniFile);
		}

		if (setting & OPTION_SNAPEDGES)
		{
			WritePrivateProfileString(section, L"SnapEdges", m_SnapEdges ? L"1" : L"0", iniFile);
		}

		if (setting & OPTION_KEEPONSCREEN)
		{
			WritePrivateProfileString(section, L"KeepOnScreen", m_KeepOnScreen ? L"1" : L"0", iniFile);
		}

		if (setting & OPTION_AUTOSELECTSCREEN)
		{
			WritePrivateProfileString(section, L"AutoSelectScreen", m_AutoSelectScreen ? L"1" : L"0", iniFile);
		}

		if (setting & OPTION_ALWAYSONTOP)
		{
			_itow_s(m_WindowZPosition, buffer, 10);
			WritePrivateProfileString(section, L"AlwaysOnTop", buffer, iniFile);
		}
	}
}

/*
** Reads the skin file and creates the meters and measures.
**
*/
bool MeterWindow::ReadSkin()
{
	WCHAR buffer[128];

	std::wstring iniFile = GetFilePath();

	// Verify whether the file exists
	if (_waccess(iniFile.c_str(), 0) == -1)
	{
		std::wstring message = GetFormattedString(ID_STR_UNABLETOREFRESHSKIN, m_FolderPath.c_str(), m_FileName.c_str());
		GetRainmeter().ShowMessage(m_Window, message.c_str(), MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	std::wstring resourcePath = GetResourcesPath();
	bool hasResourcesFolder = (_waccess(resourcePath.c_str(), 0) == 0);

	m_Parser.Initialize(iniFile, this, nullptr, &resourcePath);

	// Read options from Rainmeter.ini.
	ReadOptions();

	// Temporarily read "__UseD2D" from skin for easy testing
	bool useD2D = GetRainmeter().GetUseD2D();
	if (revision_beta)
	{
		useD2D = m_Parser.ReadBool(L"Rainmeter", L"__UseD2D", useD2D);
	}

	m_Canvas = Gfx::Canvas::Create(useD2D ? Gfx::Renderer::PreferD2D : Gfx::Renderer::GDIP);
	m_Canvas->SetAccurateText(m_Parser.ReadBool(L"Rainmeter", L"AccurateText", false));

	// Gotta have some kind of buffer during initialization
	CreateDoubleBuffer(1, 1);

	// Check the version
	UINT appVersion = m_Parser.ReadUInt(L"Rainmeter", L"AppVersion", 0);
	if (appVersion > RAINMETER_VERSION)
	{
		if (appVersion % 1000 != 0)
		{
			_snwprintf_s(buffer, _TRUNCATE, L"%u.%u.%u", appVersion / 1000000, (appVersion / 1000) % 1000, appVersion % 1000);
		}
		else
		{
			_snwprintf_s(buffer, _TRUNCATE, L"%u.%u", appVersion / 1000000, (appVersion / 1000) % 1000);
		}

		std::wstring text = GetFormattedString(ID_STR_NEWVERSIONREQUIRED, m_FolderPath.c_str(), m_FileName.c_str(), buffer);
		GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	// Initialize window variables
	SetWindowPositionVariables(m_ScreenX, m_ScreenY);
	SetWindowSizeVariables(0, 0);

	static const RECT defMargins = {0};
	m_BackgroundMargins = m_Parser.ReadRECT(L"Rainmeter", L"BackgroundMargins", defMargins);
	m_DragMargins = m_Parser.ReadRECT(L"Rainmeter", L"DragMargins", defMargins);

	m_BackgroundMode = (BGMODE)m_Parser.ReadInt(L"Rainmeter", L"BackgroundMode", BGMODE_IMAGE);
	m_SolidBevel = (BEVELTYPE)m_Parser.ReadInt(L"Rainmeter", L"BevelType", BEVELTYPE_NONE);

	m_SolidColor = m_Parser.ReadColor(L"Rainmeter", L"SolidColor", Color::Gray);
	m_SolidColor2 = m_Parser.ReadColor(L"Rainmeter", L"SolidColor2", m_SolidColor.GetValue());
	m_SolidAngle = (Gdiplus::REAL)m_Parser.ReadFloat(L"Rainmeter", L"GradientAngle", 0.0);

	m_DynamicWindowSize = m_Parser.ReadBool(L"Rainmeter", L"DynamicWindowSize", false);

	if (m_BackgroundMode == BGMODE_IMAGE || m_BackgroundMode == BGMODE_SCALED_IMAGE || m_BackgroundMode == BGMODE_TILED_IMAGE)
	{
		m_BackgroundName = m_Parser.ReadString(L"Rainmeter", L"Background", L"");
		if (!m_BackgroundName.empty())
		{
			MakePathAbsolute(m_BackgroundName);
		}
		else
		{
			m_BackgroundMode = BGMODE_COPY;
		}
	}

	m_Mouse.ReadOptions(m_Parser, L"Rainmeter");

	m_OnRefreshAction = m_Parser.ReadString(L"Rainmeter", L"OnRefreshAction", L"", false);
	m_OnCloseAction = m_Parser.ReadString(L"Rainmeter", L"OnCloseAction", L"", false);
	m_OnFocusAction = m_Parser.ReadString(L"Rainmeter", L"OnFocusAction", L"", false);
	m_OnUnfocusAction = m_Parser.ReadString(L"Rainmeter", L"OnUnfocusAction", L"", false);
	m_OnUpdateAction = m_Parser.ReadString(L"Rainmeter", L"OnUpdateAction", L"", false);
	m_OnWakeAction = m_Parser.ReadString(L"Rainmeter", L"OnWakeAction", L"", false);

	m_WindowUpdate = m_Parser.ReadInt(L"Rainmeter", L"Update", INTERVAL_METER);
	m_TransitionUpdate = m_Parser.ReadInt(L"Rainmeter", L"TransitionUpdate", INTERVAL_TRANSITION);
	m_ToolTipHidden = m_Parser.ReadBool(L"Rainmeter", L"ToolTipHidden", false);

	if (Platform::IsAtLeastWinVista())
	{
		if (m_Parser.ReadBool(L"Rainmeter", L"Blur", false))
		{
			const WCHAR* blurRegion = m_Parser.ReadString(L"Rainmeter", L"BlurRegion", L"", false).c_str();

			if (*blurRegion)
			{
				m_BlurMode = BLURMODE_REGION;
				m_BlurRegion = CreateRectRgn(0, 0, 0, 0);	// Create empty region
				int i = 1;

				do
				{
					ResizeBlur(blurRegion, RGN_OR);

					// Check for BlurRegion2, BlurRegion3, etc.
					_snwprintf_s(buffer, _TRUNCATE, L"BlurRegion%i", ++i);
					blurRegion = m_Parser.ReadString(L"Rainmeter", buffer, L"").c_str();
				}
				while (*blurRegion);
			}
			else
			{
				m_BlurMode = BLURMODE_FULL;
			}
		}
		else
		{
			m_BlurMode = BLURMODE_NONE;
		}
	}

	// Load fonts in Resources folder
	if (hasResourcesFolder)
	{
		WIN32_FIND_DATA fd;
		resourcePath += L"Fonts\\*";

		HANDLE find = FindFirstFileEx(
			resourcePath.c_str(),
			(Platform::IsAtLeastWin7()) ? FindExInfoBasic : FindExInfoStandard,
			&fd,
			FindExSearchNameMatch,
			nullptr,
			0);

		if (find != INVALID_HANDLE_VALUE)
		{
			m_FontCollection = m_Canvas->CreateFontCollection();

			do
			{
				if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					std::wstring file(resourcePath, 0, resourcePath.length() - 1);
					file += fd.cFileName;
					if (!m_FontCollection->AddFile(file.c_str()))
					{
						LogErrorF(this, L"Unable to load font: %s", file.c_str());
					}
				}
			}
			while (FindNextFile(find, &fd));

			FindClose(find);
		}
	}

	// Load local fonts
	const WCHAR* localFont = m_Parser.ReadString(L"Rainmeter", L"LocalFont", L"").c_str();
	if (*localFont)
	{
		if (!m_FontCollection)
		{
			m_FontCollection = m_Canvas->CreateFontCollection();
		}

		int i = 1;
		do
		{
			// Try program folder first
			std::wstring szFontFile = GetRainmeter().GetPath() + L"Fonts\\";
			szFontFile += localFont;
			if (!m_FontCollection->AddFile(szFontFile.c_str()))
			{
				szFontFile = localFont;
				MakePathAbsolute(szFontFile);
				if (!m_FontCollection->AddFile(szFontFile.c_str()))
				{
					LogErrorF(this, L"Unable to load font: %s", localFont);
				}
			}

			// Check for LocalFont2, LocalFont3, etc.
			_snwprintf_s(buffer, _TRUNCATE, L"LocalFont%i", ++i);
			localFont = m_Parser.ReadString(L"Rainmeter", buffer, L"").c_str();
		}
		while (*localFont);
	}

	// Create all meters and measures. The meters and measures are not initialized in this loop
	// to avoid errors caused by referencing nonexistent [sections] in the options.
	m_HasNetMeasures = false;
	m_HasButtons = false;
	Meter* prevMeter = nullptr;
	for (auto iter = m_Parser.GetSections().cbegin(); iter != m_Parser.GetSections().cend(); ++iter)
	{
		const WCHAR* section = (*iter).c_str();

		if (_wcsicmp(L"Rainmeter", section) != 0 &&
			_wcsicmp(L"Variables", section) != 0 &&
			_wcsicmp(L"Metadata", section) != 0)
		{
			const std::wstring& measureName = m_Parser.ReadString(section, L"Measure", L"", false);
			if (!measureName.empty())
			{
				Measure* measure = Measure::Create(measureName.c_str(), this, section);
				if (measure)
				{
					m_Measures.push_back(measure);
					m_Parser.AddMeasure(measure);

					if (measure->GetTypeID() == TypeID<MeasureNet>())
					{
						m_HasNetMeasures = true;
					}
				}

				continue;
			}

			const std::wstring& meterName = m_Parser.ReadString(section, L"Meter", L"", false);
			if (!meterName.empty())
			{
				// It's a meter
				Meter* meter = Meter::Create(meterName.c_str(), this, section);
				if (meter)
				{
					m_Meters.push_back(meter);
					meter->SetRelativeMeter(prevMeter);

					if (meter->GetTypeID() == TypeID<MeterButton>())
					{
						m_HasButtons = true;
					}

					prevMeter = meter;
				}

				continue;
			}
		}
	}

	if (m_Meters.empty())
	{
		std::wstring text = GetFormattedString(ID_STR_NOMETERSINSKIN, m_FolderPath.c_str(), m_FileName.c_str());
		GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	// Read measure options. This is done before the meters to ensure that e.g. Substitute is used
	// when the meters get the value of the measure. The measures cannot be initialized yet as som
	// measures (e.g. Script) except that the meters are ready when calling Initialize().
	for (auto iter = m_Measures.cbegin(); iter != m_Measures.cend(); ++iter)
	{
		Measure* measure = *iter;
		measure->ReadOptions(m_Parser);
	}

	// Initialize meters.
	for (auto iter = m_Meters.cbegin(); iter != m_Meters.cend(); ++iter)
	{
		Meter* meter = *iter;
		meter->ReadOptions(m_Parser);
		meter->Initialize();

		if (!meter->GetToolTipText().empty())
		{
			meter->CreateToolTip(this);
		}
	}

	// Initialize measures.
	for (auto iter = m_Measures.cbegin(); iter != m_Measures.cend(); ++iter)
	{
		Measure* measure = *iter;
		measure->Initialize();
	}

	// Set window size (and CURRENTCONFIGWIDTH/HEIGHT) temporarily
	for (auto iter = m_Meters.cbegin(); iter != m_Meters.cend(); ++iter)
	{
		bool bActiveTransition = true;  // Do not track the change of ActiveTransition
		UpdateMeter(*iter, bActiveTransition, true);
	}
	ResizeWindow(true);

	return true;
}

/*
** Changes the size of the window and re-adjusts the background
*/
bool MeterWindow::ResizeWindow(bool reset)
{
	int w = m_BackgroundMargins.left;
	int h = m_BackgroundMargins.top;

	// Get the largest meter point
	std::vector<Meter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		int mr = (*j)->GetX() + (*j)->GetW();
		w = max(w, mr);
		int mb = (*j)->GetY() + (*j)->GetH();
		h = max(h, mb);
	}

	w += m_BackgroundMargins.right;
	h += m_BackgroundMargins.bottom;

	w = max(w, m_BackgroundSize.cx);
	h = max(h, m_BackgroundSize.cy);

	if (!reset && m_WindowW == w && m_WindowH == h)
	{
		WindowToScreen();
		return false;		// The window is already correct size
	}

	// Reset size (this is calculated below)

	delete m_Background;
	m_Background = nullptr;

	if ((m_BackgroundMode == BGMODE_IMAGE || m_BackgroundMode == BGMODE_SCALED_IMAGE || m_BackgroundMode == BGMODE_TILED_IMAGE) && !m_BackgroundName.empty())
	{
		// Load the background
		TintedImage* tintedBackground = new TintedImage(L"Background");
		tintedBackground->ReadOptions(m_Parser, L"Rainmeter");
		tintedBackground->LoadImage(m_BackgroundName, true);

		if (!tintedBackground->IsLoaded())
		{
			m_BackgroundSize.cx = 0;
			m_BackgroundSize.cy = 0;

			m_WindowW = 0;
			m_WindowH = 0;
		}
		else
		{
			Bitmap* tempBackground = tintedBackground->GetImage();

			// Calculate the window dimensions
			m_BackgroundSize.cx = tempBackground->GetWidth();
			m_BackgroundSize.cy = tempBackground->GetHeight();

			if (m_BackgroundMode == BGMODE_IMAGE)
			{
				w = m_BackgroundSize.cx;
				h = m_BackgroundSize.cy;
			}
			else
			{
				w = max(w, m_BackgroundSize.cx);
				h = max(h, m_BackgroundSize.cy);
			}

			Bitmap* background = new Bitmap(w, h, PixelFormat32bppPARGB);
			Graphics graphics(background);

			if (m_BackgroundMode == BGMODE_IMAGE)
			{
				Rect r(0, 0, w, h);
				graphics.DrawImage(tempBackground, r, 0, 0, w, h, UnitPixel);
			}
			else
			{
				// Scale the background to fill the whole window
				if (m_BackgroundMode == BGMODE_SCALED_IMAGE)
				{
					const RECT m = m_BackgroundMargins;

					if (m.top > 0)
					{
						if (m.left > 0)
						{
							// Top-Left
							Rect r(0, 0, m.left, m.top);
							graphics.DrawImage(tempBackground, r, 0, 0, m.left, m.top, UnitPixel);
						}

						// Top
						Rect r(m.left, 0, w - m.left - m.right, m.top);
						graphics.DrawImage(tempBackground, r, m.left, 0, m_BackgroundSize.cx - m.left - m.right, m.top, UnitPixel);

						if (m.right > 0)
						{
							// Top-Right
							Rect r(w - m.right, 0, m.right, m.top);
							graphics.DrawImage(tempBackground, r, m_BackgroundSize.cx - m.right, 0, m.right, m.top, UnitPixel);
						}
					}

					if (m.left > 0)
					{
						// Left
						Rect r(0, m.top, m.left, h - m.top - m.bottom);
						graphics.DrawImage(tempBackground, r, 0, m.top, m.left, m_BackgroundSize.cy - m.top - m.bottom, UnitPixel);
					}

					// Center
					Rect r(m.left, m.top, w - m.left - m.right, h - m.top - m.bottom);
					graphics.DrawImage(tempBackground, r, m.left, m.top, m_BackgroundSize.cx - m.left - m.right, m_BackgroundSize.cy - m.top - m.bottom, UnitPixel);

					if (m.right > 0)
					{
						// Right
						Rect r(w - m.right, m.top, m.right, h - m.top - m.bottom);
						graphics.DrawImage(tempBackground, r, m_BackgroundSize.cx - m.right, m.top, m.right, m_BackgroundSize.cy - m.top - m.bottom, UnitPixel);
					}

					if (m.bottom > 0)
					{
						if (m.left > 0)
						{
							// Bottom-Left
							Rect r(0, h - m.bottom, m.left, m.bottom);
							graphics.DrawImage(tempBackground, r, 0, m_BackgroundSize.cy - m.bottom, m.left, m.bottom, UnitPixel);
						}

						// Bottom
						Rect r(m.left, h - m.bottom, w - m.left - m.right, m.bottom);
						graphics.DrawImage(tempBackground, r, m.left, m_BackgroundSize.cy - m.bottom, m_BackgroundSize.cx - m.left - m.right, m.bottom, UnitPixel);

						if (m.right > 0)
						{
							// Bottom-Right
							Rect r(w - m.right, h - m.bottom, m.right, m.bottom);
							graphics.DrawImage(tempBackground, r, m_BackgroundSize.cx - m.right, m_BackgroundSize.cy - m.bottom, m.right, m.bottom, UnitPixel);
						}
					}
				}
				else
				{
					ImageAttributes imgAttr;
					imgAttr.SetWrapMode(WrapModeTile);

					Rect r(0, 0, w, h);
					graphics.DrawImage(tempBackground, r, 0, 0, w, h, UnitPixel, &imgAttr);
				}
			}

			m_Background = background;

			// Get the size form the background bitmap
			m_WindowW = m_Background->GetWidth();
			m_WindowH = m_Background->GetHeight();

			WindowToScreen();
		}

		delete tintedBackground;
	}
	else
	{
		m_WindowW = w;
		m_WindowH = h;
		WindowToScreen();
	}

	SetWindowSizeVariables(m_WindowW, m_WindowH);

	return true;
}

/*
** Creates the back buffer bitmap.
**
*/
void MeterWindow::CreateDoubleBuffer(int cx, int cy)
{
	m_Canvas->Resize(cx, cy);
}

/*
** Redraws the meters and paints the window
**
*/
void MeterWindow::Redraw()
{
	if (m_ResizeWindow)
	{
		ResizeWindow(m_ResizeWindow == RESIZEMODE_RESET);
		SetResizeWindowMode(RESIZEMODE_NONE);
	}

	// Create or clear the doublebuffer
	{
		int cx = m_WindowW;
		int cy = m_WindowH;

		if (cx == 0 || cy == 0)
		{
			// Set dummy size to avoid invalid state
			cx = 1;
			cy = 1;
		}

		if (cx != m_Canvas->GetW() || cy != m_Canvas->GetH())
		{
			CreateDoubleBuffer(cx, cy);
		}
	}

	if (!m_Canvas->BeginDraw())
	{
		return;
	}

	m_Canvas->Clear();

	if (m_WindowW != 0 && m_WindowH != 0)
	{
		if (m_Background)
		{
			const Rect dst(0, 0, m_WindowW, m_WindowH);
			const Rect src(0, 0, m_Background->GetWidth(), m_Background->GetHeight());
			m_Canvas->DrawBitmap(m_Background, dst, src);
		}
		else if (m_BackgroundMode == BGMODE_SOLID)
		{
			// Draw the solid color background
			Rect r(0, 0, m_WindowW, m_WindowH);

			if (m_SolidColor.GetA() != 0 || m_SolidColor2.GetA() != 0)
			{
				if (m_SolidColor.GetValue() == m_SolidColor2.GetValue())
				{
					m_Canvas->Clear(m_SolidColor);
				}
				else
				{
					Gdiplus::Graphics& graphics = m_Canvas->BeginGdiplusContext();
					LinearGradientBrush gradient(r, m_SolidColor, m_SolidColor2, m_SolidAngle, TRUE);
					graphics.FillRectangle(&gradient, r);
					m_Canvas->EndGdiplusContext();
				}
			}

			if (m_SolidBevel != BEVELTYPE_NONE)
			{
				Color lightColor(255, 255, 255, 255);
				Color darkColor(255, 0, 0, 0);

				if (m_SolidBevel == BEVELTYPE_DOWN)
				{
					lightColor.SetValue(Color::MakeARGB(255, 0, 0, 0));
					darkColor.SetValue(Color::MakeARGB(255, 255, 255, 255));
				}

				Pen light(lightColor);
				Pen dark(darkColor);

				Gdiplus::Graphics& graphics = m_Canvas->BeginGdiplusContext();
				Meter::DrawBevel(graphics, r, light, dark);
				m_Canvas->EndGdiplusContext();
			}
		}

		// Draw the meters
		std::vector<Meter*>::const_iterator j = m_Meters.begin();
		for ( ; j != m_Meters.end(); ++j)
		{
			const Matrix* matrix = (*j)->GetTransformationMatrix();
			if (matrix && !matrix->IsIdentity())
			{
				m_Canvas->SetTransform(*matrix);
				(*j)->Draw(*m_Canvas);
				m_Canvas->ResetTransform();
			}
			else
			{
				(*j)->Draw(*m_Canvas);
			}
		}
	}

	UpdateWindow(m_TransparencyValue, false, true);

	m_Canvas->EndDraw();
}

/*
** Updates the transition state
**
*/
void MeterWindow::PostUpdate(bool bActiveTransition)
{
	// Start/stop the transition timer if necessary
	if (bActiveTransition && !m_ActiveTransition)
	{
		SetTimer(m_Window, TIMER_TRANSITION, m_TransitionUpdate, nullptr);
		m_ActiveTransition = true;
	}
	else if (m_ActiveTransition && !bActiveTransition)
	{
		KillTimer(m_Window, TIMER_TRANSITION);
		m_ActiveTransition = false;
	}
}

/*
** Updates the given measure
**
*/
bool MeterWindow::UpdateMeasure(Measure* measure, bool force)
{
	bool bUpdate = false;

	if (force)
	{
		measure->ResetUpdateCounter();
	}

	int updateDivider = measure->GetUpdateDivider();
	if (updateDivider >= 0 || force)
	{
		if (measure->HasDynamicVariables() &&
			(measure->GetUpdateCounter() + 1) >= updateDivider)
		{
			measure->ReadOptions(m_Parser);
		}

		bUpdate = measure->Update();
	}

	return bUpdate;
}

/*
** Updates the given meter
**
*/
bool MeterWindow::UpdateMeter(Meter* meter, bool& bActiveTransition, bool force)
{
	bool bUpdate = false;

	if (force)
	{
		meter->ResetUpdateCounter();
	}

	int updateDivider = meter->GetUpdateDivider();
	if (updateDivider >= 0 || force)
	{
		if (meter->HasDynamicVariables() &&
			(meter->GetUpdateCounter() + 1) >= updateDivider)
		{
			meter->ReadOptions(m_Parser);
		}

		bUpdate = meter->Update();
	}

	// Update tooltips
	if (!meter->HasToolTip())
	{
		if (!meter->GetToolTipText().empty())
		{
			meter->CreateToolTip(this);
		}
	}
	else
	{
		meter->UpdateToolTip();
	}

	// Check for transitions
	if (!bActiveTransition && meter->HasActiveTransition())
	{
		bActiveTransition = true;
	}

	return bUpdate;
}

/*
** Updates all the measures and redraws the meters
**
*/
void MeterWindow::Update(bool refresh)
{
	++m_UpdateCounter;

	if (!m_Measures.empty())
	{
		// Pre-updates
		if (m_HasNetMeasures)
		{
			MeasureNet::UpdateIFTable();
			MeasureNet::UpdateStats();
		}

		// Update all measures
		std::vector<Measure*>::const_iterator i = m_Measures.begin();
		for ( ; i != m_Measures.end(); ++i)
		{
			if (UpdateMeasure((*i), refresh))
			{
				(*i)->DoUpdateAction();
				(*i)->DoChangeAction();
			}
		}
	}

	DialogAbout::UpdateMeasures(this);

	// Update all meters
	bool bActiveTransition = false;
	bool bUpdate = false;
	std::vector<Meter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		if (UpdateMeter((*j), bActiveTransition, refresh))
		{
			bUpdate = true;

			(*j)->DoUpdateAction();
		}
	}

	// Redraw all meters
	if (bUpdate || m_ResizeWindow || refresh)
	{
		if (m_DynamicWindowSize)
		{
			// Resize the window
			SetResizeWindowMode(RESIZEMODE_CHECK);
		}

		// If our option is to disable when in an RDP session, then check if in an RDP session.
		// Only redraw if we are not in a remote session
		if (GetRainmeter().IsRedrawable())
		{
			Redraw();
		}
	}

	// Post-updates
	PostUpdate(bActiveTransition);

	if (!m_OnUpdateAction.empty())
	{
		GetRainmeter().ExecuteCommand(m_OnUpdateAction.c_str(), this);
	}
}

/*
** Updates the window contents
**
*/
void MeterWindow::UpdateWindow(int alpha, bool reset, bool canvasBeginDrawCalled)
{
	if (reset)
	{
		AddWindowExStyle(WS_EX_LAYERED);
	}

	BLENDFUNCTION blendPixelFunction = {AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA};
	POINT ptSrc = {0, 0};
	SIZE szWindow = {m_Canvas->GetW(), m_Canvas->GetH()};

	if (!canvasBeginDrawCalled) m_Canvas->BeginDraw();

	HDC dcMemory = m_Canvas->GetDC();
	if (!UpdateLayeredWindow(m_Window, nullptr, nullptr, &szWindow, dcMemory, &ptSrc, 0, &blendPixelFunction, ULW_ALPHA))
	{
		// Retry after resetting WS_EX_LAYERED flag.
		RemoveWindowExStyle(WS_EX_LAYERED);
		AddWindowExStyle(WS_EX_LAYERED);
		UpdateLayeredWindow(m_Window, nullptr, nullptr, &szWindow, dcMemory, &ptSrc, 0, &blendPixelFunction, ULW_ALPHA);
	}
	m_Canvas->ReleaseDC(dcMemory);

	if (!canvasBeginDrawCalled) m_Canvas->EndDraw();

	m_TransparencyValue = alpha;
}

/*
** Updates the window transparency (using existing contents).
**
*/
void MeterWindow::UpdateWindowTransparency(int alpha)
{
	BLENDFUNCTION blendPixelFunction = {AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA};
	UpdateLayeredWindow(m_Window, nullptr, nullptr, nullptr, nullptr, nullptr, 0, &blendPixelFunction, ULW_ALPHA);
	m_TransparencyValue = alpha;
}

/*
** Handles the timers. The METERTIMER updates all the measures
** MOUSETIMER is used to hide/show the window.
**
*/
LRESULT MeterWindow::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case TIMER_METER:
		Update(false);
		break;

	case TIMER_MOUSE:
		if (!GetRainmeter().IsMenuActive() && !m_Dragging)
		{
			ShowWindowIfAppropriate();

			if (m_WindowZPosition == ZPOSITION_ONTOPMOST)
			{
				ChangeZPos(ZPOSITION_ONTOPMOST);
			}

			if (m_MouseOver)
			{
				POINT pos = System::GetCursorPosition();

				if (!m_ClickThrough)
				{
					if (WindowFromPoint(pos) == m_Window)
					{
						SetMouseLeaveEvent(false);
					}
					else
					{
						// Run all mouse leave actions
						OnMouseLeave(m_WindowDraggable ? WM_NCMOUSELEAVE : WM_MOUSELEAVE, 0, 0);
					}
				}
				else
				{
					bool keyDown = IsCtrlKeyDown() || IsShiftKeyDown() || IsAltKeyDown();

					if (!keyDown || GetWindowFromPoint(pos) != m_Window)
					{
						// Run all mouse leave actions
						OnMouseLeave(m_WindowDraggable ? WM_NCMOUSELEAVE : WM_MOUSELEAVE, 0, 0);
					}
				}
			}
		}
		break;

	case TIMER_TRANSITION:
		{
			// Redraw only if there is active transition still going
			bool bActiveTransition = false;
			std::vector<Meter*>::const_iterator j = m_Meters.begin();
			for ( ; j != m_Meters.end(); ++j)
			{
				if ((*j)->HasActiveTransition())
				{
					bActiveTransition = true;
					break;
				}
			}

			if (bActiveTransition)
			{
				Redraw();
			}
			else
			{
				// Stop the transition timer
				KillTimer(m_Window, TIMER_TRANSITION);
				m_ActiveTransition = false;
			}
		}
		break;

	case TIMER_FADE:
		{
			ULONGLONG ticks = System::GetTickCount64();
			if (m_FadeStartTime == 0)
			{
				m_FadeStartTime = ticks;
			}

			if (ticks - m_FadeStartTime > (ULONGLONG)m_FadeDuration)
			{
				KillTimer(m_Window, TIMER_FADE);
				m_FadeStartTime = 0;
				if (m_FadeEndValue == 0)
				{
					ShowWindow(m_Window, SW_HIDE);
				}
				else
				{
					UpdateWindowTransparency(m_FadeEndValue);
				}
			}
			else
			{
				double value = (double)(__int64)(ticks - m_FadeStartTime);
				value /= m_FadeDuration;
				value *= m_FadeEndValue - m_FadeStartValue;
				value += m_FadeStartValue;
				value = min(value, 255);
				value = max(value, 0);

				UpdateWindowTransparency((int)value);
			}
		}
		break;

	case TIMER_DEACTIVATE:
		if (m_FadeStartTime == 0)
		{
			KillTimer(m_Window, TIMER_DEACTIVATE);
			GetRainmeter().RemoveUnmanagedMeterWindow(this);
			delete this;
		}
		break;
	}

	return 0;
}

void MeterWindow::FadeWindow(int from, int to)
{
	if (m_FadeDuration == 0)
	{
		if (to == 0)
		{
			ShowWindow(m_Window, SW_HIDE);
		}
		else
		{
			if (m_FadeDuration == 0)
			{
				UpdateWindowTransparency(to);
			}
			if (from == 0)
			{
				if (!m_Hidden)
				{
					ShowWindow(m_Window, SW_SHOWNOACTIVATE);
				}
			}
		}
	}
	else
	{
		m_FadeStartValue = from;
		m_FadeEndValue = to;
		UpdateWindowTransparency(from);
		if (from == 0)
		{
			if (!m_Hidden)
			{
				ShowWindow(m_Window, SW_SHOWNOACTIVATE);
			}
		}

		SetTimer(m_Window, TIMER_FADE, INTERVAL_FADE, nullptr);
	}
}

void MeterWindow::HideFade()
{
	m_Hidden = true;
	if (IsWindowVisible(m_Window))
	{
		FadeWindow(m_AlphaValue, 0);
	}
}

void MeterWindow::ShowFade()
{
	m_Hidden = false;
	if (!IsWindowVisible(m_Window))
	{
		FadeWindow(0, (m_WindowHide == HIDEMODE_FADEOUT) ? 255 : m_AlphaValue);
	}
}

/*
** Show the window if it is temporarily hidden.
**
*/
void MeterWindow::ShowWindowIfAppropriate()
{
	bool keyDown = IsCtrlKeyDown() || IsShiftKeyDown() || IsAltKeyDown();

	POINT pos = System::GetCursorPosition();
	POINT posScr = pos;

	MapWindowPoints(nullptr, m_Window, &pos, 1);
	bool inside = HitTest(pos.x, pos.y);

	if (inside)
	{
		inside = (GetWindowFromPoint(posScr) == m_Window);
	}

	if (m_ClickThrough)
	{
		if (!inside || keyDown)
		{
			// If Alt, shift or control is down, remove the transparent flag
			RemoveWindowExStyle(WS_EX_TRANSPARENT);
		}
	}

	if (m_WindowHide)
	{
		if (!m_Hidden && !inside && !keyDown)
		{
			switch (m_WindowHide)
			{
			case HIDEMODE_HIDE:
				if (m_TransparencyValue == 0 || !IsWindowVisible(m_Window))
				{
					ShowWindow(m_Window, SW_SHOWNOACTIVATE);
					FadeWindow(0, m_AlphaValue);
				}
				break;

			case HIDEMODE_FADEIN:
				if (m_AlphaValue != 255 && m_TransparencyValue == 255)
				{
					FadeWindow(255, m_AlphaValue);
				}
				break;

			case HIDEMODE_FADEOUT:
				if (m_AlphaValue != 255 && m_TransparencyValue == m_AlphaValue)
				{
					FadeWindow(m_AlphaValue, 255);
				}
				break;
			}
		}
	}
	else
	{
		if (!m_Hidden)
		{
			if (m_TransparencyValue == 0 || !IsWindowVisible(m_Window))
			{
				ShowWindow(m_Window, SW_SHOWNOACTIVATE);
				FadeWindow(0, m_AlphaValue);
			}
		}
	}
}

/*
** Retrieves a handle to the window that contains the specified point.
**
*/
HWND MeterWindow::GetWindowFromPoint(POINT pos)
{
	HWND hwndPos = WindowFromPoint(pos);

	if (hwndPos == m_Window || (!m_ClickThrough && m_WindowHide != HIDEMODE_HIDE))
	{
		return hwndPos;
	}

	MapWindowPoints(nullptr, m_Window, &pos, 1);

	if (HitTest(pos.x, pos.y))
	{
		if (hwndPos)
		{
			HWND hWnd = GetAncestor(hwndPos, GA_ROOT);
			while (hWnd = FindWindowEx(nullptr, hWnd, METERWINDOW_CLASS_NAME, nullptr))
			{
				if (hWnd == m_Window)
				{
					return hwndPos;
				}
			}
		}
		return m_Window;
	}

	return hwndPos;
}

/*
** Checks if the given point is inside the window.
**
*/
bool MeterWindow::HitTest(int x, int y)
{
	return m_Canvas->IsTransparentPixel(x, y);
}

/*
** Handles all buttons and cursor.
**
*/
void MeterWindow::HandleButtons(POINT pos, BUTTONPROC proc, bool execute)
{
	bool redraw = false;
	HCURSOR cursor = nullptr;

	std::vector<Meter*>::const_reverse_iterator j = m_Meters.rbegin();
	for ( ; j != m_Meters.rend(); ++j)
	{
		// Hidden meters are ignored
		if ((*j)->IsHidden()) continue;

		MeterButton* button = nullptr;
		if (m_HasButtons && (*j)->GetTypeID() == TypeID<MeterButton>())
		{
			button = (MeterButton*)(*j);
			if (button)
			{
				switch (proc)
				{
				case BUTTONPROC_DOWN:
					redraw |= button->MouseDown(pos);
					break;

				case BUTTONPROC_UP:
					redraw |= button->MouseUp(pos, execute);
					break;

				case BUTTONPROC_MOVE:
				default:
					redraw |= button->MouseMove(pos);
					break;
				}
			}
		}

		// Get cursor if required
		if (!cursor && (*j)->GetMouse().GetCursorState())
		{
			if ((*j)->HasMouseAction())
			{
				if ((*j)->HitTest(pos.x, pos.y))
				{
					cursor = (*j)->GetMouse().GetCursor();
				}
			}
			else
			{
				// Special case for Button meter: reacts only on valid pixel in button image
				if (button && button->HitTest2(pos.x, pos.y))
				{
					cursor = (*j)->GetMouse().GetCursor();
				}
			}
		}
	}

	if (redraw)
	{
		Redraw();
	}

	if (!cursor)
	{
		cursor = LoadCursor(nullptr, IDC_ARROW);
	}

	SetCursor(cursor);
}

/*
** During setting the cursor do nothing.
**
*/
LRESULT MeterWindow::OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

/*
** Enters context menu loop.
**
*/
LRESULT MeterWindow::OnEnterMenuLoop(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Set cursor to default
	SetCursor(LoadCursor(nullptr, IDC_ARROW));

	return 0;
}

/*
** When we get WM_MOUSEMOVE messages, hide the window as the mouse is over it.
**
*/
LRESULT MeterWindow::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	bool keyDown = IsCtrlKeyDown() || IsShiftKeyDown() || IsAltKeyDown();

	if (!keyDown)
	{
		if (m_ClickThrough)
		{
			AddWindowExStyle(WS_EX_TRANSPARENT);
		}

		if (!m_Hidden)
		{
			// If Alt, shift or control is down, do not hide the window
			switch (m_WindowHide)
			{
			case HIDEMODE_HIDE:
				if (m_TransparencyValue == m_AlphaValue)
				{
					FadeWindow(m_AlphaValue, 0);
				}
				break;

			case HIDEMODE_FADEIN:
				if (m_AlphaValue != 255 && m_TransparencyValue == m_AlphaValue)
				{
					FadeWindow(m_AlphaValue, 255);
				}
				break;

			case HIDEMODE_FADEOUT:
				if (m_AlphaValue != 255 && m_TransparencyValue == 255)
				{
					FadeWindow(255, m_AlphaValue);
				}
				break;
			}
		}
	}

	if (!m_ClickThrough || keyDown)
	{
		POINT pos;
		pos.x = GET_X_LPARAM(lParam);
		pos.y = GET_Y_LPARAM(lParam);

		if (uMsg == WM_NCMOUSEMOVE)
		{
			// Map to local window
			MapWindowPoints(nullptr, m_Window, &pos, 1);
		}

		++m_MouseMoveCounter;

		while (DoMoveAction(pos.x, pos.y, MOUSE_LEAVE)) ;
		while (DoMoveAction(pos.x, pos.y, MOUSE_OVER)) ;

		// Handle buttons
		HandleButtons(pos, BUTTONPROC_MOVE);
	}

	return 0;
}

/*
** When we get WM_MOUSELEAVE messages, run all leave actions.
**
*/
LRESULT MeterWindow::OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos = System::GetCursorPosition();
	HWND hWnd = WindowFromPoint(pos);
	if (!hWnd || (hWnd != m_Window && GetParent(hWnd) != m_Window))  // ignore tooltips
	{
		++m_MouseMoveCounter;

		POINT pos = {SHRT_MIN, SHRT_MIN};
		while (DoMoveAction(pos.x, pos.y, MOUSE_LEAVE)) ;  // Leave all forcibly

		// Handle buttons
		HandleButtons(pos, BUTTONPROC_MOVE);
	}

	return 0;
}

/*
** When we get WM_MOUSEWHEEL messages.
**
*/
LRESULT MeterWindow::OnMouseScrollMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_MOUSEWHEEL)  // If sent through WM_INPUT, uMsg is WM_INPUT.
	{
		// Fix for Notepad++, which sends WM_MOUSEWHEEL to unfocused windows.
		if (m_Window != GetFocus())
		{
			return 0;
		}
	}

	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	MapWindowPoints(nullptr, m_Window, &pos, 1);

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_MOVE);

	const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
	DoAction(pos.x, pos.y, (delta < 0) ? MOUSE_MW_DOWN : MOUSE_MW_UP, false);

	return 0;
}

/*
** When we get WM_MOUSEHWHEEL messages.
**
*/
LRESULT MeterWindow::OnMouseHScrollMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	MapWindowPoints(nullptr, m_Window, &pos, 1);

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_MOVE);

	const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
	DoAction(pos.x, pos.y, (delta < 0) ? MOUSE_MW_LEFT : MOUSE_MW_RIGHT, false);

	return 0;
}

/*
** Handle the menu commands.
**
*/
LRESULT MeterWindow::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case IDM_SKIN_EDITSKIN:
		GetRainmeter().EditSkinFile(m_FolderPath, m_FileName);
		break;

	case IDM_SKIN_REFRESH:
		Refresh(false);
		break;

	case IDM_SKIN_OPENSKINSFOLDER:
		GetRainmeter().OpenSkinFolder(m_FolderPath);
		break;

	case IDM_SKIN_MANAGESKIN:
		DialogManage::OpenSkin(this);
		break;

	case IDM_SKIN_VERYTOPMOST:
		SetWindowZPosition(ZPOSITION_ONTOPMOST);
		break;

	case IDM_SKIN_TOPMOST:
		SetWindowZPosition(ZPOSITION_ONTOP);
		break;

	case IDM_SKIN_BOTTOM:
		SetWindowZPosition(ZPOSITION_ONBOTTOM);
		break;

	case IDM_SKIN_NORMAL:
		SetWindowZPosition(ZPOSITION_NORMAL);
		break;

	case IDM_SKIN_ONDESKTOP:
		SetWindowZPosition(ZPOSITION_ONDESKTOP);
		break;

	case IDM_SKIN_KEEPONSCREEN:
		SetKeepOnScreen(!m_KeepOnScreen);
		break;

	case IDM_SKIN_CLICKTHROUGH:
		SetClickThrough(!m_ClickThrough);
		break;

	case IDM_SKIN_DRAGGABLE:
		SetWindowDraggable(!m_WindowDraggable);
		break;

	case IDM_SKIN_HIDEONMOUSE:
		SetWindowHide((m_WindowHide == HIDEMODE_NONE) ? HIDEMODE_HIDE : HIDEMODE_NONE);
		break;

	case IDM_SKIN_TRANSPARENCY_FADEIN:
		SetWindowHide((m_WindowHide == HIDEMODE_NONE) ? HIDEMODE_FADEIN : HIDEMODE_NONE);
		break;

	case IDM_SKIN_TRANSPARENCY_FADEOUT:
		SetWindowHide((m_WindowHide == HIDEMODE_NONE) ? HIDEMODE_FADEOUT : HIDEMODE_NONE);
		break;

	case IDM_SKIN_REMEMBERPOSITION:
		SetSavePosition(!m_SavePosition);
		break;

	case IDM_SKIN_SNAPTOEDGES:
		SetSnapEdges(!m_SnapEdges);
		break;

	case IDM_CLOSESKIN:
		if (m_State != STATE_CLOSING)
		{
			GetRainmeter().DeactivateSkin(this, -1);
		}
		break;

	case IDM_SKIN_FROMRIGHT:
		m_WindowXFromRight = !m_WindowXFromRight;

		SavePositionIfAppropriate();
		break;

	case IDM_SKIN_FROMBOTTOM:
		m_WindowYFromBottom = !m_WindowYFromBottom;

		SavePositionIfAppropriate();
		break;

	case IDM_SKIN_XPERCENTAGE:
		m_WindowXPercentage = !m_WindowXPercentage;

		SavePositionIfAppropriate();
		break;

	case IDM_SKIN_YPERCENTAGE:
		m_WindowYPercentage = !m_WindowYPercentage;

		SavePositionIfAppropriate();
		break;

	case IDM_SKIN_MONITOR_AUTOSELECT:
		m_AutoSelectScreen = !m_AutoSelectScreen;

		WriteOptions(OPTION_POSITION | OPTION_AUTOSELECTSCREEN);
		break;

	default:
		if (wParam >= IDM_SKIN_TRANSPARENCY_0 && wParam <= IDM_SKIN_TRANSPARENCY_90)
		{
			m_AlphaValue = (int)(255.0 - (wParam - IDM_SKIN_TRANSPARENCY_0) * (230.0 / (IDM_SKIN_TRANSPARENCY_90 - IDM_SKIN_TRANSPARENCY_0)));
			UpdateWindowTransparency(m_AlphaValue);
			WriteOptions(OPTION_ALPHAVALUE);
		}
		else if (wParam == IDM_SKIN_MONITOR_PRIMARY || wParam >= ID_MONITOR_FIRST && wParam <= ID_MONITOR_LAST)
		{
			const int numOfMonitors = (int)System::GetMonitorCount();
			const MultiMonitorInfo& monitorsInfo = System::GetMultiMonitorInfo();
			const std::vector<MonitorInfo>& monitors = monitorsInfo.monitors;

			int screenIndex;
			bool screenDefined;
			if (wParam == IDM_SKIN_MONITOR_PRIMARY)
			{
				screenIndex = monitorsInfo.primary;
				screenDefined = false;
			}
			else
			{
				screenIndex = (wParam & 0x0ffff) - ID_MONITOR_FIRST;
				screenDefined = true;
			}

			if (screenIndex >= 0 && (screenIndex == 0 || screenIndex <= numOfMonitors && monitors[screenIndex - 1].active))
			{
				m_AutoSelectScreen = false;

				m_WindowXScreen = m_WindowYScreen = screenIndex;
				m_WindowXScreenDefined = m_WindowYScreenDefined = screenDefined;

				m_Parser.ResetMonitorVariables(this);  // Set present monitor variables
				WriteOptions(OPTION_POSITION | OPTION_AUTOSELECTSCREEN);
			}
		}
		else if (wParam >= IDM_SKIN_CUSTOMCONTEXTMENU_FIRST && wParam <= IDM_SKIN_CUSTOMCONTEXTMENU_LAST)
		{
			std::wstring action;

			int position = (int)wParam - IDM_SKIN_CUSTOMCONTEXTMENU_FIRST + 1;
			if (position == 1)
			{
				action = m_Parser.ReadString(L"Rainmeter", L"ContextAction", L"", false);
			}
			else
			{
				WCHAR buffer[128];

				_snwprintf_s(buffer, _TRUNCATE, L"ContextAction%i", position);
				action = m_Parser.ReadString(L"Rainmeter", buffer, L"", false);
			}

			if (!action.empty())
			{
				GetRainmeter().ExecuteCommand(action.c_str(), this);
			}
		}
		else
		{
			// Forward to tray window, which handles all the other commands
			HWND tray = GetRainmeter().GetTrayWindow()->GetWindow();

			if (wParam == IDM_QUIT)
			{
				PostMessage(tray, WM_COMMAND, wParam, lParam);
			}
			else
			{
				SendMessage(tray, WM_COMMAND, wParam, lParam);
			}
		}
		break;
	}

	return 0;
}

/*
** Helper function for setting ClickThrough
**
*/
void MeterWindow::SetClickThrough(bool b)
{
	m_ClickThrough = b;
	WriteOptions(OPTION_CLICKTHROUGH);

	if (!m_ClickThrough)
	{
		// Remove transparent flag
		RemoveWindowExStyle(WS_EX_TRANSPARENT);
	}

	if (m_MouseOver)
	{
		SetMouseLeaveEvent(m_ClickThrough);
	}
}

/*
** Helper function for setting KeepOnScreen
**
*/
void MeterWindow::SetKeepOnScreen(bool b)
{
	m_KeepOnScreen = b;
	WriteOptions(OPTION_KEEPONSCREEN);

	if (m_KeepOnScreen)
	{
		int x = m_ScreenX;
		int y = m_ScreenY;

		MapCoordsToScreen(x, y, m_WindowW, m_WindowH);

		if (x != m_ScreenX || y != m_ScreenY)
		{
			MoveWindow(x, y);
		}
	}
}

/*
** Helper function for setting WindowDraggable
**
*/
void MeterWindow::SetWindowDraggable(bool b)
{
	m_WindowDraggable = b;
	WriteOptions(OPTION_DRAGGABLE);
}

/*
** Helper function for setting SavePosition
**
*/
void MeterWindow::SetSavePosition(bool b)
{
	m_SavePosition = b;
	WriteOptions(OPTION_POSITION | OPTION_SAVEPOSITION);
}

/*
** Helper function for setting SavePosition
**
*/
void MeterWindow::SavePositionIfAppropriate()
{
	if (m_SavePosition)
	{
		WriteOptions(OPTION_POSITION);
	}
	else
	{
		ScreenToWindow();
		DialogManage::UpdateSkins(this);
	}
}

/*
** Helper function for setting SnapEdges
**
*/
void MeterWindow::SetSnapEdges(bool b)
{
	m_SnapEdges = b;
	WriteOptions(OPTION_SNAPEDGES);
}

/*
** Helper function for setting WindowHide
**
*/
void MeterWindow::SetWindowHide(HIDEMODE hide)
{
	m_WindowHide = hide;
	UpdateWindowTransparency(m_AlphaValue);
	WriteOptions(OPTION_HIDEONMOUSEOVER);
}

/*
** Helper function for setting Position
**
*/
void MeterWindow::SetWindowZPosition(ZPOSITION zpos)
{
	ChangeSingleZPos(zpos);
	WriteOptions(OPTION_ALWAYSONTOP);
}

/*
** Handle dragging the window
**
*/
LRESULT MeterWindow::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if ((wParam & 0xFFF0) != SC_MOVE)
	{
		return DefWindowProc(m_Window, uMsg, wParam, lParam);
	}

	// --- SC_MOVE ---

	// Prepare the dragging flags
	m_Dragging = true;
	m_Dragged = false;

	// Run the DefWindowProc so the dragging works
	LRESULT result = DefWindowProc(m_Window, uMsg, wParam, lParam);

	if (m_Dragged)
	{
		SavePositionIfAppropriate();

		POINT pos = System::GetCursorPosition();
		MapWindowPoints(nullptr, m_Window, &pos, 1);

		// Handle buttons
		HandleButtons(pos, BUTTONPROC_UP, false);  // redraw only
	}
	else  // not dragged
	{
		if ((wParam & 0x000F) == 2)  // triggered by mouse
		{
			// Post the WM_NCLBUTTONUP message so the LeftMouseUpAction works
			PostMessage(m_Window, WM_NCLBUTTONUP, (WPARAM)HTCAPTION, lParam);
		}
	}

	// Clear the dragging flags
	m_Dragging = false;
	m_Dragged = false;

	return result;
}

/*
** Starts dragging
**
*/
LRESULT MeterWindow::OnEnterSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_Dragging)
	{
		m_Dragged = true;  // Don't post the WM_NCLBUTTONUP message!

		// Set cursor to default
		SetCursor(LoadCursor(nullptr, IDC_ARROW));
	}

	return 0;
}

/*
** Ends dragging
**
*/
LRESULT MeterWindow::OnExitSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

/*
** This is overwritten so that the window can be dragged
**
*/
LRESULT MeterWindow::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_WindowDraggable && !GetRainmeter().GetDisableDragging())
	{
		POINT pos;
		pos.x = GET_X_LPARAM(lParam);
		pos.y = GET_Y_LPARAM(lParam);
		MapWindowPoints(nullptr, m_Window, &pos, 1);

		int x1 = m_DragMargins.left;
		if (x1 < 0) x1 += m_WindowW;

		int x2 = m_WindowW - m_DragMargins.right;
		if (x2 > m_WindowW) x2 -= m_WindowW;

		if (pos.x >= x1 && pos.x < x2)
		{
			int y1 = m_DragMargins.top;
			if (y1 < 0) y1 += m_WindowH;

			int y2 = m_WindowH - m_DragMargins.bottom;
			if (y2 > m_WindowH) y2 -= m_WindowH;

			if (pos.y >= y1 && pos.y < y2)
			{
				return HTCAPTION;
			}
		}
	}
	return HTCLIENT;
}

/*
** Called when windows position is about to change
**
*/
LRESULT MeterWindow::OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPWINDOWPOS wp = (LPWINDOWPOS)lParam;

	if (m_State != STATE_REFRESHING)
	{
		if (m_WindowZPosition == ZPOSITION_NORMAL && GetRainmeter().IsNormalStayDesktop() && System::GetShowDesktop())
		{
			if (!(wp->flags & (SWP_NOOWNERZORDER | SWP_NOACTIVATE)))
			{
				// Set window on top of all other ZPOSITION_ONDESKTOP, ZPOSITION_BOTTOM, and ZPOSITION_NORMAL windows
				wp->hwndInsertAfter = System::GetBackmostTopWindow();
			}
		}
		else if (m_WindowZPosition == ZPOSITION_ONDESKTOP || m_WindowZPosition == ZPOSITION_ONBOTTOM)
		{
			// Do not change the z-order. This keeps the window on bottom.
			wp->flags |= SWP_NOZORDER;
		}
	}

	if ((wp->flags & SWP_NOMOVE) == 0)
	{
		if (m_SnapEdges && !(IsCtrlKeyDown() || IsShiftKeyDown()))
		{
			// only process movement (ignore anything without winpos values)
			if (wp->cx != 0 && wp->cy != 0)
			{
				// Search display monitor that has the largest area of intersection with the window
				const size_t numOfMonitors = System::GetMonitorCount();  // intentional
				const std::vector<MonitorInfo>& monitors = System::GetMultiMonitorInfo().monitors;

				const RECT windowRect = {wp->x, wp->y, wp->x + (m_WindowW ? m_WindowW : 1), wp->y + (m_WindowH ? m_WindowH : 1)};
				const RECT* workArea = nullptr;

				size_t maxSize = 0;
				for (auto iter = monitors.cbegin(); iter != monitors.cend(); ++iter)
				{
					RECT r;
					if ((*iter).active && IntersectRect(&r, &windowRect, &(*iter).screen))
					{
						size_t size = (r.right - r.left) * (r.bottom - r.top);
						if (size > maxSize)
						{
							workArea = &(*iter).work;
							maxSize = size;
						}
					}
				}

				// Snap to other windows
				for (auto iter = GetRainmeter().GetAllMeterWindows().cbegin(); iter != GetRainmeter().GetAllMeterWindows().cend(); ++iter)
				{
					if ((*iter).second != this)
					{
						SnapToWindow((*iter).second, wp);
					}
				}

				// Snap to work area if window is on the appropriate screen
				if (workArea)
				{
					int w = workArea->right - m_WindowW;
					int h = workArea->bottom - m_WindowH;

					if ((wp->x < SNAPDISTANCE + workArea->left) && (wp->x > workArea->left - SNAPDISTANCE)) wp->x = workArea->left;
					if ((wp->y < SNAPDISTANCE + workArea->top) && (wp->y > workArea->top - SNAPDISTANCE)) wp->y = workArea->top;
					if ((wp->x < SNAPDISTANCE + w) && (wp->x > -SNAPDISTANCE + w)) wp->x = w;
					if ((wp->y < SNAPDISTANCE + h) && (wp->y > -SNAPDISTANCE + h)) wp->y = h;
				}
			}
		}

		if (m_KeepOnScreen)
		{
			MapCoordsToScreen(wp->x, wp->y, m_WindowW, m_WindowH);
		}
	}

	return 0;
}

void MeterWindow::SnapToWindow(MeterWindow* window, LPWINDOWPOS wp)
{
	int x = window->m_ScreenX;
	int y = window->m_ScreenY;
	int w = window->m_WindowW;
	int h = window->m_WindowH;

	if (wp->y < y + h && wp->y + m_WindowH > y)
	{
		if ((wp->x < SNAPDISTANCE + x) && (wp->x > x - SNAPDISTANCE)) wp->x = x;
		if ((wp->x < SNAPDISTANCE + x + w) && (wp->x > x + w - SNAPDISTANCE)) wp->x = x + w;

		if ((wp->x + m_WindowW < SNAPDISTANCE + x) && (wp->x + m_WindowW > x - SNAPDISTANCE)) wp->x = x - m_WindowW;
		if ((wp->x + m_WindowW < SNAPDISTANCE + x + w) && (wp->x + m_WindowW > x + w - SNAPDISTANCE)) wp->x = x + w - m_WindowW;
	}

	if (wp->x < x + w && wp->x + m_WindowW > x)
	{
		if ((wp->y < SNAPDISTANCE + y) && (wp->y > y - SNAPDISTANCE)) wp->y = y;
		if ((wp->y < SNAPDISTANCE + y + h) && (wp->y > y + h - SNAPDISTANCE)) wp->y = y + h;

		if ((wp->y + m_WindowH < SNAPDISTANCE + y) && (wp->y + m_WindowH > y - SNAPDISTANCE)) wp->y = y - m_WindowH;
		if ((wp->y + m_WindowH < SNAPDISTANCE + y + h) && (wp->y + m_WindowH > y + h - SNAPDISTANCE)) wp->y = y + h - m_WindowH;
	}
}

/*
** Disables blur when Aero transparency is disabled
**
*/
LRESULT MeterWindow::OnDwmColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_BlurMode != BLURMODE_NONE && IsBlur() && c_DwmGetColorizationColor && c_DwmEnableBlurBehindWindow)
	{
		DWORD color;
		BOOL opaque;
		if (c_DwmGetColorizationColor(&color, &opaque) != S_OK)
		{
			opaque = TRUE;
		}

		BlurBehindWindow(!opaque);
	}

	return 0;
}

/*
** Disables blur when desktop composition is disabled
**
*/
LRESULT MeterWindow::OnDwmCompositionChange(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_BlurMode != BLURMODE_NONE && IsBlur() && c_DwmIsCompositionEnabled && c_DwmEnableBlurBehindWindow)
	{
		BOOL enabled;
		if (c_DwmIsCompositionEnabled(&enabled) != S_OK)
		{
			enabled = FALSE;
		}

		BlurBehindWindow(enabled);
	}

	return 0;
}

/*
** Adds the blur region to the window
**
*/
void MeterWindow::BlurBehindWindow(BOOL fEnable)
{
	if (c_DwmEnableBlurBehindWindow)
	{
		DWM_BLURBEHIND bb = {0};
		bb.fEnable = fEnable;

		if (fEnable)
		{
			// Restore blur with whatever the region was prior to disabling
			bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
			bb.hRgnBlur = m_BlurRegion;
			c_DwmEnableBlurBehindWindow(m_Window, &bb);
		}
		else
		{
			// Disable blur
			bb.dwFlags = DWM_BB_ENABLE;
			c_DwmEnableBlurBehindWindow(m_Window, &bb);
		}
	}
}

/*
** During resolution changes do nothing.
** (OnDelayedMove function is used instead.)
**
*/
LRESULT MeterWindow::OnDisplayChange(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

/*
** During setting changes do nothing.
** (OnDelayedMove function is used instead.)
**
*/
LRESULT MeterWindow::OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

/*
** Runs the action when left mouse button is down
**
*/
LRESULT MeterWindow::OnLeftButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCLBUTTONDOWN)
	{
		// Transform the point to client rect
		MapWindowPoints(nullptr, m_Window, &pos, 1);
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_DOWN);

	if (IsCtrlKeyDown() ||  // Ctrl is pressed, so only run default action
		(!DoAction(pos.x, pos.y, MOUSE_LMB_DOWN, false) && m_WindowDraggable))
	{
		// Cancel the mouse event beforehand
		SetMouseLeaveEvent(true);

		// Run the DefWindowProc so the dragging works
		return DefWindowProc(m_Window, uMsg, wParam, lParam);
	}

	return 0;
}

/*
** Runs the action when left mouse button is up
**
*/
LRESULT MeterWindow::OnLeftButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCLBUTTONUP)
	{
		// Transform the point to client rect
		MapWindowPoints(nullptr, m_Window, &pos, 1);
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_UP);

	DoAction(pos.x, pos.y, MOUSE_LMB_UP, false);

	return 0;
}

/*
** Runs the action when left mouse button is double-clicked
**
*/
LRESULT MeterWindow::OnLeftButtonDoubleClick(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCLBUTTONDBLCLK)
	{
		// Transform the point to client rect
		MapWindowPoints(nullptr, m_Window, &pos, 1);
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_DOWN);

	if (!DoAction(pos.x, pos.y, MOUSE_LMB_DBLCLK, false))
	{
		DoAction(pos.x, pos.y, MOUSE_LMB_DOWN, false);
	}

	return 0;
}

/*
** Runs the action when right mouse button is down
**
*/
LRESULT MeterWindow::OnRightButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCRBUTTONDOWN)
	{
		// Transform the point to client rect
		MapWindowPoints(nullptr, m_Window, &pos, 1);
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_MOVE);

	DoAction(pos.x, pos.y, MOUSE_RMB_DOWN, false);

	return 0;
}

/*
** Runs the action when right mouse button is up
**
*/
LRESULT MeterWindow::OnRightButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_MOVE);

	if (IsCtrlKeyDown() ||  // Ctrl is pressed, so only run default action
		!DoAction(pos.x, pos.y, MOUSE_RMB_UP, false))
	{
		// Run the DefWindowProc so the context menu works
		return DefWindowProc(m_Window, WM_RBUTTONUP, wParam, lParam);
	}

	return 0;
}

/*
** Runs the action when right mouse button is double-clicked
**
*/
LRESULT MeterWindow::OnRightButtonDoubleClick(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCRBUTTONDBLCLK)
	{
		// Transform the point to client rect
		MapWindowPoints(nullptr, m_Window, &pos, 1);
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_MOVE);

	if (!DoAction(pos.x, pos.y, MOUSE_RMB_DBLCLK, false))
	{
		DoAction(pos.x, pos.y, MOUSE_RMB_DOWN, false);
	}

	return 0;
}

/*
** Runs the action when middle mouse button is down
**
*/
LRESULT MeterWindow::OnMiddleButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCMBUTTONDOWN)
	{
		// Transform the point to client rect
		MapWindowPoints(nullptr, m_Window, &pos, 1);
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_MOVE);

	DoAction(pos.x, pos.y, MOUSE_MMB_DOWN, false);

	return 0;
}

/*
** Runs the action when middle mouse button is up
**
*/
LRESULT MeterWindow::OnMiddleButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCMBUTTONUP)
	{
		// Transform the point to client rect
		MapWindowPoints(nullptr, m_Window, &pos, 1);
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_MOVE);

	DoAction(pos.x, pos.y, MOUSE_MMB_UP, false);

	return 0;
}

/*
** Runs the action when middle mouse button is double-clicked
**
*/
LRESULT MeterWindow::OnMiddleButtonDoubleClick(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCMBUTTONDBLCLK)
	{
		// Transform the point to client rect
		MapWindowPoints(nullptr, m_Window, &pos, 1);
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_MOVE);

	if (!DoAction(pos.x, pos.y, MOUSE_MMB_DBLCLK, false))
	{
		DoAction(pos.x, pos.y, MOUSE_MMB_DOWN, false);
	}

	return 0;
}

/*
** Runs the action when a X mouse button is down
**
*/
LRESULT MeterWindow::OnXButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCXBUTTONDOWN)
	{
		// Transform the point to client rect
		MapWindowPoints(nullptr, m_Window, &pos, 1);
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_MOVE);

	if (GET_XBUTTON_WPARAM (wParam) == XBUTTON1)
	{
		DoAction(pos.x, pos.y, MOUSE_X1MB_DOWN, false);
	}
	else if (GET_XBUTTON_WPARAM (wParam) == XBUTTON2)
	{
		DoAction(pos.x, pos.y, MOUSE_X2MB_DOWN, false);
	}

	return 0;
}

/*
** Runs the action when a X mouse button is up
**
*/
LRESULT MeterWindow::OnXButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCXBUTTONUP)
	{
		// Transform the point to client rect
		MapWindowPoints(nullptr, m_Window, &pos, 1);
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_MOVE);

	if (GET_XBUTTON_WPARAM (wParam) == XBUTTON1)
	{
		DoAction(pos.x, pos.y, MOUSE_X1MB_UP, false);
	}
	else if (GET_XBUTTON_WPARAM (wParam) == XBUTTON2)
	{
		DoAction(pos.x, pos.y, MOUSE_X2MB_UP, false);
	}

	return 0;
}

/*
** Runs the action when a X mouse button is double-clicked
**
*/
LRESULT MeterWindow::OnXButtonDoubleClick(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCXBUTTONDBLCLK)
	{
		// Transform the point to client rect
		MapWindowPoints(nullptr, m_Window, &pos, 1);
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_MOVE);

	if (GET_XBUTTON_WPARAM (wParam) == XBUTTON1 &&
		!DoAction(pos.x, pos.y, MOUSE_X1MB_DBLCLK, false))
	{
		DoAction(pos.x, pos.y, MOUSE_X1MB_DOWN, false);
	}
	else if (GET_XBUTTON_WPARAM (wParam) == XBUTTON2 &&
		!DoAction(pos.x, pos.y, MOUSE_X2MB_DBLCLK, false))
	{
		DoAction(pos.x, pos.y, MOUSE_X2MB_DOWN, false);
	}

	return 0;
}

/*
** Runs the action when the MeterWindow gets or loses focus
**
*/
LRESULT MeterWindow::OnSetWindowFocus(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SETFOCUS:
		if (!m_OnFocusAction.empty())
		{
			GetRainmeter().ExecuteCommand(m_OnFocusAction.c_str(), this);
		}
		break;

	case WM_KILLFOCUS:
		if (!m_OnUnfocusAction.empty())
		{
			GetRainmeter().ExecuteCommand(m_OnUnfocusAction.c_str(), this);
		}
		break;
	}

	return 0;
}

/*
** Handles the context menu. The menu is recreated every time it is shown.
**
*/
LRESULT MeterWindow::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	RECT rect;
	GetWindowRect(m_Window, &rect);

	if ((lParam & 0xFFFFFFFF) == 0xFFFFFFFF)  // WM_CONTEXTMENU is generated from the keyboard (Shift+F10/VK_APPS)
	{
		// Set menu position to (0,0) on the window
		pos.x = rect.left;
		pos.y = rect.top;
	}
	else
	{
		pos.x = GET_X_LPARAM(lParam);
		pos.y = GET_Y_LPARAM(lParam);

		// Transform the point to client rect
		POINT posc = {pos.x - rect.left, pos.y - rect.top};

		// Handle buttons
		HandleButtons(posc, BUTTONPROC_MOVE);

		// If RMB up or RMB down or double-click cause actions, do not show the menu!
		if (!IsCtrlKeyDown() &&  // Ctrl is pressed, so ignore any actions
			(DoAction(posc.x, posc.y, MOUSE_RMB_UP, false) || DoAction(posc.x, posc.y, MOUSE_RMB_DOWN, true) || DoAction(posc.x, posc.y, MOUSE_RMB_DBLCLK, true)))
		{
			return 0;
		}
	}

	GetRainmeter().ShowContextMenu(pos, this);

	return 0;
}

/*
** Executes the action if such are defined. Returns true, if action was executed.
** If the test is true, the action is not executed.
**
*/
bool MeterWindow::DoAction(int x, int y, MOUSEACTION action, bool test)
{
	std::wstring command;

	// Check if the hitpoint was over some meter
	std::vector<Meter*>::const_reverse_iterator j = m_Meters.rbegin();
	for ( ; j != m_Meters.rend(); ++j)
	{
		// Hidden meters are ignored
		if ((*j)->IsHidden()) continue;

		const Mouse& mouse = (*j)->GetMouse();
		if (mouse.HasActionCommand(action) && (*j)->HitTest(x, y))
		{
			command = mouse.GetActionCommand(action);
			break;
		}
	}

	if (command.empty())
	{
		if (m_Mouse.HasActionCommand(action) && HitTest(x, y))
		{
			command = m_Mouse.GetActionCommand(action);
		}
	}

	if (!command.empty())
	{
		if (!test)
		{
			GetRainmeter().ExecuteCommand(command.c_str(), this);
		}

		return true;
	}

	return false;
}

/*
** Executes the action if such are defined. Returns true, if meter/window which should be processed still may exist.
**
*/
bool MeterWindow::DoMoveAction(int x, int y, MOUSEACTION action)
{
	bool buttonFound = false;

	// Check if the hitpoint was over some meter
	std::vector<Meter*>::const_reverse_iterator j = m_Meters.rbegin();
	for ( ; j != m_Meters.rend(); ++j)
	{
		if (!(*j)->IsHidden() && (*j)->HitTest(x, y))
		{
			if (action == MOUSE_OVER)
			{
				if (!m_MouseOver)
				{
					// If the mouse is over a meter it's also over the main window
					//LogDebugF(L"@Enter: %s", m_FolderPath.c_str());
					m_MouseOver = true;
					SetMouseLeaveEvent(false);
					RegisterMouseInput();

					if (!m_Mouse.GetOverAction().empty())
					{
						UINT currCounter = m_MouseMoveCounter;
						GetRainmeter().ExecuteCommand(m_Mouse.GetOverAction().c_str(), this);
						return (currCounter == m_MouseMoveCounter);
					}
				}

				// Handle button
				MeterButton* button = nullptr;
				if (m_HasButtons && (*j)->GetTypeID() == TypeID<MeterButton>())
				{
					button = (MeterButton*)(*j);
					if (button)
					{
						if (!buttonFound)
						{
							button->SetFocus(true);
							buttonFound = true;
						}
						else
						{
							button->SetFocus(false);
						}
					}
				}

				if (!(*j)->IsMouseOver())
				{
					const Mouse& mouse = (*j)->GetMouse();
					if (!mouse.GetOverAction().empty() ||
						!mouse.GetLeaveAction().empty() ||
						button)
					{
						//LogDebugF(L"MeterEnter: %s - [%s]", m_FolderPath.c_str(), (*j)->GetName());
						(*j)->SetMouseOver(true);

						if (!mouse.GetOverAction().empty())
						{
							UINT currCounter = m_MouseMoveCounter;
							GetRainmeter().ExecuteCommand(mouse.GetOverAction().c_str(), this);
							return (currCounter == m_MouseMoveCounter);
						}
					}
				}
			}
		}
		else
		{
			if (action == MOUSE_LEAVE)
			{
				if ((*j)->IsMouseOver())
				{
					// Handle button
					if (m_HasButtons && (*j)->GetTypeID() == TypeID<MeterButton>())
					{
						MeterButton* button = (MeterButton*)(*j);
						button->SetFocus(false);
					}

					//LogDebugF(L"MeterLeave: %s - [%s]", m_FolderPath.c_str(), (*j)->GetName());
					(*j)->SetMouseOver(false);

					const Mouse& mouse = (*j)->GetMouse();
					if (!mouse.GetLeaveAction().empty())
					{
						GetRainmeter().ExecuteCommand(mouse.GetLeaveAction().c_str(), this);
						return true;
					}
				}
			}
		}
	}

	if (HitTest(x, y))
	{
		// If no meters caused actions, do the default actions
		if (action == MOUSE_OVER)
		{
			if (!m_MouseOver)
			{
				//LogDebugF(L"Enter: %s", m_FolderPath.c_str());
				m_MouseOver = true;
				SetMouseLeaveEvent(false);
				RegisterMouseInput();

				if (!m_Mouse.GetOverAction().empty())
				{
					UINT currCounter = m_MouseMoveCounter;
					GetRainmeter().ExecuteCommand(m_Mouse.GetOverAction().c_str(), this);
					return (currCounter == m_MouseMoveCounter);
				}
			}
		}
	}
	else
	{
		if (action == MOUSE_LEAVE)
		{
			// Mouse leave happens when the mouse is outside the window
			if (m_MouseOver)
			{
				//LogDebugF(L"Leave: %s", m_FolderPath.c_str());
				m_MouseOver = false;
				SetMouseLeaveEvent(true);
				UnregisterMouseInput();

				if (!m_Mouse.GetLeaveAction().empty())
				{
					GetRainmeter().ExecuteCommand(m_Mouse.GetLeaveAction().c_str(), this);
					return true;
				}
			}
		}
	}

	return false;
}

/*
** Sends mouse wheel messages to the window if the window does not have focus.
**
*/
LRESULT MeterWindow::OnMouseInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos = System::GetCursorPosition();
	HWND hwnd = WindowFromPoint(pos);

	// Only process for unfocused skin window.
	if (m_Window == hwnd && m_Window != GetFocus())
	{
		RAWINPUT ri;
		UINT riSize = sizeof(ri);
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &ri, &riSize, sizeof(RAWINPUTHEADER));
		if (ri.header.dwType == RIM_TYPEMOUSE) 
		{
			WPARAM wheelDelta = MAKEWPARAM(0, HIWORD((short)ri.data.mouse.usButtonData));
			LPARAM wheelPos = MAKELPARAM(pos.x, pos.y);

			if (ri.data.mouse.usButtonFlags == RI_MOUSE_WHEEL)
			{
				OnMouseScrollMove(WM_INPUT, wheelDelta, wheelPos);
			}
			else if (ri.data.mouse.usButtonFlags == RI_MOUSE_HORIZONTAL_WHEEL)
			{
				OnMouseHScrollMove(WM_MOUSEHWHEEL, wheelDelta, wheelPos);
			}
		}
	}

	return 0;
}

/*
** Stores the new place of the window, in screen coordinates.
**
*/
LRESULT MeterWindow::OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// The lParam's x/y parameters are given in screen coordinates for overlapped and pop-up windows
	// and in parent-client coordinates for child windows.

	// Store the new window position
	m_ScreenX = GET_X_LPARAM(lParam);
	m_ScreenY = GET_Y_LPARAM(lParam);

	SetWindowPositionVariables(m_ScreenX, m_ScreenY);

	if (m_Dragging)
	{
		ScreenToWindow();
	}

	return 0;
}

/*
** Performs an action when returning from sleep
**
*/
LRESULT MeterWindow::OnWake(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (wParam == PBT_APMRESUMEAUTOMATIC && !m_OnWakeAction.empty())
	{
		GetRainmeter().ExecuteCommand(m_OnWakeAction.c_str(), this);
	}

	return 0;
}

/*
** The main window procedure for the meter window.
**
*/
LRESULT CALLBACK MeterWindow::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	MeterWindow* window = (MeterWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	BEGIN_MESSAGEPROC
	MESSAGE(OnMouseInput, WM_INPUT)
	MESSAGE(OnMove, WM_MOVE)
	MESSAGE(OnTimer, WM_TIMER)
	MESSAGE(OnCommand, WM_COMMAND)
	MESSAGE(OnSysCommand, WM_SYSCOMMAND)
	MESSAGE(OnEnterSizeMove, WM_ENTERSIZEMOVE)
	MESSAGE(OnExitSizeMove, WM_EXITSIZEMOVE)
	MESSAGE(OnNcHitTest, WM_NCHITTEST)
	MESSAGE(OnSetCursor, WM_SETCURSOR)
	MESSAGE(OnEnterMenuLoop, WM_ENTERMENULOOP)
	MESSAGE(OnMouseMove, WM_MOUSEMOVE)
	MESSAGE(OnMouseMove, WM_NCMOUSEMOVE)
	MESSAGE(OnMouseLeave, WM_MOUSELEAVE)
	MESSAGE(OnMouseLeave, WM_NCMOUSELEAVE)
	MESSAGE(OnMouseScrollMove, WM_MOUSEWHEEL)
	MESSAGE(OnMouseHScrollMove, WM_MOUSEHWHEEL)
	MESSAGE(OnContextMenu, WM_CONTEXTMENU)
	MESSAGE(OnRightButtonDown, WM_NCRBUTTONDOWN)
	MESSAGE(OnRightButtonDown, WM_RBUTTONDOWN)
	MESSAGE(OnRightButtonUp, WM_RBUTTONUP)
	MESSAGE(OnContextMenu, WM_NCRBUTTONUP)
	MESSAGE(OnRightButtonDoubleClick, WM_RBUTTONDBLCLK)
	MESSAGE(OnRightButtonDoubleClick, WM_NCRBUTTONDBLCLK)
	MESSAGE(OnLeftButtonDown, WM_NCLBUTTONDOWN)
	MESSAGE(OnLeftButtonDown, WM_LBUTTONDOWN)
	MESSAGE(OnLeftButtonUp, WM_LBUTTONUP)
	MESSAGE(OnLeftButtonUp, WM_NCLBUTTONUP)
	MESSAGE(OnLeftButtonDoubleClick, WM_LBUTTONDBLCLK)
	MESSAGE(OnLeftButtonDoubleClick, WM_NCLBUTTONDBLCLK)
	MESSAGE(OnMiddleButtonDown, WM_NCMBUTTONDOWN)
	MESSAGE(OnMiddleButtonDown, WM_MBUTTONDOWN)
	MESSAGE(OnMiddleButtonUp, WM_MBUTTONUP)
	MESSAGE(OnMiddleButtonUp, WM_NCMBUTTONUP)
	MESSAGE(OnMiddleButtonDoubleClick, WM_MBUTTONDBLCLK)
	MESSAGE(OnMiddleButtonDoubleClick, WM_NCMBUTTONDBLCLK)
	MESSAGE(OnXButtonDown, WM_XBUTTONDOWN);
	MESSAGE(OnXButtonDown, WM_NCXBUTTONDOWN);
	MESSAGE(OnXButtonUp, WM_XBUTTONUP);
	MESSAGE(OnXButtonUp, WM_NCXBUTTONUP);
	MESSAGE(OnXButtonDoubleClick, WM_XBUTTONDBLCLK);
	MESSAGE(OnXButtonDoubleClick, WM_NCXBUTTONDBLCLK);
	MESSAGE(OnWindowPosChanging, WM_WINDOWPOSCHANGING)
	MESSAGE(OnCopyData, WM_COPYDATA)
	MESSAGE(OnDelayedRefresh, WM_METERWINDOW_DELAYED_REFRESH)
	MESSAGE(OnDelayedMove, WM_METERWINDOW_DELAYED_MOVE)
	MESSAGE(OnDwmColorChange, WM_DWMCOLORIZATIONCOLORCHANGED)
	MESSAGE(OnDwmCompositionChange, WM_DWMCOMPOSITIONCHANGED)
	MESSAGE(OnSettingChange, WM_SETTINGCHANGE)
	MESSAGE(OnDisplayChange, WM_DISPLAYCHANGE)
	MESSAGE(OnSetWindowFocus, WM_SETFOCUS)
	MESSAGE(OnSetWindowFocus, WM_KILLFOCUS)
	MESSAGE(OnWake, WM_POWERBROADCAST)
	END_MESSAGEPROC
}

/*
** The initial window procedure for the meter window. Passes control to WndProc after initial setup.
**
*/
LRESULT CALLBACK MeterWindow::InitialWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_NCCREATE)
	{
		MeterWindow* window = (MeterWindow*)((LPCREATESTRUCT)lParam)->lpCreateParams;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)window);

		// Change the window procedure over to MainWndProc now that GWLP_USERDATA is set
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
		return TRUE;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
** Handles delayed refresh
**
*/
LRESULT MeterWindow::OnDelayedRefresh(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Refresh(false);
	return 0;
}

/*
** Handles delayed move.
** Do not save the position in this handler for the sake of preventing move by temporal resolution/workarea change.
**
*/
LRESULT MeterWindow::OnDelayedMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	m_Parser.ResetMonitorVariables(this);

	// Move the window temporarily
	ResizeWindow(false);
	SetWindowPos(m_Window, nullptr, m_ScreenX, m_ScreenY, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

	return 0;
}

/*
** Handles bangs from the exe
**
*/
LRESULT MeterWindow::OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	COPYDATASTRUCT* pCopyDataStruct = (COPYDATASTRUCT*)lParam;

	if (pCopyDataStruct && (pCopyDataStruct->dwData == 1) && (pCopyDataStruct->cbData > 0))
	{
		if (GetRainmeter().HasMeterWindow(this))
		{
			const WCHAR* command = (const WCHAR*)pCopyDataStruct->lpData;
			GetRainmeter().ExecuteCommand(command, this);
		}
		else
		{
			// This meterwindow has been deactivated
			LogWarning(L"Unable to bang unloaded skin");
		}

		return TRUE;
	}

	return FALSE;
}

/*
** Sets up the window position variables.
**
*/
void MeterWindow::SetWindowPositionVariables(int x, int y)
{
	WCHAR buffer[32];

	_itow_s(x, buffer, 10);
	m_Parser.SetBuiltInVariable(L"CURRENTCONFIGX", buffer);
	_itow_s(y, buffer, 10);
	m_Parser.SetBuiltInVariable(L"CURRENTCONFIGY", buffer);
}

/*
** Sets up the window size variables.
**
*/
void MeterWindow::SetWindowSizeVariables(int w, int h)
{
	WCHAR buffer[32];

	_itow_s(w, buffer, 10);
	m_Parser.SetBuiltInVariable(L"CURRENTCONFIGWIDTH", buffer);
	_itow_s(h, buffer, 10);
	m_Parser.SetBuiltInVariable(L"CURRENTCONFIGHEIGHT", buffer);
}

/*
** Converts the path to absolute by adding the skin's path to it (unless it already is absolute).
**
*/
void MeterWindow::MakePathAbsolute(std::wstring& path)
{
	if (path.empty() || PathUtil::IsAbsolute(path))
	{
		return;  // It's already absolute path (or it's empty)
	}
	else
	{
		std::wstring absolute;
		absolute.reserve(GetRainmeter().GetSkinPath().size() + m_FolderPath.size() + 1 + path.size());
		absolute = GetRainmeter().GetSkinPath();
		absolute += m_FolderPath;
		absolute += L'\\';
		absolute += path;
		absolute.swap(path);
	}
}

std::wstring MeterWindow::GetFilePath()
{
	std::wstring file = GetRainmeter().GetSkinPath() + m_FolderPath;
	file += L'\\';
	file += m_FileName;
	return file;
}

std::wstring MeterWindow::GetRootPath()
{
	std::wstring path = GetRainmeter().GetSkinPath();

	std::wstring::size_type loc;
	if ((loc = m_FolderPath.find_first_of(L'\\')) != std::wstring::npos)
	{
		path.append(m_FolderPath, 0, loc + 1);
	}
	else
	{
		path += m_FolderPath;
		path += L'\\';
	}

	return path;
}

std::wstring MeterWindow::GetResourcesPath()
{
	std::wstring path = GetRootPath();
	path += L"@Resources\\";
	return path;
}

std::wstring MeterWindow::GetSkinPath()
{
	std::wstring path;
	if (!m_FolderPath.empty())
	{
		path += m_FolderPath;
		path += L"\\";
	}

	path += m_FileName;
	return path;
}

Meter* MeterWindow::GetMeter(const std::wstring& meterName)
{
	const WCHAR* name = meterName.c_str();
	std::vector<Meter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		if (_wcsicmp((*j)->GetName(), name) == 0)
		{
			return (*j);
		}
	}
	return nullptr;
}
