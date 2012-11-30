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

int CMeterWindow::c_InstanceCount = 0;

HINSTANCE CMeterWindow::c_DwmInstance = NULL;
FPDWMENABLEBLURBEHINDWINDOW CMeterWindow::c_DwmEnableBlurBehindWindow = NULL;
FPDWMGETCOLORIZATIONCOLOR CMeterWindow::c_DwmGetColorizationColor = NULL;
FPDWMSETWINDOWATTRIBUTE CMeterWindow::c_DwmSetWindowAttribute = NULL;
FPDWMISCOMPOSITIONENABLED CMeterWindow::c_DwmIsCompositionEnabled = NULL;

extern CRainmeter* Rainmeter;

/*
** Constructor
**
*/
CMeterWindow::CMeterWindow(const std::wstring& folderPath, const std::wstring& file) : m_FolderPath(folderPath), m_FileName(file),
	m_DoubleBuffer(),
	m_DIBSectionBuffer(),
	m_DIBSectionBufferPixels(),
	m_DIBSectionBufferW(),
	m_DIBSectionBufferH(),
	m_Background(),
	m_BackgroundSize(),
	m_Window(),
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
	m_Refreshing(false),
	m_Hidden(false),
	m_ResizeWindow(RESIZEMODE_NONE),
	m_UpdateCounter(),
	m_MouseMoveCounter(),
	m_FontCollection(),
	m_ToolTipHidden(false)
{
	if (!c_DwmInstance && CSystem::GetOSPlatform() >= OSPLATFORM_VISTA)
	{
		c_DwmInstance = CSystem::RmLoadLibrary(L"dwmapi.dll");
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
		wc.hInstance = Rainmeter->GetInstance();
		wc.hCursor = NULL;  // The cursor should be controlled by using SetCursor() when needed.
		wc.lpszClassName = METERWINDOW_CLASS_NAME;
		RegisterClassEx(&wc);
	}

	++c_InstanceCount;
}

/*
** Destructor
**
*/
CMeterWindow::~CMeterWindow()
{
	if (!m_OnCloseAction.empty())
	{
		Rainmeter->ExecuteCommand(m_OnCloseAction.c_str(), this);
	}

	Dispose(false);

	--c_InstanceCount;

	if (c_InstanceCount == 0)
	{
		UnregisterClass(METERWINDOW_CLASS_NAME, Rainmeter->GetInstance());

		if (c_DwmInstance)
		{
			FreeLibrary(c_DwmInstance);
			c_DwmInstance = NULL;

			c_DwmEnableBlurBehindWindow = NULL;
			c_DwmGetColorizationColor = NULL;
			c_DwmSetWindowAttribute = NULL;
			c_DwmIsCompositionEnabled = NULL;
		}
	}
}

/*
** Kills timers/hooks and disposes buffers
**
*/
void CMeterWindow::Dispose(bool refresh)
{
	// Kill the timer/hook
	KillTimer(m_Window, TIMER_METER);
	KillTimer(m_Window, TIMER_MOUSE);
	KillTimer(m_Window, TIMER_FADE);
	KillTimer(m_Window, TIMER_TRANSITION);

	UnregisterMouseInput();

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
	m_Background = NULL;

	m_BackgroundSize.cx = m_BackgroundSize.cy = 0;
	m_BackgroundName.clear();

	if (m_BlurRegion)
	{
		DeleteObject(m_BlurRegion);
		m_BlurRegion = NULL;
	}

	if (m_FontCollection)
	{
		CMeterString::FreeFontCache(m_FontCollection);
		delete m_FontCollection;
		m_FontCollection = NULL;
	}

	if (!refresh)
	{
		// Destroy double buffers and window
		delete m_DoubleBuffer;
		m_DoubleBuffer = NULL;

		if (m_DIBSectionBuffer)
		{
			DeleteObject(m_DIBSectionBuffer);
			m_DIBSectionBuffer = NULL;
		}

		if (m_Window)
		{
			DestroyWindow(m_Window);
			m_Window = NULL;
		}
	}
}

/*
** Initializes the window, creates the class and the window.
**
*/
void CMeterWindow::Initialize()
{
	m_Window = CreateWindowEx(
		WS_EX_TOOLWINDOW,
		METERWINDOW_CLASS_NAME,
		NULL,
		WS_POPUP,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		Rainmeter->GetInstance(),
		this);

	setlocale(LC_NUMERIC, "C");

	// Mark the window to ignore the Aero peek
	IgnoreAeroPeek();

	// Gotta have some kind of buffer during initialization
	CreateDoubleBuffer(1, 1);

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
void CMeterWindow::IgnoreAeroPeek()
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
void CMeterWindow::RegisterMouseInput()
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

void CMeterWindow::UnregisterMouseInput()
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

void CMeterWindow::AddWindowExStyle(LONG_PTR flag)
{
	LONG_PTR style = GetWindowLongPtr(m_Window, GWL_EXSTYLE);
	if ((style & flag) == 0)
	{
		SetWindowLongPtr(m_Window, GWL_EXSTYLE, style | flag);
	}
}

void CMeterWindow::RemoveWindowExStyle(LONG_PTR flag)
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
void CMeterWindow::Deactivate()
{
	HideFade();
	SetTimer(m_Window, TIMER_DEACTIVATE, m_FadeDuration + 50, NULL);
}

/*
** Rebuilds the skin.
**
*/
void CMeterWindow::Refresh(bool init, bool all)
{
	assert(Rainmeter != NULL);

	Rainmeter->SetCurrentParser(&m_Parser);

	std::wstring notice = L"Refreshing skin \"" + m_FolderPath;
	notice += L'\\';
	notice += m_FileName;
	notice += L'"';
	Log(LOG_NOTICE, notice.c_str());

	m_Refreshing = true;
	SetResizeWindowMode(RESIZEMODE_RESET);

	if (!init)
	{
		Dispose(true);
	}

	ZPOSITION oldZPos = m_WindowZPosition;

	if (!ReadSkin())
	{
		Rainmeter->DeactivateSkin(this, -1);
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

	SetWindowPos(m_Window, NULL, m_ScreenX, m_ScreenY, m_WindowW, m_WindowH, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

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
		SetTimer(m_Window, TIMER_METER, m_WindowUpdate, NULL);
	}

	SetTimer(m_Window, TIMER_MOUSE, INTERVAL_MOUSE, NULL);

	Rainmeter->SetCurrentParser(NULL);

	m_Refreshing = false;

	if (!m_OnRefreshAction.empty())
	{
		Rainmeter->ExecuteCommand(m_OnRefreshAction.c_str(), this);
	}
}

void CMeterWindow::SetMouseLeaveEvent(bool cancel)
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

void CMeterWindow::MapCoordsToScreen(int& x, int& y, int w, int h)
{
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

		HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);

		if (hMonitor != NULL)
		{
			MONITORINFO mi;
			mi.cbSize = sizeof(mi);
			GetMonitorInfo(hMonitor, &mi);

			x = min(x, mi.rcMonitor.right - m_WindowW);
			x = max(x, mi.rcMonitor.left);
			y = min(y, mi.rcMonitor.bottom - m_WindowH);
			y = max(y, mi.rcMonitor.top);
			return;
		}
	}

	// No monitor found for the window -> Use the default work area
	RECT workArea;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);	// Store the old value
	x = min(x, workArea.right - m_WindowW);
	x = max(x, workArea.left);
	y = min(y, workArea.bottom - m_WindowH);
	y = max(y, workArea.top);
}

/*
** Moves the window to a new place (on the virtual screen)
**
*/
void CMeterWindow::MoveWindow(int x, int y)
{
	SetWindowPos(m_Window, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

	ScreenToWindow();

	if (m_SavePosition)
	{
		WriteOptions(OPTION_POSITION);
	}
}

/*
** Sets the window's z-position
**
*/
void CMeterWindow::ChangeZPos(ZPOSITION zPos, bool all)
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
			if (CSystem::GetShowDesktop())
			{
				// Insert after the system window temporarily to keep order
				winPos = CSystem::GetWindow();
			}
			else
			{
				// Insert after the helper window
				winPos = CSystem::GetHelperWindow();
			}
		}
		else
		{
			winPos = HWND_BOTTOM;
		}
		break;

	case ZPOSITION_NORMAL:
		if (all || !Rainmeter->IsNormalStayDesktop()) break;
	case ZPOSITION_ONDESKTOP:
		if (CSystem::GetShowDesktop())
		{
			winPos = CSystem::GetHelperWindow();

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
				winPos = CSystem::GetHelperWindow();
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
void CMeterWindow::ChangeSingleZPos(ZPOSITION zPos, bool all)
{
	if (zPos == ZPOSITION_NORMAL && Rainmeter->IsNormalStayDesktop() && (!all || CSystem::GetShowDesktop()))
	{
		m_WindowZPosition = zPos;

		// Set window on top of all other ZPOSITION_ONDESKTOP, ZPOSITION_BOTTOM, and ZPOSITION_NORMAL windows
		SetWindowPos(m_Window, CSystem::GetBackmostTopWindow(), 0, 0, 0, 0, ZPOS_FLAGS);

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
** Correct number of arguments must be passed (or use CRainmeter::ExecuteBang).
*/
void CMeterWindow::RunBang(BANGCOMMAND bang, const std::vector<std::wstring>& args)
{
	if (!m_Window) return;

	switch (bang)
	{
	case BANG_REFRESH:
		// Refresh needs to be delayed since it crashes if done during Update()
		PostMessage(m_Window, WM_METERWINDOW_DELAYED_REFRESH, (WPARAM)NULL, (LPARAM)NULL);
		break;

	case BANG_REDRAW:
		Redraw();
		break;

	case BANG_UPDATE:
		KillTimer(m_Window, TIMER_METER);  // Kill timer temporarily
		Update(false);
		if (m_WindowUpdate >= 0)
		{
			SetTimer(m_Window, TIMER_METER, m_WindowUpdate, NULL);
		}
		break;

	case BANG_SHOWBLUR:
		ShowBlur();
		break;

	case BANG_HIDEBLUR:
		HideBlur();
		break;

	case BANG_TOGGLEBLUR:
		RunBang(IsBlur() ? BANG_HIDEBLUR : BANG_SHOWBLUR, args);
		break;

	case BANG_ADDBLUR:
		ResizeBlur(args[0], RGN_OR);
		if (IsBlur()) ShowBlur();
		break;

	case BANG_REMOVEBLUR:
		ResizeBlur(args[0], RGN_DIFF);
		if (IsBlur()) ShowBlur();
		break;

	case BANG_TOGGLEMETER:
		ToggleMeter(args[0]);
		break;

	case BANG_SHOWMETER:
		ShowMeter(args[0]);
		break;

	case BANG_HIDEMETER:
		HideMeter(args[0]);
		break;

	case BANG_UPDATEMETER:
		UpdateMeter(args[0]);
		break;

	case BANG_TOGGLEMETERGROUP:
		ToggleMeter(args[0], true);
		break;

	case BANG_SHOWMETERGROUP:
		ShowMeter(args[0], true);
		break;

	case BANG_HIDEMETERGROUP:
		HideMeter(args[0], true);
		break;

	case BANG_UPDATEMETERGROUP:
		UpdateMeter(args[0], true);
		break;

	case BANG_TOGGLEMEASURE:
		ToggleMeasure(args[0]);
		break;

	case BANG_ENABLEMEASURE:
		EnableMeasure(args[0]);
		break;

	case BANG_DISABLEMEASURE:
		DisableMeasure(args[0]);
		break;

	case BANG_UPDATEMEASURE:
		UpdateMeasure(args[0]);
		CDialogAbout::UpdateMeasures(this);
		break;

	case BANG_DISABLEMEASUREGROUP:
		DisableMeasure(args[0], true);
		break;

	case BANG_TOGGLEMEASUREGROUP:
		ToggleMeasure(args[0], true);
		break;

	case BANG_ENABLEMEASUREGROUP:
		EnableMeasure(args[0], true);
		break;

	case BANG_UPDATEMEASUREGROUP:
		UpdateMeasure(args[0], true);
		CDialogAbout::UpdateMeasures(this);
		break;

	case BANG_SHOW:
		m_Hidden = false;
		ShowWindow(m_Window, SW_SHOWNOACTIVATE);
		UpdateWindow((m_WindowHide == HIDEMODE_FADEOUT) ? 255 : m_AlphaValue, false);
		break;

	case BANG_HIDE:
		m_Hidden = true;
		ShowWindow(m_Window, SW_HIDE);
		break;

	case BANG_TOGGLE:
		RunBang(m_Hidden ? BANG_SHOW : BANG_HIDE, args);
		break;

	case BANG_SHOWFADE:
		ShowFade();
		break;

	case BANG_HIDEFADE:
		HideFade();
		break;

	case BANG_TOGGLEFADE:
		RunBang(m_Hidden ? BANG_SHOWFADE : BANG_HIDEFADE, args);
		break;

	case BANG_MOVE:
		{
			int x = m_Parser.ParseInt(args[0].c_str(), 0);
			int y = m_Parser.ParseInt(args[1].c_str(), 0);
			MoveWindow(x, y);
		}
		break;

	case BANG_ZPOS:
		SetWindowZPosition((ZPOSITION)m_Parser.ParseInt(args[0].c_str(), 0));
		break;

	case BANG_CLICKTHROUGH:
		{
			int f = m_Parser.ParseInt(args[0].c_str(), 0);
			SetClickThrough((f == -1) ? !m_ClickThrough : f);
		}
		break;

	case BANG_DRAGGABLE:
		{
			int f = m_Parser.ParseInt(args[0].c_str(), 0);
			SetWindowDraggable((f == -1) ? !m_WindowDraggable : f);
		}
		break;

	case BANG_SNAPEDGES:
		{
			int f = m_Parser.ParseInt(args[0].c_str(), 0);
			SetSnapEdges((f == -1) ? !m_SnapEdges : f);
		}
		break;

	case BANG_KEEPONSCREEN:
		{
			int f = m_Parser.ParseInt(args[0].c_str(), 0);
			SetKeepOnScreen((f == -1) ? !m_KeepOnScreen : f);
		}
		break;

	case BANG_SETTRANSPARENCY:
		{
			const std::wstring& arg = args[0];
			m_AlphaValue = CConfigParser::ParseInt(arg.c_str(), 255);
			m_AlphaValue = max(m_AlphaValue, 0);
			m_AlphaValue = min(m_AlphaValue, 255);
			UpdateWindow(m_AlphaValue, false);
		}
		break;

	case BANG_MOVEMETER:
		{
			int x = m_Parser.ParseInt(args[0].c_str(), 0);
			int y = m_Parser.ParseInt(args[1].c_str(), 0);
			MoveMeter(args[2], x, y);
		}
		break;

	case BANG_COMMANDMEASURE:
		{
			const std::wstring& measure = args[0];
			CMeasure* m = GetMeasure(measure);
			if (m)
			{
				m->Command(args[1]);
			}
			else
			{
				LogWithArgs(LOG_WARNING, L"!CommandMeasure: [%s] not found", measure.c_str());
			}
		}
		break;

	case BANG_PLUGIN:
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
				CMeasure* m = GetMeasure(measure);
				if (m)
				{
					m->Command(arg);
					return;
				}

				LogWithArgs(LOG_WARNING, L"!PluginBang: [%s] not found", measure.c_str());
			}
			else
			{
				Log(LOG_ERROR, L"!PluginBang: Invalid parameters");
			}
		}
		break;

	case BANG_SETVARIABLE:
		SetVariable(args[0], args[1]);
		break;

	case BANG_SETOPTION:
		SetOption(args[0], args[1], args[2], false);
		break;

	case BANG_SETOPTIONGROUP:
		SetOption(args[0], args[1], args[2], true);
		break;
	}
}

/*
** Enables blurring of the window background (using Aero)
**
*/
void CMeterWindow::ShowBlur()
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
void CMeterWindow::HideBlur()
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
void CMeterWindow::ResizeBlur(const std::wstring& arg, int mode)
{
	if (CSystem::GetOSPlatform() >= OSPLATFORM_VISTA)
	{
		WCHAR* parseSz = _wcsdup(arg.c_str());
		int type, x, y, w = 0, h = 0;

		WCHAR* token = wcstok(parseSz, L",");
		if (token)
		{
			while (token[0] == L' ') ++token;
			type = m_Parser.ParseInt(token, 0);

			token = wcstok(NULL, L",");
			if (token)
			{
				while (token[0] == L' ') ++token;
				x = m_Parser.ParseInt(token, 0);

				token = wcstok(NULL, L",");
				if (token)
				{
					while (token[0] == L' ') ++token;
					y = m_Parser.ParseInt(token, 0);

					token = wcstok(NULL, L",");
					if (token)
					{
						while (token[0] == L' ') ++token;
						w = m_Parser.ParseInt(token, 0);

						token = wcstok(NULL, L",");
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
				token = wcstok(NULL, L",");
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
bool CompareName(const CSection* section, const WCHAR* name, bool group)
{
	return (group) ? section->BelongsToGroup(name) : (_wcsicmp(section->GetName(), name) == 0);
}

/*
** Shows the given meter
**
*/
void CMeterWindow::ShowMeter(const std::wstring& name, bool group)
{
	const WCHAR* meter = name.c_str();

	std::vector<CMeter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		if (CompareName((*j), meter, group))
		{
			(*j)->Show();
			SetResizeWindowMode(RESIZEMODE_CHECK);	// Need to recalculate the window size
			if (!group) return;
		}
	}

	if (!group) LogWithArgs(LOG_ERROR, L"!ShowMeter: [%s] not found in \"%s\"", meter, m_FolderPath.c_str());
}

/*
** Hides the given meter
**
*/
void CMeterWindow::HideMeter(const std::wstring& name, bool group)
{
	const WCHAR* meter = name.c_str();

	std::vector<CMeter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		if (CompareName((*j), meter, group))
		{
			(*j)->Hide();
			SetResizeWindowMode(RESIZEMODE_CHECK);	// Need to recalculate the window size
			if (!group) return;
		}
	}

	if (!group) LogWithArgs(LOG_ERROR, L"!HideMeter: [%s] not found in \"%s\"", meter, m_FolderPath.c_str());
}

/*
** Toggles the given meter
**
*/
void CMeterWindow::ToggleMeter(const std::wstring& name, bool group)
{
	const WCHAR* meter = name.c_str();

	std::vector<CMeter*>::const_iterator j = m_Meters.begin();
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

	if (!group) LogWithArgs(LOG_ERROR, L"!ToggleMeter: [%s] not found in \"%s\"", meter, m_FolderPath.c_str());
}

/*
** Moves the given meter
**
*/
void CMeterWindow::MoveMeter(const std::wstring& name, int x, int y)
{
	const WCHAR* meter = name.c_str();

	std::vector<CMeter*>::const_iterator j = m_Meters.begin();
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

	LogWithArgs(LOG_ERROR, L"!MoveMeter: [%s] not found in \"%s\"", meter, m_FolderPath.c_str());
}

/*
** Updates the given meter
**
*/
void CMeterWindow::UpdateMeter(const std::wstring& name, bool group)
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
			UpdateMeter((*j), bActiveTransition, true);
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

	if (!group && bContinue) LogWithArgs(LOG_ERROR, L"!UpdateMeter: [%s] not found in \"%s\"", meter, m_FolderPath.c_str());
}

/*
** Enables the given measure
**
*/
void CMeterWindow::EnableMeasure(const std::wstring& name, bool group)
{
	const WCHAR* measure = name.c_str();

	std::vector<CMeasure*>::const_iterator i = m_Measures.begin();
	for ( ; i != m_Measures.end(); ++i)
	{
		if (CompareName((*i), measure, group))
		{
			(*i)->Enable();
			if (!group) return;
		}
	}

	if (!group) LogWithArgs(LOG_ERROR, L"!EnableMeasure: [%s] not found in \"%s\"", measure, m_FolderPath.c_str());
}

/*
** Disables the given measure
**
*/
void CMeterWindow::DisableMeasure(const std::wstring& name, bool group)
{
	const WCHAR* measure = name.c_str();

	std::vector<CMeasure*>::const_iterator i = m_Measures.begin();
	for ( ; i != m_Measures.end(); ++i)
	{
		if (CompareName((*i), measure, group))
		{
			(*i)->Disable();
			if (!group) return;
		}
	}

	if (!group) LogWithArgs(LOG_ERROR, L"!DisableMeasure: [%s] not found in \"%s\"", measure, m_FolderPath.c_str());
}

/*
** Toggless the given measure
**
*/
void CMeterWindow::ToggleMeasure(const std::wstring& name, bool group)
{
	const WCHAR* measure = name.c_str();

	std::vector<CMeasure*>::const_iterator i = m_Measures.begin();
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

	if (!group) LogWithArgs(LOG_ERROR, L"!ToggleMeasure: [%s] not found in \"%s\"", measure, m_FolderPath.c_str());
}

/*
** Updates the given measure
**
*/
void CMeterWindow::UpdateMeasure(const std::wstring& name, bool group)
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
			if (bNetStats && (*i)->GetTypeID() == TypeID<CMeasureNet>())
			{
				CMeasureNet::UpdateIFTable();
				CMeasureNet::UpdateStats();
				bNetStats = false;
			}

			UpdateMeasure((*i), true);
			if (!group) return;
		}
	}

	if (!group) LogWithArgs(LOG_ERROR, L"!UpdateMeasure: [%s] not found in \"%s\"", measure, m_FolderPath.c_str());
}

/*
** Sets variable to given value.
**
*/
void CMeterWindow::SetVariable(const std::wstring& variable, const std::wstring& value)
{
	double result;
	if (m_Parser.ParseFormula(value, &result))
	{
		WCHAR buffer[256];
		int len = _snwprintf_s(buffer, _TRUNCATE, L"%.5f", result);
		CMeasure::RemoveTrailingZero(buffer, len);

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
void CMeterWindow::SetOption(const std::wstring& section, const std::wstring& option, const std::wstring& value, bool group)
{
	auto setValue = [&](CSection* section, const std::wstring& option, const std::wstring& value)
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
		CMeter* meter = GetMeter(section);
		if (meter)
		{
			setValue(meter, option, value);
			return;
		}

		CMeasure* measure = GetMeasure(section);
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
void CMeterWindow::WindowToScreen()
{
	if (CSystem::GetMonitorCount() == 0)
	{
		Log(LOG_ERROR, L"No monitors (WindowToScreen)");
		return;
	}

	std::wstring::size_type index, index2;
	int pixel = 0;
	float num;
	int screenx, screeny, screenh, screenw;

	const MultiMonitorInfo& multimonInfo = CSystem::GetMultiMonitorInfo();
	const std::vector<MonitorInfo>& monitors = multimonInfo.monitors;

	// Clear position flags
	m_WindowXScreen = m_WindowYScreen = multimonInfo.primary; // Default to primary screen
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
			if (screenIndex >= 0 && (screenIndex == 0 || screenIndex <= (int)monitors.size() && monitors[screenIndex-1].active))
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
		screenx = multimonInfo.vsL;
		screenw = multimonInfo.vsW;
	}
	else
	{
		screenx = monitors[m_WindowXScreen-1].screen.left;
		screenw = monitors[m_WindowXScreen-1].screen.right - monitors[m_WindowXScreen-1].screen.left;
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
			if (screenIndex >= 0 && (screenIndex == 0 || screenIndex <= (int)monitors.size() && monitors[screenIndex-1].active))
			{
				m_WindowYScreen = screenIndex;
				m_WindowYScreenDefined = true;
			}
		}
	}
	if (m_WindowYScreen == 0)
	{
		screeny = multimonInfo.vsT;
		screenh = multimonInfo.vsH;
	}
	else
	{
		screeny = monitors[m_WindowYScreen-1].screen.top;
		screenh = monitors[m_WindowYScreen-1].screen.bottom - monitors[m_WindowYScreen-1].screen.top;
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
void CMeterWindow::ScreenToWindow()
{
	WCHAR buffer[256];
	int pixel = 0;
	float num;
	int screenx, screeny, screenh, screenw;

	const MultiMonitorInfo& multimonInfo = CSystem::GetMultiMonitorInfo();
	const std::vector<MonitorInfo>& monitors = multimonInfo.monitors;

	if (monitors.empty())
	{
		Log(LOG_ERROR, L"No monitors (ScreenToWindow)");
		return;
	}

	// Correct to auto-selected screen
	if (m_AutoSelectScreen)
	{
		RECT rect = {m_ScreenX, m_ScreenY, m_ScreenX + m_WindowW, m_ScreenY + m_WindowH};
		HMONITOR hMonitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);

		if (hMonitor != NULL)
		{
			for (size_t i = 0, isize = monitors.size(); i < isize; ++i)
			{
				if (monitors[i].active && monitors[i].handle == hMonitor)
				{
					int screenIndex = (int)i + 1;
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
		screenx = multimonInfo.vsL;
		screenw = multimonInfo.vsW;
	}
	else
	{
		screenx = monitors[m_WindowXScreen-1].screen.left;
		screenw = monitors[m_WindowXScreen-1].screen.right - monitors[m_WindowXScreen-1].screen.left;
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
		_snwprintf_s(buffer, _TRUNCATE, L"%sR", buffer);
	}
	if (m_WindowXScreenDefined)
	{
		_snwprintf_s(buffer, _TRUNCATE, L"%s@%i", buffer, m_WindowXScreen);
	}
	m_WindowX = buffer;

	// --- Calculate WindowY ---

	if (m_WindowYScreen == 0)
	{
		screeny = multimonInfo.vsT;
		screenh = multimonInfo.vsH;
	}
	else
	{
		screeny = monitors[m_WindowYScreen-1].screen.top;
		screenh = monitors[m_WindowYScreen-1].screen.bottom - monitors[m_WindowYScreen-1].screen.top;
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
		_snwprintf_s(buffer, _TRUNCATE, L"%sB", buffer);
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
void CMeterWindow::ReadOptions()
{
	WCHAR buffer[32];

	const WCHAR* section = m_FolderPath.c_str();
	CConfigParser parser;
	parser.Initialize(Rainmeter->GetIniFile(), NULL, section);

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

	m_WindowDraggable = 0!=parser.ReadInt(section, L"Draggable", 1);
	addWriteFlag(OPTION_DRAGGABLE);

	m_SnapEdges = 0!=parser.ReadInt(section, L"SnapEdges", 1);
	addWriteFlag(OPTION_SNAPEDGES);

	m_ClickThrough = 0!=parser.ReadInt(section, L"ClickThrough", 0);
	addWriteFlag(OPTION_CLICKTHROUGH);

	m_KeepOnScreen = 0!=parser.ReadInt(section, L"KeepOnScreen", 1);
	addWriteFlag(OPTION_KEEPONSCREEN);

	m_SavePosition = 0!=parser.ReadInt(section, L"SavePosition", 1);
	m_WindowStartHidden = 0!=parser.ReadInt(section, L"StartHidden", 0);
	m_AutoSelectScreen = 0!=parser.ReadInt(section, L"AutoSelectScreen", 0);

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
void CMeterWindow::WriteOptions(INT setting)
{
	const WCHAR* iniFile = Rainmeter->GetIniFile().c_str();

	if (*iniFile)
	{
		WCHAR buffer[32];
		const WCHAR* section = m_FolderPath.c_str();

		if (setting != OPTION_ALL)
		{
			CDialogManage::UpdateSkins(this);
		}

		if (setting & OPTION_POSITION)
		{
			// If position needs to be save, do so.
			if (m_SavePosition)
			{
				ScreenToWindow();
				WritePrivateProfileString(section, L"WindowX", m_WindowX.c_str(), iniFile);
				WritePrivateProfileString(section, L"WindowY", m_WindowY.c_str(), iniFile);
			}
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
bool CMeterWindow::ReadSkin()
{
	WCHAR buffer[128];

	std::wstring iniFile = GetFilePath();

	// Verify whether the file exists
	if (_waccess(iniFile.c_str(), 0) == -1)
	{
		std::wstring message = GetFormattedString(ID_STR_UNABLETOREFRESHSKIN, m_FolderPath.c_str(), m_FileName.c_str());
		Rainmeter->ShowMessage(m_Window, message.c_str(), MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	std::wstring resourcePath = GetResourcesPath();
	bool hasResourcesFolder = (_waccess(resourcePath.c_str(), 0) == 0);

	m_Parser.Initialize(iniFile, this, NULL, &resourcePath);

	// Read options from Rainmeter.ini.
	ReadOptions();

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
		Rainmeter->ShowMessage(m_Window, text.c_str(), MB_OK | MB_ICONEXCLAMATION);
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

	m_DynamicWindowSize = 0!=m_Parser.ReadInt(L"Rainmeter", L"DynamicWindowSize", 0);

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

	m_Mouse.ReadOptions(m_Parser, L"Rainmeter", this);

	m_OnRefreshAction = m_Parser.ReadString(L"Rainmeter", L"OnRefreshAction", L"", false);
	m_OnCloseAction = m_Parser.ReadString(L"Rainmeter", L"OnCloseAction", L"", false);
	m_OnFocusAction = m_Parser.ReadString(L"Rainmeter", L"OnFocusAction", L"", false);
	m_OnUnfocusAction = m_Parser.ReadString(L"Rainmeter", L"OnUnfocusAction", L"", false);

	m_WindowUpdate = m_Parser.ReadInt(L"Rainmeter", L"Update", INTERVAL_METER);
	m_TransitionUpdate = m_Parser.ReadInt(L"Rainmeter", L"TransitionUpdate", INTERVAL_TRANSITION);
	m_ToolTipHidden = 0 != m_Parser.ReadInt(L"Rainmeter", L"ToolTipHidden", 0);

	if (CSystem::GetOSPlatform() >= OSPLATFORM_VISTA)
	{
		if (0 != m_Parser.ReadInt(L"Rainmeter", L"Blur", 0))
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
			(CSystem::GetOSPlatform() >= OSPLATFORM_7) ? FindExInfoBasic : FindExInfoStandard,
			&fd,
			FindExSearchNameMatch,
			NULL,
			0);

		if (find != INVALID_HANDLE_VALUE)
		{
			m_FontCollection = new PrivateFontCollection();

			do
			{
				if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					std::wstring file(resourcePath, 0, resourcePath.length() - 1);
					file += fd.cFileName;
					Status status = m_FontCollection->AddFontFile(file.c_str());
					if (status != Ok)
					{
						std::wstring error = L"Unable to load font: ";
						error += file.c_str();
						Log(LOG_ERROR, error.c_str());
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
			m_FontCollection = new PrivateFontCollection();
		}

		int i = 1;
		do
		{
			// Try program folder first
			std::wstring szFontFile = Rainmeter->GetPath() + L"Fonts\\";
			szFontFile += localFont;
			Status status = m_FontCollection->AddFontFile(szFontFile.c_str());
			if (status != Ok)
			{
				szFontFile = localFont;
				MakePathAbsolute(szFontFile);
				status = m_FontCollection->AddFontFile(szFontFile.c_str());
				if (status != Ok)
				{
					std::wstring error = L"Unable to load font: ";
					error += localFont;
					Log(LOG_ERROR, error.c_str());
				}
			}

			// Check for LocalFont2, LocalFont3, etc.
			_snwprintf_s(buffer, _TRUNCATE, L"LocalFont%i", ++i);
			localFont = m_Parser.ReadString(L"Rainmeter", buffer, L"").c_str();
		}
		while (*localFont);
	}

	// Create all meters and measures.
	m_HasNetMeasures = false;
	m_HasButtons = false;
	CMeter* prevMeter = NULL;
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
				CMeasure* measure = CMeasure::Create(measureName.c_str(), this, section);
				if (measure)
				{
					m_Measures.push_back(measure);
					m_Parser.AddMeasure(measure);

					if (measure->GetTypeID() == TypeID<CMeasureNet>())
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
				CMeter* meter = CMeter::Create(meterName.c_str(), this, section);
				if (meter)
				{
					m_Meters.push_back(meter);
					meter->SetRelativeMeter(prevMeter);

					if (!m_HasButtons && meter->GetTypeID() == TypeID<CMeterButton>())
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
		Rainmeter->ShowMessage(m_Window, text.c_str(), MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	// Initialize meters. Separate loop to avoid errors caused with section
	// variables for nonexistent measures/meters.
	for (auto iter = m_Meters.cbegin(); iter != m_Meters.cend(); ++iter)
	{
		CMeter* meter = *iter;
		meter->ReadOptions(m_Parser);
		meter->Initialize();

		if (!meter->GetToolTipText().empty())
		{
			meter->CreateToolTip(this);
		}
	}

	// Initialize measures. Separate loop to avoid errors caused by
	// referencing nonexistent [measures] in the measure options.
	for (auto iter = m_Measures.cbegin(); iter != m_Measures.cend(); ++iter)
	{
		CMeasure* measure = *iter;
		measure->ReadOptions(m_Parser);
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
bool CMeterWindow::ResizeWindow(bool reset)
{
	int w = m_BackgroundMargins.left;
	int h = m_BackgroundMargins.top;

	// Get the largest meter point
	std::vector<CMeter*>::const_iterator j = m_Meters.begin();
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
	m_Background = NULL;

	if ((m_BackgroundMode == BGMODE_IMAGE || m_BackgroundMode == BGMODE_SCALED_IMAGE || m_BackgroundMode == BGMODE_TILED_IMAGE) && !m_BackgroundName.empty())
	{
		// Load the background
		CTintedImage* tintedBackground = new CTintedImage(L"Background");
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
void CMeterWindow::CreateDoubleBuffer(int cx, int cy)
{
	// Create DIBSection bitmap
	BITMAPV4HEADER bmiHeader = {sizeof(BITMAPV4HEADER)};
	bmiHeader.bV4Width = cx;
	bmiHeader.bV4Height = -cy;  // top-down DIB
	bmiHeader.bV4Planes = 1;
	bmiHeader.bV4BitCount = 32;
	bmiHeader.bV4V4Compression = BI_BITFIELDS;
	bmiHeader.bV4RedMask = 0x00FF0000;
	bmiHeader.bV4GreenMask = 0x0000FF00;
	bmiHeader.bV4BlueMask = 0x000000FF;
	bmiHeader.bV4AlphaMask = 0xFF000000;

	m_DIBSectionBuffer = CreateDIBSection(NULL, (BITMAPINFO*)&bmiHeader, DIB_RGB_COLORS, (void**)&m_DIBSectionBufferPixels, NULL, 0);

	// Create GDI+ bitmap from DIBSection's pixels
	m_DoubleBuffer = new Bitmap(cx, cy, cx * 4, PixelFormat32bppPARGB, (BYTE*)m_DIBSectionBufferPixels);

	m_DIBSectionBufferW = cx;
	m_DIBSectionBufferH = cy;
}

/*
** Redraws the meters and paints the window
**
*/
void CMeterWindow::Redraw()
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

		if (cx != m_DIBSectionBufferW || cy != m_DIBSectionBufferH || m_DIBSectionBufferPixels == NULL)
		{
			delete m_DoubleBuffer;
			if (m_DIBSectionBuffer) DeleteObject(m_DIBSectionBuffer);
			m_DIBSectionBufferPixels = NULL;

			CreateDoubleBuffer(cx, cy);
		}
		else
		{
			memset(m_DIBSectionBufferPixels, 0, cx * cy * 4);
		}
	}

	if (m_WindowW != 0 && m_WindowH != 0)
	{
		Graphics graphics(m_DoubleBuffer);

		if (m_Background)
		{
			// Copy the background over the doublebuffer
			Rect r(0, 0, m_WindowW, m_WindowH);
			graphics.DrawImage(m_Background, r, 0, 0, m_Background->GetWidth(), m_Background->GetHeight(), UnitPixel);
		}
		else if (m_BackgroundMode == BGMODE_SOLID)
		{
			// Draw the solid color background
			Rect r(0, 0, m_WindowW, m_WindowH);

			if (m_SolidColor.GetA() != 0 || m_SolidColor2.GetA() != 0)
			{
				if (m_SolidColor.GetValue() == m_SolidColor2.GetValue())
				{
					graphics.Clear(m_SolidColor);
				}
				else
				{
					LinearGradientBrush gradient(r, m_SolidColor, m_SolidColor2, m_SolidAngle, TRUE);
					graphics.FillRectangle(&gradient, r);
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

				CMeter::DrawBevel(graphics, r, light, dark);
			}
		}

		// Draw the meters
		std::vector<CMeter*>::const_iterator j = m_Meters.begin();
		for ( ; j != m_Meters.end(); ++j)
		{
			const Matrix* matrix = (*j)->GetTransformationMatrix();
			if (matrix && !matrix->IsIdentity())
			{
				// Change the world matrix
				graphics.SetTransform(matrix);

				(*j)->Draw(graphics);

				// Set back to identity matrix
				graphics.ResetTransform();
			}
			else
			{
				(*j)->Draw(graphics);
			}
		}
	}

	UpdateWindow(m_TransparencyValue, false);
}

/*
** Updates the transition state
**
*/
void CMeterWindow::PostUpdate(bool bActiveTransition)
{
	// Start/stop the transition timer if necessary
	if (bActiveTransition && !m_ActiveTransition)
	{
		SetTimer(m_Window, TIMER_TRANSITION, m_TransitionUpdate, NULL);
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
bool CMeterWindow::UpdateMeasure(CMeasure* measure, bool force)
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

		if (measure->Update())
		{
			bUpdate = true;
		}
	}

	return bUpdate;
}

/*
** Updates the given meter
**
*/
bool CMeterWindow::UpdateMeter(CMeter* meter, bool& bActiveTransition, bool force)
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

		if (meter->Update())
		{
			bUpdate = true;
		}
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
void CMeterWindow::Update(bool refresh)
{
	++m_UpdateCounter;

	if (!m_Measures.empty())
	{
		// Pre-updates
		if (m_HasNetMeasures)
		{
			CMeasureNet::UpdateIFTable();
			CMeasureNet::UpdateStats();
		}

		// Update all measures
		std::vector<CMeasure*>::const_iterator i = m_Measures.begin();
		for ( ; i != m_Measures.end(); ++i)
		{
			UpdateMeasure((*i), refresh);
		}
	}

	CDialogAbout::UpdateMeasures(this);

	// Update all meters
	bool bActiveTransition = false;
	bool bUpdate = false;
	std::vector<CMeter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		if (UpdateMeter((*j), bActiveTransition, refresh))
		{
			bUpdate = true;
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
		if (Rainmeter->IsRedrawable())
		{
			Redraw();
		}
	}

	// Post-updates
	PostUpdate(bActiveTransition);
}

/*
** Updates the window contents
**
*/
void CMeterWindow::UpdateWindow(int alpha, bool reset)
{
	if (reset)
	{
		AddWindowExStyle(WS_EX_LAYERED);
	}

	BLENDFUNCTION blendPixelFunction = {AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA};
	POINT ptWindowScreenPosition = {m_ScreenX, m_ScreenY};
	POINT ptSrc = {0, 0};
	SIZE szWindow = {m_DIBSectionBufferW, m_DIBSectionBufferH};

	HDC dcScreen = GetDC(0);
	HDC dcMemory = CreateCompatibleDC(dcScreen);
	SelectObject(dcMemory, m_DIBSectionBuffer);

	BOOL ret = UpdateLayeredWindow(m_Window, dcScreen, &ptWindowScreenPosition, &szWindow, dcMemory, &ptSrc, 0, &blendPixelFunction, ULW_ALPHA);
	if (!ret)
	{
		// Retry after resetting WS_EX_LAYERED flag
		RemoveWindowExStyle(WS_EX_LAYERED);
		AddWindowExStyle(WS_EX_LAYERED);
		UpdateLayeredWindow(m_Window, dcScreen, &ptWindowScreenPosition, &szWindow, dcMemory, &ptSrc, 0, &blendPixelFunction, ULW_ALPHA);
	}

	ReleaseDC(0, dcScreen);
	DeleteDC(dcMemory);

	m_TransparencyValue = alpha;
}

/*
** Handles the timers. The METERTIMER updates all the measures
** MOUSETIMER is used to hide/show the window.
**
*/
LRESULT CMeterWindow::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (wParam == TIMER_METER)
	{
		Update(false);
	}
	else if (wParam == TIMER_MOUSE)
	{
		if (!Rainmeter->IsMenuActive() && !m_Dragging)
		{
			ShowWindowIfAppropriate();

			if (m_WindowZPosition == ZPOSITION_ONTOPMOST)
			{
				ChangeZPos(ZPOSITION_ONTOPMOST);
			}

			if (m_MouseOver)
			{
				POINT pos = CSystem::GetCursorPosition();

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
	}
	else if (wParam == TIMER_TRANSITION)
	{
		// Redraw only if there is active transition still going
		bool bActiveTransition = false;
		std::vector<CMeter*>::const_iterator j = m_Meters.begin();
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
	else if (wParam == TIMER_FADE)
	{
		ULONGLONG ticks = CSystem::GetTickCount64();
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
				UpdateWindow(m_FadeEndValue, false);
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

			UpdateWindow((int)value, false);
		}
	}
	else if (wParam == TIMER_DEACTIVATE)
	{
		if (m_FadeStartTime == 0)
		{
			KillTimer(m_Window, TIMER_DEACTIVATE);
			Rainmeter->DeleteMeterWindow(this, true);  // "delete this;"
		}
	}

	return 0;
}

void CMeterWindow::FadeWindow(int from, int to)
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
				UpdateWindow(to, false);
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
		UpdateWindow(from, false);
		if (from == 0)
		{
			if (!m_Hidden)
			{
				ShowWindow(m_Window, SW_SHOWNOACTIVATE);
			}
		}

		SetTimer(m_Window, TIMER_FADE, INTERVAL_FADE, NULL);
	}
}

void CMeterWindow::HideFade()
{
	m_Hidden = true;
	if (IsWindowVisible(m_Window))
	{
		FadeWindow(m_AlphaValue, 0);
	}
}

void CMeterWindow::ShowFade()
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
void CMeterWindow::ShowWindowIfAppropriate()
{
	bool keyDown = IsCtrlKeyDown() || IsShiftKeyDown() || IsAltKeyDown();

	POINT pos = CSystem::GetCursorPosition();
	POINT posScr = pos;

	MapWindowPoints(NULL, m_Window, &pos, 1);
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
HWND CMeterWindow::GetWindowFromPoint(POINT pos)
{
	HWND hwndPos = WindowFromPoint(pos);

	if (hwndPos == m_Window || (!m_ClickThrough && m_WindowHide != HIDEMODE_HIDE))
	{
		return hwndPos;
	}

	MapWindowPoints(NULL, m_Window, &pos, 1);

	if (HitTest(pos.x, pos.y))
	{
		if (hwndPos)
		{
			HWND hWnd = GetAncestor(hwndPos, GA_ROOT);
			while (hWnd = FindWindowEx(NULL, hWnd, METERWINDOW_CLASS_NAME, NULL))
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
bool CMeterWindow::HitTest(int x, int y)
{
	if (x >= 0 && y >= 0 && x < m_WindowW && y < m_WindowH)
	{
		// Check transparent pixels
		if (m_DIBSectionBufferPixels)
		{
			DWORD pixel = m_DIBSectionBufferPixels[y * m_WindowW + x];  // top-down DIB
			return ((pixel & 0xFF000000) != 0);
		}
		else
		{
			return true;
		}
	}

	return false;
}

/*
** Handles all buttons and cursor.
**
*/
void CMeterWindow::HandleButtons(POINT pos, BUTTONPROC proc, bool execute)
{
	bool redraw = false;
	HCURSOR cursor = NULL;

	std::vector<CMeter*>::const_reverse_iterator j = m_Meters.rbegin();
	for ( ; j != m_Meters.rend(); ++j)
	{
		// Hidden meters are ignored
		if ((*j)->IsHidden()) continue;

		CMeterButton* button = NULL;
		if (m_HasButtons && (*j)->GetTypeID() == TypeID<CMeterButton>())
		{
			button = (CMeterButton*)(*j);
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

		if (!cursor &&
			((*j)->HasMouseAction() || button) &&
			(*j)->GetMouse().GetCursorState() &&
			(*j)->HitTest(pos.x, pos.y))
		{
			cursor = (*j)->GetMouse().GetCursor();
		}
	}

	if (redraw)
	{
		Redraw();
	}

	if (!cursor)
	{
		cursor = LoadCursor(NULL, IDC_ARROW);
	}

	SetCursor(cursor);
}

/*
** During setting the cursor do nothing.
**
*/
LRESULT CMeterWindow::OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

/*
** Enters context menu loop.
**
*/
LRESULT CMeterWindow::OnEnterMenuLoop(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Set cursor to default
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	return 0;
}

/*
** When we get WM_MOUSEMOVE messages, hide the window as the mouse is over it.
**
*/
LRESULT CMeterWindow::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			MapWindowPoints(NULL, m_Window, &pos, 1);
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
LRESULT CMeterWindow::OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos = CSystem::GetCursorPosition();
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
LRESULT CMeterWindow::OnMouseScrollMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
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

	MapWindowPoints(NULL, m_Window, &pos, 1);

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
LRESULT CMeterWindow::OnMouseHScrollMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	MapWindowPoints(NULL, m_Window, &pos, 1);

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
LRESULT CMeterWindow::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (wParam == IDM_SKIN_EDITSKIN)
	{
		Rainmeter->EditSkinFile(m_FolderPath, m_FileName);
	}
	else if (wParam == IDM_SKIN_REFRESH)
	{
		Refresh(false);
	}
	else if (wParam == IDM_SKIN_OPENSKINSFOLDER)
	{
		Rainmeter->OpenSkinFolder(m_FolderPath);
	}
	else if (wParam == IDM_SKIN_MANAGESKIN)
	{
		CDialogManage::OpenSkin(this);
	}
	else if (wParam == IDM_SKIN_VERYTOPMOST)
	{
		SetWindowZPosition(ZPOSITION_ONTOPMOST);
	}
	else if (wParam == IDM_SKIN_TOPMOST)
	{
		SetWindowZPosition(ZPOSITION_ONTOP);
	}
	else if (wParam == IDM_SKIN_BOTTOM)
	{
		SetWindowZPosition(ZPOSITION_ONBOTTOM);
	}
	else if (wParam == IDM_SKIN_NORMAL)
	{
		SetWindowZPosition(ZPOSITION_NORMAL);
	}
	else if (wParam == IDM_SKIN_ONDESKTOP)
	{
		SetWindowZPosition(ZPOSITION_ONDESKTOP);
	}
	else if (wParam == IDM_SKIN_KEEPONSCREEN)
	{
		SetKeepOnScreen(!m_KeepOnScreen);
	}
	else if (wParam == IDM_SKIN_CLICKTHROUGH)
	{
		SetClickThrough(!m_ClickThrough);
	}
	else if (wParam == IDM_SKIN_DRAGGABLE)
	{
		SetWindowDraggable(!m_WindowDraggable);
	}
	else if (wParam == IDM_SKIN_HIDEONMOUSE)
	{
		SetWindowHide((m_WindowHide == HIDEMODE_NONE) ? HIDEMODE_HIDE : HIDEMODE_NONE);
	}
	else if (wParam == IDM_SKIN_TRANSPARENCY_FADEIN)
	{
		SetWindowHide((m_WindowHide == HIDEMODE_NONE) ? HIDEMODE_FADEIN : HIDEMODE_NONE);
	}
	else if (wParam == IDM_SKIN_TRANSPARENCY_FADEOUT)
	{
		SetWindowHide((m_WindowHide == HIDEMODE_NONE) ? HIDEMODE_FADEOUT : HIDEMODE_NONE);
	}
	else if (wParam == IDM_SKIN_REMEMBERPOSITION)
	{
		SetSavePosition(!m_SavePosition);
	}
	else if (wParam == IDM_SKIN_SNAPTOEDGES)
	{
		SetSnapEdges(!m_SnapEdges);
	}
	else if (wParam >= IDM_SKIN_TRANSPARENCY_0 && wParam <= IDM_SKIN_TRANSPARENCY_90)
	{
		m_AlphaValue = (int)(255.0 - (wParam - IDM_SKIN_TRANSPARENCY_0) * (230.0 / (IDM_SKIN_TRANSPARENCY_90 - IDM_SKIN_TRANSPARENCY_0)));
		UpdateWindow(m_AlphaValue, false);
		WriteOptions(OPTION_ALPHAVALUE);
	}
	else if (wParam == IDM_CLOSESKIN)
	{
		Rainmeter->DeactivateSkin(this, -1);
	}
	else if (wParam == IDM_SKIN_FROMRIGHT)
	{
		m_WindowXFromRight = !m_WindowXFromRight;

		ScreenToWindow();
		WriteOptions(OPTION_POSITION);
	}
	else if (wParam == IDM_SKIN_FROMBOTTOM)
	{
		m_WindowYFromBottom = !m_WindowYFromBottom;

		ScreenToWindow();
		WriteOptions(OPTION_POSITION);
	}
	else if (wParam == IDM_SKIN_XPERCENTAGE)
	{
		m_WindowXPercentage = !m_WindowXPercentage;

		ScreenToWindow();
		WriteOptions(OPTION_POSITION);
	}
	else if (wParam == IDM_SKIN_YPERCENTAGE)
	{
		m_WindowYPercentage = !m_WindowYPercentage;

		ScreenToWindow();
		WriteOptions(OPTION_POSITION);
	}
	else if (wParam == IDM_SKIN_MONITOR_AUTOSELECT)
	{
		m_AutoSelectScreen = !m_AutoSelectScreen;

		ScreenToWindow();
		WriteOptions(OPTION_POSITION | OPTION_AUTOSELECTSCREEN);
	}
	else if (wParam == IDM_SKIN_MONITOR_PRIMARY || wParam >= ID_MONITOR_FIRST && wParam <= ID_MONITOR_LAST)
	{
		const MultiMonitorInfo& multimonInfo = CSystem::GetMultiMonitorInfo();
		const std::vector<MonitorInfo>& monitors = multimonInfo.monitors;

		int screenIndex;
		bool screenDefined;
		if (wParam == IDM_SKIN_MONITOR_PRIMARY)
		{
			screenIndex = multimonInfo.primary;
			screenDefined = false;
		}
		else
		{
			screenIndex = (wParam & 0x0ffff) - ID_MONITOR_FIRST;
			screenDefined = true;
		}

		if (screenIndex >= 0 && (screenIndex == 0 || screenIndex <= (int)monitors.size() && monitors[screenIndex-1].active))
		{
			if (m_AutoSelectScreen)
			{
				m_AutoSelectScreen = false;
			}

			m_WindowXScreen = m_WindowYScreen = screenIndex;
			m_WindowXScreenDefined = m_WindowYScreenDefined = screenDefined;

			m_Parser.ResetMonitorVariables(this);  // Set present monitor variables
			ScreenToWindow();
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
			Rainmeter->ExecuteCommand(action.c_str(), this);
		}
	}
	else
	{
		// Forward to tray window, which handles all the other commands
		HWND tray = Rainmeter->GetTrayWindow()->GetWindow();

		if (wParam == IDM_QUIT)
		{
			PostMessage(tray, WM_COMMAND, wParam, lParam);
		}
		else
		{
			SendMessage(tray, WM_COMMAND, wParam, lParam);
		}
	}

	return 0;
}

/*
** Helper function for setting ClickThrough
**
*/
void CMeterWindow::SetClickThrough(bool b)
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
void CMeterWindow::SetKeepOnScreen(bool b)
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
void CMeterWindow::SetWindowDraggable(bool b)
{
	m_WindowDraggable = b;
	WriteOptions(OPTION_DRAGGABLE);
}

/*
** Helper function for setting SavePosition
**
*/
void CMeterWindow::SetSavePosition(bool b)
{
	m_SavePosition = b;
	WriteOptions(OPTION_POSITION | OPTION_SAVEPOSITION);
}

/*
** Helper function for setting SnapEdges
**
*/
void CMeterWindow::SetSnapEdges(bool b)
{
	m_SnapEdges = b;
	WriteOptions(OPTION_SNAPEDGES);
}

/*
** Helper function for setting WindowHide
**
*/
void CMeterWindow::SetWindowHide(HIDEMODE hide)
{
	m_WindowHide = hide;
	UpdateWindow(m_AlphaValue, false);
	WriteOptions(OPTION_HIDEONMOUSEOVER);
}

/*
** Helper function for setting Position
**
*/
void CMeterWindow::SetWindowZPosition(ZPOSITION zpos)
{
	ChangeSingleZPos(zpos);
	WriteOptions(OPTION_ALWAYSONTOP);
}

/*
** Handle dragging the window
**
*/
LRESULT CMeterWindow::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
		ScreenToWindow();

		if (m_SavePosition)
		{
			WriteOptions(OPTION_POSITION);
		}

		POINT pos = CSystem::GetCursorPosition();
		MapWindowPoints(NULL, m_Window, &pos, 1);

		// Handle buttons
		HandleButtons(pos, BUTTONPROC_UP, false);  // redraw only

		// Workaround for the system that the window size is changed incorrectly when the window is dragged over the upper side of the virtual screen
		UpdateWindow(m_TransparencyValue, false);
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
LRESULT CMeterWindow::OnEnterSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_Dragging)
	{
		m_Dragged = true;  // Don't post the WM_NCLBUTTONUP message!

		// Set cursor to default
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	}

	return 0;
}

/*
** Ends dragging
**
*/
LRESULT CMeterWindow::OnExitSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

/*
** This is overwritten so that the window can be dragged
**
*/
LRESULT CMeterWindow::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_WindowDraggable && !Rainmeter->GetDisableDragging())
	{
		POINT pos;
		pos.x = GET_X_LPARAM(lParam);
		pos.y = GET_Y_LPARAM(lParam);
		MapWindowPoints(NULL, m_Window, &pos, 1);

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
LRESULT CMeterWindow::OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPWINDOWPOS wp = (LPWINDOWPOS)lParam;

	if (!m_Refreshing)
	{
		if (m_WindowZPosition == ZPOSITION_NORMAL && Rainmeter->IsNormalStayDesktop() && CSystem::GetShowDesktop())
		{
			if (!(wp->flags & (SWP_NOOWNERZORDER | SWP_NOACTIVATE)))
			{
				// Set window on top of all other ZPOSITION_ONDESKTOP, ZPOSITION_BOTTOM, and ZPOSITION_NORMAL windows
				wp->hwndInsertAfter = CSystem::GetBackmostTopWindow();
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
				RECT workArea;

				//HMONITOR hMonitor = MonitorFromWindow(m_Window, MONITOR_DEFAULTTONULL);  // returns incorrect monitor when the window is "On Desktop"
				RECT windowRect = {wp->x, wp->y, (wp->x + m_WindowW), (wp->y + m_WindowH)};
				HMONITOR hMonitor = MonitorFromRect(&windowRect, MONITOR_DEFAULTTONULL);

				if (hMonitor != NULL)
				{
					MONITORINFO mi;
					mi.cbSize = sizeof(mi);
					GetMonitorInfo(hMonitor, &mi);
					workArea = mi.rcWork;
				}
				else
				{
					GetClientRect(GetDesktopWindow(), &workArea);
				}

				// Snap to other windows
				std::map<std::wstring, CMeterWindow*>::const_iterator iter = Rainmeter->GetAllMeterWindows().begin();
				for ( ; iter != Rainmeter->GetAllMeterWindows().end(); ++iter)
				{
					if ((*iter).second != this)
					{
						SnapToWindow((*iter).second, wp);
					}
				}

				int w = workArea.right - m_WindowW;
				int h = workArea.bottom - m_WindowH;

				if ((wp->x < SNAPDISTANCE + workArea.left) && (wp->x > workArea.left - SNAPDISTANCE)) wp->x = workArea.left;
				if ((wp->y < SNAPDISTANCE + workArea.top) && (wp->y > workArea.top - SNAPDISTANCE)) wp->y = workArea.top;
				if ((wp->x < SNAPDISTANCE + w) && (wp->x > -SNAPDISTANCE + w)) wp->x = w;
				if ((wp->y < SNAPDISTANCE + h) && (wp->y > -SNAPDISTANCE + h)) wp->y = h;
			}
		}

		if (m_KeepOnScreen)
		{
			MapCoordsToScreen(wp->x, wp->y, m_WindowW, m_WindowH);
		}
	}

	return DefWindowProc(m_Window, uMsg, wParam, lParam);
}

void CMeterWindow::SnapToWindow(CMeterWindow* window, LPWINDOWPOS wp)
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
LRESULT CMeterWindow::OnDwmColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
LRESULT CMeterWindow::OnDwmCompositionChange(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
void CMeterWindow::BlurBehindWindow(BOOL fEnable)
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
LRESULT CMeterWindow::OnDisplayChange(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

/*
** During setting changes do nothing.
** (OnDelayedMove function is used instead.)
**
*/
LRESULT CMeterWindow::OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

/*
** Runs the action when left mouse button is down
**
*/
LRESULT CMeterWindow::OnLeftButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCLBUTTONDOWN)
	{
		// Transform the point to client rect
		MapWindowPoints(NULL, m_Window, &pos, 1);
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
LRESULT CMeterWindow::OnLeftButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCLBUTTONUP)
	{
		// Transform the point to client rect
		MapWindowPoints(NULL, m_Window, &pos, 1);
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
LRESULT CMeterWindow::OnLeftButtonDoubleClick(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCLBUTTONDBLCLK)
	{
		// Transform the point to client rect
		MapWindowPoints(NULL, m_Window, &pos, 1);
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
LRESULT CMeterWindow::OnRightButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCRBUTTONDOWN)
	{
		// Transform the point to client rect
		MapWindowPoints(NULL, m_Window, &pos, 1);
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
LRESULT CMeterWindow::OnRightButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
LRESULT CMeterWindow::OnRightButtonDoubleClick(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCRBUTTONDBLCLK)
	{
		// Transform the point to client rect
		MapWindowPoints(NULL, m_Window, &pos, 1);
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
LRESULT CMeterWindow::OnMiddleButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCMBUTTONDOWN)
	{
		// Transform the point to client rect
		MapWindowPoints(NULL, m_Window, &pos, 1);
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
LRESULT CMeterWindow::OnMiddleButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCMBUTTONUP)
	{
		// Transform the point to client rect
		MapWindowPoints(NULL, m_Window, &pos, 1);
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
LRESULT CMeterWindow::OnMiddleButtonDoubleClick(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCMBUTTONDBLCLK)
	{
		// Transform the point to client rect
		MapWindowPoints(NULL, m_Window, &pos, 1);
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
LRESULT CMeterWindow::OnXButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCXBUTTONDOWN)
	{
		// Transform the point to client rect
		MapWindowPoints(NULL, m_Window, &pos, 1);
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
LRESULT CMeterWindow::OnXButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCXBUTTONUP)
	{
		// Transform the point to client rect
		MapWindowPoints(NULL, m_Window, &pos, 1);
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
LRESULT CMeterWindow::OnXButtonDoubleClick(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam);

	if (uMsg == WM_NCXBUTTONDBLCLK)
	{
		// Transform the point to client rect
		MapWindowPoints(NULL, m_Window, &pos, 1);
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
LRESULT CMeterWindow::OnSetWindowFocus(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SETFOCUS:
		if (!m_OnFocusAction.empty())
		{
			Rainmeter->ExecuteCommand(m_OnFocusAction.c_str(), this);
		}
		break;

	case WM_KILLFOCUS:
		if (!m_OnUnfocusAction.empty())
		{
			Rainmeter->ExecuteCommand(m_OnUnfocusAction.c_str(), this);
		}
		break;
	}

	return 0;
}

/*
** Handles the context menu. The menu is recreated every time it is shown.
**
*/
LRESULT CMeterWindow::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam)
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

	Rainmeter->ShowContextMenu(pos, this);

	return 0;
}

/*
** Executes the action if such are defined. Returns true, if action was executed.
** If the test is true, the action is not executed.
**
*/
bool CMeterWindow::DoAction(int x, int y, MOUSEACTION action, bool test)
{
	const WCHAR* command = NULL;

	// Check if the hitpoint was over some meter
	std::vector<CMeter*>::const_reverse_iterator j = m_Meters.rbegin();
	for ( ; j != m_Meters.rend(); ++j)
	{
		// Hidden meters are ignored
		if ((*j)->IsHidden()) continue;

		const WCHAR* meterCommand = (*j)->GetMouse().GetActionCommand(action);
		if (meterCommand && (*j)->HitTest(x, y))
		{
			command = meterCommand;
			break;
		}
	}

	if (!command && HitTest(x, y))
	{
		command = m_Mouse.GetActionCommand(action);
	}

	if (command)
	{
		if (!test)
		{
			Rainmeter->ExecuteCommand(command, this);
		}

		return true;
	}

	return false;
}

/*
** Executes the action if such are defined. Returns true, if meter/window which should be processed still may exist.
**
*/
bool CMeterWindow::DoMoveAction(int x, int y, MOUSEACTION action)
{
	bool buttonFound = false;

	// Check if the hitpoint was over some meter
	std::vector<CMeter*>::const_reverse_iterator j = m_Meters.rbegin();
	for ( ; j != m_Meters.rend(); ++j)
	{
		if (!(*j)->IsHidden() && (*j)->HitTest(x, y))
		{
			if (action == MOUSE_OVER)
			{
				if (!m_MouseOver)
				{
					// If the mouse is over a meter it's also over the main window
					//LogWithArgs(LOG_DEBUG, L"@Enter: %s", m_FolderPath.c_str());
					m_MouseOver = true;
					SetMouseLeaveEvent(false);
					RegisterMouseInput();

					if (!m_Mouse.GetOverAction().empty())
					{
						UINT currCounter = m_MouseMoveCounter;
						Rainmeter->ExecuteCommand(m_Mouse.GetOverAction().c_str(), this);
						return (currCounter == m_MouseMoveCounter);
					}
				}

				// Handle button
				CMeterButton* button = NULL;
				if (m_HasButtons && (*j)->GetTypeID() == TypeID<CMeterButton>())
				{
					button = (CMeterButton*)(*j);
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
					if (!((*j)->GetMouse().GetOverAction().empty()) ||
						!((*j)->GetMouse().GetLeaveAction().empty()) ||
						button)
					{
						//LogWithArgs(LOG_DEBUG, L"MeterEnter: %s - [%s]", m_FolderPath.c_str(), (*j)->GetName());
						(*j)->SetMouseOver(true);

						if (!((*j)->GetMouse().GetOverAction().empty()))
						{
							UINT currCounter = m_MouseMoveCounter;
							Rainmeter->ExecuteCommand((*j)->GetMouse().GetOverAction().c_str(), this);
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
					if (m_HasButtons && (*j)->GetTypeID() == TypeID<CMeterButton>())
					{
						CMeterButton* button = (CMeterButton*)(*j);
						button->SetFocus(false);
					}

					//LogWithArgs(LOG_DEBUG, L"MeterLeave: %s - [%s]", m_FolderPath.c_str(), (*j)->GetName());
					(*j)->SetMouseOver(false);

					if (!((*j)->GetMouse().GetLeaveAction().empty()))
					{
						Rainmeter->ExecuteCommand((*j)->GetMouse().GetLeaveAction().c_str(), this);
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
				//LogWithArgs(LOG_DEBUG, L"Enter: %s", m_FolderPath.c_str());
				m_MouseOver = true;
				SetMouseLeaveEvent(false);
				RegisterMouseInput();

				if (!m_Mouse.GetOverAction().empty())
				{
					UINT currCounter = m_MouseMoveCounter;
					Rainmeter->ExecuteCommand(m_Mouse.GetOverAction().c_str(), this);
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
				//LogWithArgs(LOG_DEBUG, L"Leave: %s", m_FolderPath.c_str());
				m_MouseOver = false;
				SetMouseLeaveEvent(true);
				UnregisterMouseInput();

				if (!m_Mouse.GetLeaveAction().empty())
				{
					Rainmeter->ExecuteCommand(m_Mouse.GetLeaveAction().c_str(), this);
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
LRESULT CMeterWindow::OnMouseInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pos = CSystem::GetCursorPosition();
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
LRESULT CMeterWindow::OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
** The main window procedure for the meter window.
**
*/
LRESULT CALLBACK CMeterWindow::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CMeterWindow* window = (CMeterWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

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
	END_MESSAGEPROC
}

/*
** The initial window procedure for the meter window. Passes control to WndProc after initial setup.
**
*/
LRESULT CALLBACK CMeterWindow::InitialWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_NCCREATE)
	{
		CMeterWindow* window = (CMeterWindow*)((LPCREATESTRUCT)lParam)->lpCreateParams;
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
LRESULT CMeterWindow::OnDelayedRefresh(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Refresh(false);
	return 0;
}

/*
** Handles delayed move
**
*/
LRESULT CMeterWindow::OnDelayedMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	m_Parser.ResetMonitorVariables(this);

	// Move the window to correct position
	ResizeWindow(true);

	if (m_KeepOnScreen)
	{
		MapCoordsToScreen(m_ScreenX, m_ScreenY, m_WindowW, m_WindowH);
	}

	SetWindowPos(m_Window, NULL, m_ScreenX, m_ScreenY, m_WindowW, m_WindowH, SWP_NOZORDER | SWP_NOACTIVATE);

	return 0;
}

/*
** Handles bangs from the exe
**
*/
LRESULT CMeterWindow::OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	COPYDATASTRUCT* pCopyDataStruct = (COPYDATASTRUCT*)lParam;

	if (pCopyDataStruct && (pCopyDataStruct->dwData == 1) && (pCopyDataStruct->cbData > 0))
	{
		// Check that we're still alive
		bool found = false;
		std::map<std::wstring, CMeterWindow*>::const_iterator iter = Rainmeter->GetAllMeterWindows().begin();
		for ( ; iter != Rainmeter->GetAllMeterWindows().end(); ++iter)
		{
			if ((*iter).second == this)
			{
				found = true;
				break;
			}
		}

		if (found)
		{
			const WCHAR* command = (const WCHAR*)pCopyDataStruct->lpData;
			Rainmeter->ExecuteCommand(command, this);
		}
		else
		{
			// This meterwindow has been deactivated
			Log(LOG_WARNING, L"Unable to bang unloaded skin");
		}

		return TRUE;
	}

	return FALSE;
}

/*
** Sets up the window position variables.
**
*/
void CMeterWindow::SetWindowPositionVariables(int x, int y)
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
void CMeterWindow::SetWindowSizeVariables(int w, int h)
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
void CMeterWindow::MakePathAbsolute(std::wstring& path)
{
	if (path.empty() || CSystem::IsAbsolutePath(path))
	{
		return;  // It's already absolute path (or it's empty)
	}
	else
	{
		std::wstring absolute;
		absolute.reserve(Rainmeter->GetSkinPath().size() + m_FolderPath.size() + 1 + path.size());
		absolute = Rainmeter->GetSkinPath();
		absolute += m_FolderPath;
		absolute += L'\\';
		absolute += path;
		absolute.swap(path);
	}
}

std::wstring CMeterWindow::GetFilePath()
{
	std::wstring file = Rainmeter->GetSkinPath() + m_FolderPath;
	file += L'\\';
	file += m_FileName;
	return file;
}

std::wstring CMeterWindow::GetRootPath()
{
	std::wstring path = Rainmeter->GetSkinPath();

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

std::wstring CMeterWindow::GetResourcesPath()
{
	std::wstring path = GetRootPath();
	path += L"@Resources\\";
	return path;
}

CMeter* CMeterWindow::GetMeter(const std::wstring& meterName)
{
	const WCHAR* name = meterName.c_str();
	std::vector<CMeter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		if (_wcsicmp((*j)->GetName(), name) == 0)
		{
			return (*j);
		}
	}
	return NULL;
}
