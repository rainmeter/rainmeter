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
CMeterWindow::CMeterWindow(const std::wstring& config, const std::wstring& iniFile) : m_SkinName(config), m_SkinIniFile(iniFile),
	m_DoubleBuffer(),
	m_DIBSectionBuffer(),
	m_DIBSectionBufferPixels(),
	m_DIBSectionBufferW(),
	m_DIBSectionBufferH(),
	m_Background(),
	m_BackgroundSize(),
	m_Window(),
	m_MouseOver(false),
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
	m_NativeTransparency(true),
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
	m_ResetRegion(false),
	m_UpdateCounter(),
	m_MouseMoveCounter(),
	m_Rainmeter(),
	m_FontCollection(),
	m_MouseActionCursor(true),
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

	++c_InstanceCount;
}

/*
** Destructor
**
*/
CMeterWindow::~CMeterWindow()
{
	WriteConfig();

	// Kill the timer
	KillTimer(m_Window, TIMER_METER);
	KillTimer(m_Window, TIMER_MOUSE);
	KillTimer(m_Window, TIMER_FADE);
	KillTimer(m_Window, TIMER_TRANSITION);

	// Destroy the meters
	std::list<CMeter*>::iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		delete (*j);
	}

	// Destroy the measures
	std::list<CMeasure*>::iterator i = m_Measures.begin();
	for ( ; i != m_Measures.end(); ++i)
	{
		delete (*i);
	}

	delete m_Background;
	delete m_DoubleBuffer;
	if (m_DIBSectionBuffer) DeleteObject(m_DIBSectionBuffer);

	if (m_BlurRegion) DeleteObject(m_BlurRegion);

	if (m_Window) DestroyWindow(m_Window);

	if (m_FontCollection)
	{
		CMeterString::FreeFontCache(m_FontCollection);
		delete m_FontCollection;
	}

	--c_InstanceCount;

	if (c_InstanceCount == 0)
	{
		BOOL Result;
		int counter = 0;
		do
		{
			// Wait for the window to die
			Result = UnregisterClass(METERWINDOW_CLASS_NAME, m_Rainmeter->GetInstance());
			Sleep(100);
			++counter;
		}
		while(!Result && counter < 10);

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
** Initializes the window, creates the class and the window.
**
*/
int CMeterWindow::Initialize(CRainmeter& Rainmeter)
{
	m_Rainmeter = &Rainmeter;

	// Register the windowclass
	WNDCLASSEX wc = {sizeof(WNDCLASSEX)};
	wc.style = CS_NOCLOSE | CS_DBLCLKS;
	wc.lpfnWndProc = InitialWndProc;
	wc.hInstance = m_Rainmeter->GetInstance();
	wc.hCursor = NULL;  // The cursor should be controlled by using SetCursor() when needed.
	wc.lpszClassName = METERWINDOW_CLASS_NAME;

	if (!RegisterClassEx(&wc))
	{
		DWORD err = GetLastError();

		if (err != 0 && ERROR_CLASS_ALREADY_EXISTS != err)
		{
			throw CError(L"Unable to register class");
		}
	}

	m_Window = CreateWindowEx(WS_EX_TOOLWINDOW,
							METERWINDOW_CLASS_NAME,
							NULL,
							WS_POPUP,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							CW_USEDEFAULT,
							NULL,
							NULL,
							m_Rainmeter->GetInstance(),
							this);

	if (m_Window == NULL)
	{
		throw CError(L"Unable to register window");
	}

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

	Log(LOG_NOTICE, L"Initialization successful");

	return 0;
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
** Unloads the skin with delay to avoid crash (and for fade to complete).
**
*/
void CMeterWindow::Deactivate()
{
	HideFade();
	SetTimer(m_Window, TIMER_DEACTIVATE, m_FadeDuration + 50, NULL);
}

/*
** This deletes everything and rebuilds the config again.
**
*/
void CMeterWindow::Refresh(bool init, bool all)
{
	assert(m_Rainmeter != NULL);

	m_Rainmeter->SetCurrentParser(&m_Parser);

	std::wstring notice = L"Refreshing skin \"" + m_SkinName;
	notice += L'\\';
	notice += m_SkinIniFile;
	notice += L'"';
	Log(LOG_NOTICE, notice.c_str());

	m_Refreshing = true;

	if (!init)
	{
		// First destroy everything
		KillTimer(m_Window, TIMER_METER);
		KillTimer(m_Window, TIMER_MOUSE);
		KillTimer(m_Window, TIMER_FADE);
		KillTimer(m_Window, TIMER_TRANSITION);

		m_ActiveTransition = false;

		m_MouseOver = false;
		SetMouseLeaveEvent(true);

		std::list<CMeasure*>::iterator i = m_Measures.begin();
		for ( ; i != m_Measures.end(); ++i)
		{
			delete (*i);
		}
		m_Measures.clear();

		std::list<CMeter*>::iterator j = m_Meters.begin();
		for ( ; j != m_Meters.end(); ++j)
		{
			delete (*j);
		}
		m_Meters.clear();

		delete m_Background;
		m_Background = NULL;

		m_BackgroundSize.cx = m_BackgroundSize.cy = 0;

		m_BackgroundName.erase();

		if (m_BlurRegion) DeleteObject(m_BlurRegion);
		m_BlurRegion = NULL;

		if (m_FontCollection)
		{
			CMeterString::FreeFontCache(m_FontCollection);
			delete m_FontCollection;
			m_FontCollection = NULL;
		}
	}

	ZPOSITION oldZPos = m_WindowZPosition;

	//TODO: Should these be moved to a Reload command instead of hitting the disk on every refresh
	ReadConfig();	// Read the general settings
	if (!ReadSkin())
	{
		m_Rainmeter->DeactivateConfig(this, -1);
		return;
	}

	InitializeMeasures();
	InitializeMeters();

	// Remove transparent flag
	LONG style = GetWindowLong(m_Window, GWL_EXSTYLE);
	if ((style & WS_EX_TRANSPARENT) != 0)
	{
		SetWindowLong(m_Window, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
	}

	m_Hidden = m_WindowStartHidden;

	// Set the window region
	CreateRegion(true);	// Clear the region
	UpdateTransparency(m_AlphaValue, true);  // Add/Remove layered flag
	Update(false);

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
		if (0 == SetTimer(m_Window, TIMER_METER, m_WindowUpdate, NULL))
		{
			throw CError(L"Unable to set timer");
		}
	}

	if (0 == SetTimer(m_Window, TIMER_MOUSE, INTERVAL_MOUSE, NULL))	// Mouse position is checked twice per sec
	{
		throw CError(L"Unable to set timer");
	}

	m_Rainmeter->SetCurrentParser(NULL);

	m_Refreshing = false;

	if (!m_OnRefreshAction.empty())
	{
		m_Rainmeter->ExecuteCommand(m_OnRefreshAction.c_str(), this);
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
		WriteConfig(SETTING_WINDOWPOSITION);
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
					if (GetWindowLong(winPos, GWL_EXSTYLE) & WS_EX_TOPMOST)
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
	if (zPos == ZPOSITION_NORMAL && m_Rainmeter->IsNormalStayDesktop() && (!all || CSystem::GetShowDesktop()))
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
		CDialogAbout::UpdateMeasures(this);
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
		UpdateTransparency((m_WindowHide == HIDEMODE_FADEOUT) ? 255 : m_AlphaValue, false);
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
			double value;
			int x = m_Parser.ParseFormula(args[0], &value) ? (int)value : _wtoi(args[0].c_str());
			int y = m_Parser.ParseFormula(args[1], &value) ? (int)value : _wtoi(args[1].c_str());

			MoveWindow(x, y);
		}
		break;

	case BANG_ZPOS:
		SetWindowZPosition((ZPOSITION)_wtoi(args[0].c_str()));
		break;

	case BANG_CLICKTHROUGH:
		{
			int f = _wtoi(args[0].c_str());
			SetClickThrough((f == -1) ? !m_ClickThrough : f);
		}
		break;

	case BANG_DRAGGABLE:
		{
			int f = _wtoi(args[0].c_str());
			SetWindowDraggable((f == -1) ? !m_WindowDraggable : f);
		}
		break;

	case BANG_SNAPEDGES:
		{
			int f = _wtoi(args[0].c_str());
			SetSnapEdges((f == -1) ? !m_SnapEdges : f);
		}
		break;

	case BANG_KEEPONSCREEN:
		{
			int f = _wtoi(args[0].c_str());
			SetKeepOnScreen((f == -1) ? !m_KeepOnScreen : f);
		}
		break;

	case BANG_SETTRANSPARENCY:
		{
			const std::wstring& arg = args[0];
			m_AlphaValue = CConfigParser::ParseInt(arg.c_str(), 255);
			m_AlphaValue = max(m_AlphaValue, 0);
			m_AlphaValue = min(m_AlphaValue, 255);
			UpdateTransparency(m_AlphaValue, false);
		}
		break;

	case BANG_MOVEMETER:
		{
			double value;
			int x = m_Parser.ParseFormula(args[0], &value) ? (int)value : _wtoi(args[0].c_str());
			int y = m_Parser.ParseFormula(args[1], &value) ? (int)value : _wtoi(args[1].c_str());

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
** This is a helper template that compares the given name to measure/meter's name.
**
*/
template <class T>
bool CompareName(T* m, const WCHAR* name, bool group)
{
	return (group) ? m->BelongsToGroup(name) : (_wcsicmp(m->GetName(), name) == 0);
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
		double val;
		int type, x, y, w = 0, h = 0;

		WCHAR* token = wcstok(parseSz, L",");
		if (token)
		{
			while (token[0] == L' ') ++token;
			type = (m_Parser.ParseFormula(token, &val)) ? (int)val : _wtoi(token);

			token = wcstok(NULL, L",");
			if (token)
			{
				while (token[0] == L' ') ++token;
				x = (m_Parser.ParseFormula(token, &val)) ? (int)val : _wtoi(token);

				token = wcstok(NULL, L",");
				if (token)
				{
					while (token[0] == L' ') ++token;
					y = (m_Parser.ParseFormula(token, &val)) ? (int)val : _wtoi(token);

					token = wcstok(NULL, L",");
					if (token)
					{
						while (token[0] == L' ') ++token;
						w = (m_Parser.ParseFormula(token, &val)) ? (int)val : _wtoi(token);

						token = wcstok(NULL, L",");
						if (token)
						{
							while (token[0] == L' ') ++token;
							h = (m_Parser.ParseFormula(token, &val)) ? (int)val : _wtoi(token);
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
					int r = (m_Parser.ParseFormula(token, &val)) ? (int)val : _wtoi(token);
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
** Shows the given meter
**
*/
void CMeterWindow::ShowMeter(const std::wstring& name, bool group)
{
	const WCHAR* meter = name.c_str();

	std::list<CMeter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		if (CompareName((*j), meter, group))
		{
			(*j)->Show();
			m_ResetRegion = true;	// Need to recalculate the window region
			if (!group) return;
		}
	}

	if (!group) LogWithArgs(LOG_NOTICE, L"!ShowMeter: [%s] not found in \"%s\"", meter, m_SkinName.c_str());
}

/*
** Hides the given meter
**
*/
void CMeterWindow::HideMeter(const std::wstring& name, bool group)
{
	const WCHAR* meter = name.c_str();

	std::list<CMeter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		if (CompareName((*j), meter, group))
		{
			(*j)->Hide();
			m_ResetRegion = true;	// Need to recalculate the windowregion
			if (!group) return;
		}
	}

	if (!group) LogWithArgs(LOG_ERROR, L"!HideMeter: [%s] not found in \"%s\"", meter, m_SkinName.c_str());
}

/*
** Toggles the given meter
**
*/
void CMeterWindow::ToggleMeter(const std::wstring& name, bool group)
{
	const WCHAR* meter = name.c_str();

	std::list<CMeter*>::const_iterator j = m_Meters.begin();
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
			m_ResetRegion = true;	// Need to recalculate the window region
			if (!group) return;
		}
	}

	if (!group) LogWithArgs(LOG_ERROR, L"!ToggleMeter: [%s] not found in \"%s\"", meter, m_SkinName.c_str());
}

/*
** Moves the given meter
**
*/
void CMeterWindow::MoveMeter(const std::wstring& name, int x, int y)
{
	const WCHAR* meter = name.c_str();

	std::list<CMeter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		if (CompareName((*j), meter, false))
		{
			(*j)->SetX(x);
			(*j)->SetY(y);
			m_ResetRegion = true;	// Need to recalculate the window region
			return;
		}
	}

	LogWithArgs(LOG_ERROR, L"!MoveMeter: [%s] not found in \"%s\"", meter, m_SkinName.c_str());
}

/*
** Updates the given meter
**
*/
void CMeterWindow::UpdateMeter(const std::wstring& name, bool group)
{
	const WCHAR* meter = name.c_str();

	bool bActiveTransition = false;
	bool bContinue = true;
	std::list<CMeter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		if (bContinue && CompareName((*j), meter, group))
		{
			UpdateMeter((*j), bActiveTransition, true);
			m_ResetRegion = true;	// Need to recalculate the windowregion
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

	if (!group && bContinue) LogWithArgs(LOG_ERROR, L"!UpdateMeter: [%s] not found in \"%s\"", meter, m_SkinName.c_str());
}

/*
** Enables the given measure
**
*/
void CMeterWindow::EnableMeasure(const std::wstring& name, bool group)
{
	const WCHAR* measure = name.c_str();

	std::list<CMeasure*>::const_iterator i = m_Measures.begin();
	for ( ; i != m_Measures.end(); ++i)
	{
		if (CompareName((*i), measure, group))
		{
			(*i)->Enable();
			if (!group) return;
		}
	}

	if (!group) LogWithArgs(LOG_ERROR, L"!EnableMeasure: [%s] not found in \"%s\"", measure, m_SkinName.c_str());
}

/*
** Disables the given measure
**
*/
void CMeterWindow::DisableMeasure(const std::wstring& name, bool group)
{
	const WCHAR* measure = name.c_str();

	std::list<CMeasure*>::const_iterator i = m_Measures.begin();
	for ( ; i != m_Measures.end(); ++i)
	{
		if (CompareName((*i), measure, group))
		{
			(*i)->Disable();
			if (!group) return;
		}
	}

	if (!group) LogWithArgs(LOG_ERROR, L"!DisableMeasure: [%s] not found in \"%s\"", measure, m_SkinName.c_str());
}

/*
** Toggless the given measure
**
*/
void CMeterWindow::ToggleMeasure(const std::wstring& name, bool group)
{
	const WCHAR* measure = name.c_str();

	std::list<CMeasure*>::const_iterator i = m_Measures.begin();
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

	if (!group) LogWithArgs(LOG_ERROR, L"!ToggleMeasure: [%s] not found in \"%s\"", measure, m_SkinName.c_str());
}

/*
** Updates the given measure
**
*/
void CMeterWindow::UpdateMeasure(const std::wstring& name, bool group)
{
	const WCHAR* measure = name.c_str();

	bool bNetStats = m_HasNetMeasures;
	std::list<CMeasure*>::const_iterator i = m_Measures.begin();
	for ( ; i != m_Measures.end(); ++i)
	{
		if (CompareName((*i), measure, group))
		{
			if (bNetStats && dynamic_cast<CMeasureNet*>(*i) != NULL)
			{
				CMeasureNet::UpdateIFTable();
				CMeasureNet::UpdateStats();
				bNetStats = false;
			}

			UpdateMeasure((*i), true);
			if (!group) return;
		}
	}

	if (!group) LogWithArgs(LOG_ERROR, L"!UpdateMeasure: [%s] not found in \"%s\"", measure, m_SkinName.c_str());
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
	if (group)
	{
		for (std::list<CMeter*>::const_iterator j = m_Meters.begin(); j != m_Meters.end(); ++j)
		{
			if ((*j)->BelongsToGroup(section))
			{
				// Force DynamicVariables temporarily (it will reset back to original setting in ReadConfig())
				(*j)->SetDynamicVariables(true);

				if (value.empty())
				{
					GetParser().DeleteValue((*j)->GetOriginalName(), option);
				}
				else
				{
					GetParser().SetValue((*j)->GetOriginalName(), option, value);
				}
			}
		}

		for (std::list<CMeasure*>::const_iterator i = m_Measures.begin(); i != m_Measures.end(); ++i)
		{
			if ((*i)->BelongsToGroup(section))
			{
				// Force DynamicVariables temporarily (it will reset back to original setting in ReadConfig())
				(*i)->SetDynamicVariables(true);

				if (value.empty())
				{
					GetParser().DeleteValue((*i)->GetOriginalName(), option);
				}
				else
				{
					GetParser().SetValue((*i)->GetOriginalName(), option, value);
				}
			}
		}
	}
	else
	{
		CMeter* meter = GetMeter(section);
		if (meter)
		{
			// Force DynamicVariables temporarily (it will reset back to original setting in ReadConfig())
			meter->SetDynamicVariables(true);

			if (value.empty())
			{
				GetParser().DeleteValue(section, option);
			}
			else
			{
				GetParser().SetValue(section, option, value);
			}

			return;
		}

		CMeasure* measure = GetMeasure(section);
		if (measure)
		{
			// Force DynamicVariables temporarily (it will reset back to original setting in ReadConfig())
			measure->SetDynamicVariables(true);

			if (value.empty())
			{
				GetParser().DeleteValue(section, option);
			}
			else
			{
				GetParser().SetValue(section, option, value);
			}

			return;
		}

		// Is it a style?
	}
}

/* WindowToScreen
**
** Calculates the screen cordinates from the WindowX/Y config
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

	const MULTIMONITOR_INFO& multimonInfo = CSystem::GetMultiMonitorInfo();
	const std::vector<MONITOR_INFO>& monitors = multimonInfo.monitors;

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

	const MULTIMONITOR_INFO& multimonInfo = CSystem::GetMultiMonitorInfo();
	const std::vector<MONITOR_INFO>& monitors = multimonInfo.monitors;

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
	if (m_WindowXFromRight == true)
	{
		pixel = (screenx + screenw) - m_ScreenX;
		pixel -= m_AnchorScreenX;
	}
	else
	{
		pixel = m_ScreenX - screenx;
		pixel += m_AnchorScreenX;
	}
	if (m_WindowXPercentage == true)
	{
		num = 100.0f * (float)pixel / (float)screenw;
		_snwprintf_s(buffer, _TRUNCATE, L"%.5f%%", num);
	}
	else
	{
		_itow_s(pixel, buffer, 10);
	}
	if (m_WindowXFromRight == true)
	{
		_snwprintf_s(buffer, _TRUNCATE, L"%sR", buffer);
	}
	if (m_WindowXScreenDefined == true)
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
	if (m_WindowYFromBottom == true)
	{
		pixel = (screeny + screenh) - m_ScreenY;
		pixel -= m_AnchorScreenY;
	}
	else
	{
		pixel = m_ScreenY - screeny;
		pixel += m_AnchorScreenY;
	}
	if (m_WindowYPercentage == true)
	{
		num = 100.0f * (float)pixel / (float)screenh;
		_snwprintf_s(buffer, _TRUNCATE, L"%.5f%%", num);
	}
	else
	{
		_itow_s(pixel, buffer, 10);
	}
	if (m_WindowYFromBottom == true)
	{
		_snwprintf_s(buffer, _TRUNCATE, L"%sB", buffer);
	}
	if (m_WindowYScreenDefined == true)
	{
		_snwprintf_s(buffer, _TRUNCATE, L"%s@%i", buffer, m_WindowYScreen);
	}
	m_WindowY = buffer;
}

/*
** Reads the current config
**
*/
void CMeterWindow::ReadConfig()
{
	WCHAR buffer[32];
	const std::wstring& iniFile = m_Rainmeter->GetIniFile();
	const WCHAR* section = L"Rainmeter";

	// Reset settings to the default value
	m_WindowX = L'0';
	m_WindowY = L'0';
	m_AnchorX = L'0';
	m_AnchorY = L'0';
	m_WindowZPosition = ZPOSITION_NORMAL;
	m_WindowDraggable = true;
	m_WindowHide = HIDEMODE_NONE;
	m_WindowStartHidden = false;
	m_SavePosition = true;
	m_SnapEdges = true;
	m_NativeTransparency = true;
	m_ClickThrough = false;
	m_KeepOnScreen = true;
	m_AutoSelectScreen = false;
	m_AlphaValue = 255;
	m_FadeDuration = 250;
	m_ConfigGroup.clear();

	CConfigParser parser;
	parser.Initialize(iniFile.c_str(), m_Rainmeter, NULL, m_SkinName.c_str());

	for (int i = 0; i < 2; ++i)
	{
		// Check if the window position should be read as a formula
		double value;
		m_WindowX = parser.ReadString(section, L"WindowX", m_WindowX.c_str());
		if (!m_WindowX.empty() && m_WindowX[0] == L'(' && m_WindowX[m_WindowX.size() - 1] == L')')
		{
			if (!parser.ParseFormula(m_WindowX, &value))
			{
				value = 0.0;
			}
			_itow_s((int)value, buffer, 10);
			m_WindowX = buffer;
		}
		m_WindowY = parser.ReadString(section, L"WindowY", m_WindowY.c_str());
		if (!m_WindowY.empty() && m_WindowY[0] == L'(' && m_WindowY[m_WindowY.size() - 1] == L')')
		{
			if (!parser.ParseFormula(m_WindowY, &value))
			{
				value = 0.0;
			}
			_itow_s((int)value, buffer, 10);
			m_WindowY = buffer;
		}

		m_AnchorX = parser.ReadString(section, L"AnchorX", m_AnchorX.c_str());
		m_AnchorY = parser.ReadString(section, L"AnchorY", m_AnchorY.c_str());

		int zPos = parser.ReadInt(section, L"AlwaysOnTop", m_WindowZPosition);
		m_WindowZPosition = (zPos >= ZPOSITION_ONDESKTOP && zPos <= ZPOSITION_ONTOPMOST) ? (ZPOSITION)zPos : ZPOSITION_NORMAL;

		int hideMode = parser.ReadInt(section, L"HideOnMouseOver", m_WindowHide);
		m_WindowHide = (hideMode >= HIDEMODE_NONE && hideMode <= HIDEMODE_FADEOUT) ? (HIDEMODE)hideMode : HIDEMODE_NONE;

		m_WindowDraggable = 0!=parser.ReadInt(section, L"Draggable", m_WindowDraggable);
		m_WindowStartHidden = 0!=parser.ReadInt(section, L"StartHidden", m_WindowStartHidden);
		m_SavePosition = 0!=parser.ReadInt(section, L"SavePosition", m_SavePosition);
		m_SnapEdges = 0!=parser.ReadInt(section, L"SnapEdges", m_SnapEdges);
		m_NativeTransparency = 0!=parser.ReadInt(section, L"NativeTransparency", m_NativeTransparency);
		m_ClickThrough = 0!=parser.ReadInt(section, L"ClickThrough", m_ClickThrough);
		m_KeepOnScreen = 0!=parser.ReadInt(section, L"KeepOnScreen", m_KeepOnScreen);
		m_AutoSelectScreen = 0!=parser.ReadInt(section, L"AutoSelectScreen", m_AutoSelectScreen);

		m_AlphaValue = parser.ReadInt(section, L"AlphaValue", m_AlphaValue);
		m_AlphaValue = max(m_AlphaValue, 0);
		m_AlphaValue = min(m_AlphaValue, 255);

		m_FadeDuration = parser.ReadInt(section, L"FadeDuration", m_FadeDuration);

		m_ConfigGroup = parser.ReadString(section, L"Group", m_ConfigGroup.c_str());

		// On the second loop override settings from the skin's section
		section = m_SkinName.c_str();
	}

	// Set WindowXScreen/WindowYScreen temporarily
	WindowToScreen();
}

/*
** Writes the new settings to the config
**
*/
void CMeterWindow::WriteConfig(INT setting)
{
	const WCHAR* iniFile = m_Rainmeter->GetIniFile().c_str();

	if (*iniFile)
	{
		WCHAR buffer[32];
		const WCHAR* section = m_SkinName.c_str();

		if (setting != SETTING_ALL)
		{
			CDialogManage::UpdateSkins(this);
		}

		if (setting & SETTING_WINDOWPOSITION)
		{
			// If position needs to be save, do so.
			if (m_SavePosition)
			{
				ScreenToWindow();
				WritePrivateProfileString(section, L"WindowX", m_WindowX.c_str(), iniFile);
				WritePrivateProfileString(section, L"WindowY", m_WindowY.c_str(), iniFile);
			}
		}

		if (setting & SETTING_ALPHAVALUE)
		{
			_itow_s(m_AlphaValue, buffer, 10);
			WritePrivateProfileString(section, L"AlphaValue", buffer, iniFile);
		}

		if (setting & SETTING_FADEDURATION)
		{
			_itow_s(m_FadeDuration, buffer, 10);
			WritePrivateProfileString(section, L"FadeDuration", buffer, iniFile);
		}

		if (setting & SETTING_CLICKTHROUGH)
		{
			WritePrivateProfileString(section, L"ClickThrough", m_ClickThrough ? L"1" : L"0", iniFile);
		}

		if (setting & SETTING_WINDOWDRAGGABLE)
		{
			WritePrivateProfileString(section, L"Draggable", m_WindowDraggable ? L"1" : L"0", iniFile);
		}

		if (setting & SETTING_HIDEONMOUSEOVER)
		{
			_itow_s(m_WindowHide, buffer, 10);
			WritePrivateProfileString(section, L"HideOnMouseOver", buffer, iniFile);
		}

		if (setting & SETTING_SAVEPOSITION)
		{
			WritePrivateProfileString(section, L"SavePosition", m_SavePosition ? L"1" : L"0", iniFile);
		}

		if (setting & SETTING_SNAPEDGES)
		{
			WritePrivateProfileString(section, L"SnapEdges", m_SnapEdges ? L"1" : L"0", iniFile);
		}

		if (setting & SETTING_KEEPONSCREEN)
		{
			WritePrivateProfileString(section, L"KeepOnScreen", m_KeepOnScreen ? L"1" : L"0", iniFile);
		}

		if (setting & SETTING_AUTOSELECTSCREEN)
		{
			WritePrivateProfileString(section, L"AutoSelectScreen", m_AutoSelectScreen ? L"1" : L"0", iniFile);
		}

		if (setting & SETTING_ALWAYSONTOP)
		{
			_itow_s(m_WindowZPosition, buffer, 10);
			WritePrivateProfileString(section, L"AlwaysOnTop", buffer, iniFile);
		}
	}
}

/*
** Reads the skin config, creates the meters and measures and does the bindings.
**
*/
bool CMeterWindow::ReadSkin()
{
	WCHAR buffer[128];

	std::wstring iniFile = m_Rainmeter->GetSkinPath() + m_SkinName;
	iniFile += L'\\';
	iniFile += m_SkinIniFile;

	// Verify whether the file exists
	if (_waccess(iniFile.c_str(), 0) == -1)
	{
		std::wstring message = GetFormattedString(ID_STR_UNABLETOREFRESHSKIN, m_SkinName.c_str(), m_SkinIniFile.c_str());
		MessageBox(m_Window, message.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
		return false;
	}

	m_Parser.Initialize(iniFile.c_str(), m_Rainmeter, this);

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

		std::wstring text = GetFormattedString(ID_STR_NEWVERSIONREQUIRED, m_SkinName.c_str(), m_SkinIniFile.c_str(), buffer);
		MessageBox(m_Window, text.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
		return false;
	}

	// Initialize window variables
	SetWindowPositionVariables(m_ScreenX, m_ScreenY);
	SetWindowSizeVariables(0, 0);

	// Global settings
	const std::wstring& group = m_Parser.ReadString(L"Rainmeter", L"Group", L"");
	if (!group.empty())
	{
		m_ConfigGroup += L'|';
		m_ConfigGroup += group;
	}
	InitializeGroup(m_ConfigGroup);

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

	m_LeftMouseDownAction = m_Parser.ReadString(L"Rainmeter", L"LeftMouseDownAction", L"", false);
	m_RightMouseDownAction = m_Parser.ReadString(L"Rainmeter", L"RightMouseDownAction", L"", false);
	m_MiddleMouseDownAction = m_Parser.ReadString(L"Rainmeter", L"MiddleMouseDownAction", L"", false);
	m_LeftMouseUpAction = m_Parser.ReadString(L"Rainmeter", L"LeftMouseUpAction", L"", false);
	m_RightMouseUpAction = m_Parser.ReadString(L"Rainmeter", L"RightMouseUpAction", L"", false);
	m_MiddleMouseUpAction = m_Parser.ReadString(L"Rainmeter", L"MiddleMouseUpAction", L"", false);
	m_LeftMouseDoubleClickAction = m_Parser.ReadString(L"Rainmeter", L"LeftMouseDoubleClickAction", L"", false);
	m_RightMouseDoubleClickAction = m_Parser.ReadString(L"Rainmeter", L"RightMouseDoubleClickAction", L"", false);
	m_MiddleMouseDoubleClickAction = m_Parser.ReadString(L"Rainmeter", L"MiddleMouseDoubleClickAction", L"", false);
	m_MouseOverAction = m_Parser.ReadString(L"Rainmeter", L"MouseOverAction", L"", false);
	m_MouseLeaveAction = m_Parser.ReadString(L"Rainmeter", L"MouseLeaveAction", L"", false);
	m_OnRefreshAction = m_Parser.ReadString(L"Rainmeter", L"OnRefreshAction", L"", false);

	m_WindowUpdate = m_Parser.ReadInt(L"Rainmeter", L"Update", INTERVAL_METER);
	m_TransitionUpdate = m_Parser.ReadInt(L"Rainmeter", L"TransitionUpdate", INTERVAL_TRANSITION);
	m_MouseActionCursor = 0 != m_Parser.ReadInt(L"Rainmeter", L"MouseActionCursor", 1);
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

					// Here we are checking to see if there are more than one blur region
					// to be loaded. They will be named BlurRegion2, BlurRegion3, etc.
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

	// Checking for localfonts
	const WCHAR* localFont = m_Parser.ReadString(L"Rainmeter", L"LocalFont", L"").c_str();
	// If there is a local font we want to load it
	if (*localFont)
	{
		m_FontCollection = new PrivateFontCollection();
		int i = 1;

		do
		{
			// We want to check the fonts folder first
			// !!!!!!! - We may want to fix the method in which I get the path to
			// Rainmeter/fonts
			std::wstring szFontFile = m_Rainmeter->GetPath() + L"Fonts\\";
			szFontFile += localFont;
			Status nResults = m_FontCollection->AddFontFile(szFontFile.c_str());

			// It wasn't found in the fonts folder, check the local folder
			if (nResults != Ok)
			{
				szFontFile = m_Rainmeter->GetSkinPath(); // Get the local path
				szFontFile += m_SkinName;
				szFontFile += L'\\';
				szFontFile += localFont;
				nResults = m_FontCollection->AddFontFile(szFontFile.c_str());

				// The font wasn't found, check full path.
				if (nResults != Ok)
				{
					szFontFile = localFont;
					nResults = m_FontCollection->AddFontFile(szFontFile.c_str());

					if (nResults != Ok)
					{
						std::wstring error = L"Unable to load font file: ";
						error += localFont;
						Log(LOG_ERROR, error.c_str());
					}
				}
			}

			// Here we are checking to see if there are more than one local font
			// to be loaded. They will be named LocalFont2, LocalFont3, etc.
			_snwprintf_s(buffer, _TRUNCATE, L"LocalFont%i", ++i);
			localFont = m_Parser.ReadString(L"Rainmeter", buffer, L"").c_str();
		}
		while (*localFont);
	}

	// Create the meters and measures

	m_HasNetMeasures = false;
	m_HasButtons = false;

	// Get all the sections (i.e. different meters, measures and the other stuff)
	std::list<std::wstring>::const_iterator iter = m_Parser.GetSections().begin();
	for ( ; iter != m_Parser.GetSections().end(); ++iter)
	{
		const WCHAR* section = (*iter).c_str();

		if (_wcsicmp(L"Rainmeter", section) != 0 &&
			_wcsicmp(L"Variables", section) != 0 &&
			_wcsicmp(L"Metadata", section) != 0)
		{
			// Check if the item is a meter or a measure (or perhaps something else)
			const std::wstring& measureName = m_Parser.ReadString(section, L"Measure", L"", false);
			if (!measureName.empty())
			{
				// It's a measure
				CMeasure* measure = NULL;

				try
				{
					measure = CMeasure::Create(measureName.c_str(), this, section);
					if (measure)
					{
						measure->ReadConfig(m_Parser);

						m_Measures.push_back(measure);
						m_Parser.AddMeasure(measure);

						if (!m_HasNetMeasures && dynamic_cast<CMeasureNet*>(measure))
						{
							m_HasNetMeasures = true;
						}
					}
				}
				catch (CError& error)
				{
					delete measure;
					measure = NULL;
					LogError(error);
				}

				continue;
			}

			const std::wstring& meterName = m_Parser.ReadString(section, L"Meter", L"", false);
			if (!meterName.empty())
			{
				// It's a meter
				CMeter* meter = NULL;

				try
				{
					meter = CMeter::Create(meterName.c_str(), this, section);
					if (meter)
					{
						meter->ReadConfig(m_Parser);

						m_Meters.push_back(meter);

						if (!m_HasButtons && dynamic_cast<CMeterButton*>(meter))
						{
							m_HasButtons = true;
						}
					}
				}
				catch (CError& error)
				{
					delete meter;
					meter = NULL;
					LogError(error);
				}

				continue;
			}

			// If it's not a meter or measure it will be ignored
		}
	}

	if (m_Meters.empty())
	{
		std::wstring text = GetFormattedString(ID_STR_NOMETERSINSKIN, m_SkinName.c_str(), m_SkinIniFile.c_str());
		MessageBox(m_Window, text.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
		return false;
	}
	else
	{
		// Bind the meters to the measures
		std::list<CMeter*>::const_iterator j = m_Meters.begin();
		for ( ; j != m_Meters.end(); ++j)
		{
			try
			{
				(*j)->BindMeasure(m_Measures);
			}
			catch (CError& error)
			{
				LogError(error);
			}
		}
	}

	return true;
}

/*
** Initializes all the measures
**
*/
void CMeterWindow::InitializeMeasures()
{
	// Initalize all measures
	std::list<CMeasure*>::const_iterator i = m_Measures.begin();
	for ( ; i != m_Measures.end(); ++i)
	{
		try
		{
			(*i)->Initialize();
		}
		catch (CError& error)
		{
			LogError(error);
		}
	}
}

/*
** Initializes all the meters and the background
**
*/
void CMeterWindow::InitializeMeters()
{
	// Initalize all meters
	std::list<CMeter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		try
		{
			(*j)->Initialize();
		}
		catch (CError& error)
		{
			LogError(error);
		}

		if (!(*j)->GetToolTipText().empty())
		{
			(*j)->CreateToolTip(this);
		}
	}

	Update(true);
	ResizeWindow(true);
}

/*
** Changes the size of the window and re-adjusts the background
*/
bool CMeterWindow::ResizeWindow(bool reset)
{
	int w = m_BackgroundMargins.left;
	int h = m_BackgroundMargins.top;

	// Get the largest meter point
	std::list<CMeter*>::const_iterator j = m_Meters.begin();
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
		tintedBackground->ReadConfig(m_Parser, L"Rainmeter");
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
			//Calculate the window position from the config parameters
			WindowToScreen();

			if (!m_NativeTransparency)
			{
				// Graph the desktop and place the background on top of it
				Bitmap* desktop = GrabDesktop(m_ScreenX, m_ScreenY, m_WindowW, m_WindowH);
				Graphics graphics(desktop);
				Rect r(0, 0, m_WindowW, m_WindowH);
				graphics.DrawImage(m_Background, r, 0, 0, m_WindowW, m_WindowH, UnitPixel);
				delete m_Background;
				m_Background = desktop;
			}
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

	if (!m_NativeTransparency)
	{
		// If Background is not set, take a copy from the desktop
		if (m_Background == NULL && m_BackgroundMode == BGMODE_COPY)
		{
			m_Background = GrabDesktop(m_ScreenX, m_ScreenY, m_WindowW, m_WindowH);
		}
	}

	return true;
}

/*
** Grabs a part of the desktop
*/
Bitmap* CMeterWindow::GrabDesktop(int x, int y, int w, int h)
{
	HDC desktopDC = GetDC(0);
	HDC dc = CreateCompatibleDC(desktopDC);
	HBITMAP desktopBM = CreateCompatibleBitmap(desktopDC, w, h);
	HBITMAP oldBM = (HBITMAP)SelectObject(dc, desktopBM);
	BitBlt(dc, 0, 0, w, h, desktopDC, x, y, SRCCOPY);
	SelectObject(dc, oldBM);
	DeleteDC(dc);
	ReleaseDC(0, desktopDC);
	Bitmap* background = new Bitmap(desktopBM, NULL);
	DeleteObject(desktopBM);
	return background;
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
** Creates/Clears a window region
**
*/
void CMeterWindow::CreateRegion(bool clear)
{
	if (clear)
	{
		SetWindowRgn(m_Window, NULL, TRUE);
	}
	else
	{
		HRGN region = NULL;

		// Set window region if needed
		if (!m_BackgroundName.empty())
		{
			if (m_WindowW != 0 && m_WindowH != 0)
			{
				HBITMAP background = NULL;
				m_DoubleBuffer->GetHBITMAP(Color(255,0,255), &background);
				if (background)
				{
					region = BitmapToRegion(background, RGB(255,0,255), 0x101010);
					DeleteObject(background);
				}
			}
		}

		SetWindowRgn(m_Window, region, !m_NativeTransparency);
	}
}

/*
** Redraws the meters and paints the window
**
*/
void CMeterWindow::Redraw()
{
	if (m_ResetRegion)
	{
		ResizeWindow(false);
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
		std::list<CMeter*>::const_iterator j = m_Meters.begin();
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

	if (m_ResetRegion || !m_BackgroundName.empty())
	{
		CreateRegion(false);
		m_ResetRegion = false;
	}

	UpdateTransparency(m_TransparencyValue, false);

	if (!m_NativeTransparency)
	{
		InvalidateRect(m_Window, NULL, FALSE);
	}
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
	if (updateDivider >= 0 || force || m_Refreshing)
	{
		if (measure->HasDynamicVariables() &&
			(measure->GetUpdateCounter() + 1) >= updateDivider)
		{
			try
			{
				measure->ReadConfig(m_Parser);
			}
			catch (CError& error)
			{
				LogError(error);
			}
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
	if (updateDivider >= 0 || force || m_Refreshing)
	{
		if (meter->HasDynamicVariables() &&
			(meter->GetUpdateCounter() + 1) >= updateDivider)
		{
			try
			{
				meter->ReadConfig(m_Parser);
			}
			catch (CError& error)
			{
				LogError(error);
			}
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
void CMeterWindow::Update(bool nodraw)
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
		std::list<CMeasure*>::const_iterator i = m_Measures.begin();
		for ( ; i != m_Measures.end(); ++i)
		{
			UpdateMeasure((*i), false);
		}
	}

	// Update all meters
	bool bActiveTransition = false;
	bool bUpdate = false;
	std::list<CMeter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		if (UpdateMeter((*j), bActiveTransition, false))
		{
			bUpdate = true;
		}
	}

	// Redraw all meters
	if (!nodraw && (bUpdate || m_ResetRegion || m_Refreshing))
	{
		if (m_DynamicWindowSize)
		{
			// Resize the window
			m_ResetRegion = true;
		}

		// If our option is to disable when in an RDP session, then check if in an RDP session.
		// Only redraw if we are not in a remote session
		if (m_Rainmeter->IsRedrawable())
		{
			Redraw();
		}
	}

	// Post-updates
	PostUpdate(bActiveTransition);
}

/*
** Updates the native Windows transparency
*/
void CMeterWindow::UpdateTransparency(int alpha, bool reset)
{
	if (m_NativeTransparency)
	{
		if (reset)
		{
			// Add the window flag
			LONG style = GetWindowLong(m_Window, GWL_EXSTYLE);
			SetWindowLong(m_Window, GWL_EXSTYLE, style | WS_EX_LAYERED);
		}

		BLENDFUNCTION blendPixelFunction = {AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA};
		POINT ptWindowScreenPosition = {m_ScreenX, m_ScreenY};
		POINT ptSrc = {0, 0};
		SIZE szWindow = {m_DIBSectionBufferW, m_DIBSectionBufferH};

		HDC dcScreen = GetDC(0);
		HDC dcMemory = CreateCompatibleDC(dcScreen);
		SelectObject(dcMemory, m_DIBSectionBuffer);

		UpdateLayeredWindow(m_Window, dcScreen, &ptWindowScreenPosition, &szWindow, dcMemory, &ptSrc, 0, &blendPixelFunction, ULW_ALPHA);

		ReleaseDC(0, dcScreen);
		DeleteDC(dcMemory);

		m_TransparencyValue = alpha;
	}
	else
	{
		if (reset)
		{
			// Remove the window flag
			LONG style = GetWindowLong(m_Window, GWL_EXSTYLE);
			SetWindowLong(m_Window, GWL_EXSTYLE, style & ~WS_EX_LAYERED);
		}
	}
}

/*
** Repaints the window. This does not cause update of the measures.
** This handler is called if NativeTransparency is false.
**
*/
LRESULT CMeterWindow::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC winDC = BeginPaint(m_Window, &ps);

	Graphics graphics(winDC);
	graphics.DrawImage(m_DoubleBuffer, 0, 0);

	EndPaint(m_Window, &ps);
	return 0;
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
		CDialogAbout::UpdateMeasures(this);
	}
	else if (wParam == TIMER_MOUSE)
	{
		if (!m_Rainmeter->IsMenuActive() && !m_Dragging)
		{
			ShowWindowIfAppropriate();

			if (m_WindowZPosition == ZPOSITION_ONTOPMOST)
			{
				ChangeZPos(ZPOSITION_ONTOPMOST);
			}

			if (m_MouseOver)
			{
				POINT pos;
				GetCursorPos(&pos);

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
		std::list<CMeter*>::const_iterator j = m_Meters.begin();
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
				UpdateTransparency(m_FadeEndValue, false);
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

			UpdateTransparency((int)value, false);
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
	if (!m_NativeTransparency || m_FadeDuration == 0)
	{
		if (to == 0)
		{
			ShowWindow(m_Window, SW_HIDE);
		}
		else
		{
			if (m_FadeDuration == 0)
			{
				UpdateTransparency(to, false);
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
		UpdateTransparency(from, false);
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

	POINT pos;
	GetCursorPos(&pos);
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
			LONG style = GetWindowLong(m_Window, GWL_EXSTYLE);
			if ((style & WS_EX_TRANSPARENT) != 0)
			{
				SetWindowLong(m_Window, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
			}
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
	bool drawCursor = false;

	std::list<CMeter*>::const_reverse_iterator j = m_Meters.rbegin();
	for ( ; j != m_Meters.rend(); ++j)
	{
		// Hidden meters are ignored
		if ((*j)->IsHidden()) continue;

		CMeterButton* button = NULL;
		if (m_HasButtons)
		{
			button = dynamic_cast<CMeterButton*>(*j);
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

		if (!drawCursor)
		{
			if ((*j)->HasMouseActionCursor() && (*j)->HitTest(pos.x, pos.y))
			{
				drawCursor = ((*j)->HasMouseAction() || button);
			}
		}
	}

	if (redraw)
	{
		Redraw();
	}

	// Set cursor
	SetCursor(LoadCursor(NULL, drawCursor ? IDC_HAND : IDC_ARROW));
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
			LONG style = GetWindowLong(m_Window, GWL_EXSTYLE);
			if ((style & WS_EX_TRANSPARENT) == 0)
			{
				SetWindowLong(m_Window, GWL_EXSTYLE, style | WS_EX_TRANSPARENT);
			}
		}

		if (!m_Hidden)
		{
			// If Alt, shift or control is down, do not hide the window
			switch (m_WindowHide)
			{
			case HIDEMODE_HIDE:
				if (!m_NativeTransparency || m_TransparencyValue == m_AlphaValue)
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
		pos.x = (SHORT)LOWORD(lParam);
		pos.y = (SHORT)HIWORD(lParam);

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
	POINT pos;
	GetCursorPos(&pos);
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
** Handle the menu commands.
**
*/
LRESULT CMeterWindow::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	try
	{
		if (wParam == IDM_SKIN_EDITSKIN)
		{
			m_Rainmeter->EditSkinFile(m_SkinName, m_SkinIniFile);
		}
		else if (wParam == IDM_SKIN_REFRESH)
		{
			Refresh(false);
		}
		else if (wParam == IDM_SKIN_OPENSKINSFOLDER)
		{
			m_Rainmeter->OpenSkinFolder(m_SkinName);
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
			UpdateTransparency(m_AlphaValue, false);
			WriteConfig(SETTING_ALPHAVALUE);
		}
		else if (wParam == IDM_CLOSESKIN)
		{
			m_Rainmeter->DeactivateConfig(this, -1);
		}
		else if (wParam == IDM_SKIN_FROMRIGHT)
		{
			m_WindowXFromRight = !m_WindowXFromRight;

			ScreenToWindow();
			WriteConfig(SETTING_WINDOWPOSITION);
		}
		else if (wParam == IDM_SKIN_FROMBOTTOM)
		{
			m_WindowYFromBottom = !m_WindowYFromBottom;

			ScreenToWindow();
			WriteConfig(SETTING_WINDOWPOSITION);
		}
		else if (wParam == IDM_SKIN_XPERCENTAGE)
		{
			m_WindowXPercentage = !m_WindowXPercentage;

			ScreenToWindow();
			WriteConfig(SETTING_WINDOWPOSITION);
		}
		else if (wParam == IDM_SKIN_YPERCENTAGE)
		{
			m_WindowYPercentage = !m_WindowYPercentage;

			ScreenToWindow();
			WriteConfig(SETTING_WINDOWPOSITION);
		}
		else if (wParam == IDM_SKIN_MONITOR_AUTOSELECT)
		{
			m_AutoSelectScreen = !m_AutoSelectScreen;

			ScreenToWindow();
			WriteConfig(SETTING_WINDOWPOSITION | SETTING_AUTOSELECTSCREEN);
		}
		else if (wParam == IDM_SKIN_MONITOR_PRIMARY || wParam >= ID_MONITOR_FIRST && wParam <= ID_MONITOR_LAST)
		{
			const MULTIMONITOR_INFO& multimonInfo = CSystem::GetMultiMonitorInfo();
			const std::vector<MONITOR_INFO>& monitors = multimonInfo.monitors;

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
				WriteConfig(SETTING_WINDOWPOSITION | SETTING_AUTOSELECTSCREEN);
			}
		}
		else
		{
			// Forward to tray window, which handles all the other commands
			HWND tray = m_Rainmeter->GetTrayWindow()->GetWindow();

			if (wParam == IDM_QUIT)
			{
				PostMessage(tray, WM_COMMAND, wParam, lParam);
			}
			else
			{
				SendMessage(tray, WM_COMMAND, wParam, lParam);
			}
		}
	}
	catch (CError& error)
	{
		LogError(error);
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
	WriteConfig(SETTING_CLICKTHROUGH);

	if (!m_ClickThrough)
	{
		// Remove transparent flag
		LONG style = GetWindowLong(m_Window, GWL_EXSTYLE);
		if ((style & WS_EX_TRANSPARENT) != 0)
		{
			SetWindowLong(m_Window, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
		}
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
	WriteConfig(SETTING_KEEPONSCREEN);

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
	WriteConfig(SETTING_WINDOWDRAGGABLE);
}

/*
** Helper function for setting SavePosition
**
*/
void CMeterWindow::SetSavePosition(bool b)
{
	m_SavePosition = b;
	WriteConfig(SETTING_WINDOWPOSITION | SETTING_SAVEPOSITION);
}

/*
** Helper function for setting SnapEdges
**
*/
void CMeterWindow::SetSnapEdges(bool b)
{
	m_SnapEdges = b;
	WriteConfig(SETTING_SNAPEDGES);
}

/*
** Helper function for setting WindowHide
**
*/
void CMeterWindow::SetWindowHide(HIDEMODE hide)
{
	m_WindowHide = hide;
	UpdateTransparency(m_AlphaValue, false);
	WriteConfig(SETTING_HIDEONMOUSEOVER);
}

/*
** Helper function for setting Position
**
*/
void CMeterWindow::SetWindowZPosition(ZPOSITION zpos)
{
	ChangeSingleZPos(zpos);
	WriteConfig(SETTING_ALWAYSONTOP);
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

		// Write the new place of the window to config file
		if (m_SavePosition)
		{
			WriteConfig(SETTING_WINDOWPOSITION);
		}

		POINT pos;
		GetCursorPos(&pos);
		MapWindowPoints(NULL, m_Window, &pos, 1);

		// Handle buttons
		HandleButtons(pos, BUTTONPROC_UP, false);  // redraw only

		// Workaround for the system that the window size is changed incorrectly when the window is dragged over the upper side of the virtual screen
		UpdateTransparency(m_TransparencyValue, false);
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
	if (m_WindowDraggable && !m_Rainmeter->GetDisableDragging())
	{
		POINT pos;
		pos.x = (SHORT)LOWORD(lParam);
		pos.y = (SHORT)HIWORD(lParam);
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
		if (m_WindowZPosition == ZPOSITION_NORMAL && m_Rainmeter->IsNormalStayDesktop() && CSystem::GetShowDesktop())
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
				std::map<std::wstring, CMeterWindow*>::const_iterator iter = m_Rainmeter->GetAllMeterWindows().begin();
				for ( ; iter != m_Rainmeter->GetAllMeterWindows().end(); ++iter)
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
	pos.x = (SHORT)LOWORD(lParam);
	pos.y = (SHORT)HIWORD(lParam);

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
	pos.x = (SHORT)LOWORD(lParam);
	pos.y = (SHORT)HIWORD(lParam);

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
	pos.x = (SHORT)LOWORD(lParam);
	pos.y = (SHORT)HIWORD(lParam);

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
	pos.x = (SHORT)LOWORD(lParam);
	pos.y = (SHORT)HIWORD(lParam);

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
	pos.x = (SHORT)LOWORD(lParam);
	pos.y = (SHORT)HIWORD(lParam);

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
	pos.x = (SHORT)LOWORD(lParam);
	pos.y = (SHORT)HIWORD(lParam);

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
	pos.x = (SHORT)LOWORD(lParam);
	pos.y = (SHORT)HIWORD(lParam);

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
	pos.x = (SHORT)LOWORD(lParam);
	pos.y = (SHORT)HIWORD(lParam);

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
	pos.x = (SHORT)LOWORD(lParam);
	pos.y = (SHORT)HIWORD(lParam);

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
		pos.x = (SHORT)LOWORD(lParam);
		pos.y = (SHORT)HIWORD(lParam);

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

	m_Rainmeter->ShowContextMenu(pos, this);

	return 0;
}

/*
** Executes the action if such are defined. Returns true, if action was executed.
** If the test is true, the action is not executed.
**
*/
bool CMeterWindow::DoAction(int x, int y, MOUSE mouse, bool test)
{
	// Check if the hitpoint was over some meter
	std::list<CMeter*>::const_reverse_iterator j = m_Meters.rbegin();
	for ( ; j != m_Meters.rend(); ++j)
	{
		// Hidden meters are ignored
		if ((*j)->IsHidden()) continue;

		if ((*j)->HitTest(x, y))
		{
			switch (mouse)
			{
			case MOUSE_LMB_DOWN:
				if (!((*j)->GetLeftMouseDownAction().empty()))
				{
					if (!test) m_Rainmeter->ExecuteCommand((*j)->GetLeftMouseDownAction().c_str(), this);
					return true;
				}
				break;

			case MOUSE_LMB_UP:
				if (!((*j)->GetLeftMouseUpAction().empty()))
				{
					if (!test) m_Rainmeter->ExecuteCommand((*j)->GetLeftMouseUpAction().c_str(), this);
					return true;
				}
				break;

			case MOUSE_LMB_DBLCLK:
				if (!((*j)->GetLeftMouseDoubleClickAction().empty()))
				{
					if (!test) m_Rainmeter->ExecuteCommand((*j)->GetLeftMouseDoubleClickAction().c_str(), this);
					return true;
				}
				break;

			case MOUSE_RMB_DOWN:
				if (!((*j)->GetRightMouseDownAction().empty()))
				{
					if (!test) m_Rainmeter->ExecuteCommand((*j)->GetRightMouseDownAction().c_str(), this);
					return true;
				}
				break;

			case MOUSE_RMB_UP:
				if (!((*j)->GetRightMouseUpAction().empty()))
				{
					if (!test) m_Rainmeter->ExecuteCommand((*j)->GetRightMouseUpAction().c_str(), this);
					return true;
				}
				break;

			case MOUSE_RMB_DBLCLK:
				if (!((*j)->GetRightMouseDoubleClickAction().empty()))
				{
					if (!test) m_Rainmeter->ExecuteCommand((*j)->GetRightMouseDoubleClickAction().c_str(), this);
					return true;
				}
				break;

			case MOUSE_MMB_DOWN:
				if (!((*j)->GetMiddleMouseDownAction().empty()))
				{
					if (!test) m_Rainmeter->ExecuteCommand((*j)->GetMiddleMouseDownAction().c_str(), this);
					return true;
				}
				break;

			case MOUSE_MMB_UP:
				if (!((*j)->GetMiddleMouseUpAction().empty()))
				{
					if (!test) m_Rainmeter->ExecuteCommand((*j)->GetMiddleMouseUpAction().c_str(), this);
					return true;
				}
				break;

			case MOUSE_MMB_DBLCLK:
				if (!((*j)->GetMiddleMouseDoubleClickAction().empty()))
				{
					if (!test) m_Rainmeter->ExecuteCommand((*j)->GetMiddleMouseDoubleClickAction().c_str(), this);
					return true;
				}
				break;
			}
		}
	}

	if (HitTest(x, y))
	{
		// If no meters caused actions, do the default actions
		switch (mouse)
		{
		case MOUSE_LMB_DOWN:
			if (!m_LeftMouseDownAction.empty())
			{
				if (!test) m_Rainmeter->ExecuteCommand(m_LeftMouseDownAction.c_str(), this);
				return true;
			}
			break;

		case MOUSE_LMB_UP:
			if (!m_LeftMouseUpAction.empty())
			{
				if (!test) m_Rainmeter->ExecuteCommand(m_LeftMouseUpAction.c_str(), this);
				return true;
			}
			break;

		case MOUSE_LMB_DBLCLK:
			if (!m_LeftMouseDoubleClickAction.empty())
			{
				if (!test) m_Rainmeter->ExecuteCommand(m_LeftMouseDoubleClickAction.c_str(), this);
				return true;
			}
			break;

		case MOUSE_RMB_DOWN:
			if (!m_RightMouseDownAction.empty())
			{
				if (!test) m_Rainmeter->ExecuteCommand(m_RightMouseDownAction.c_str(), this);
				return true;
			}
			break;

		case MOUSE_RMB_UP:
			if (!m_RightMouseUpAction.empty())
			{
				if (!test) m_Rainmeter->ExecuteCommand(m_RightMouseUpAction.c_str(), this);
				return true;
			}
			break;

		case MOUSE_RMB_DBLCLK:
			if (!m_RightMouseDoubleClickAction.empty())
			{
				if (!test) m_Rainmeter->ExecuteCommand(m_RightMouseDoubleClickAction.c_str(), this);
				return true;
			}
			break;

		case MOUSE_MMB_DOWN:
			if (!m_MiddleMouseDownAction.empty())
			{
				if (!test) m_Rainmeter->ExecuteCommand(m_MiddleMouseDownAction.c_str(), this);
				return true;
			}
			break;

		case MOUSE_MMB_UP:
			if (!m_MiddleMouseUpAction.empty())
			{
				if (!test) m_Rainmeter->ExecuteCommand(m_MiddleMouseUpAction.c_str(), this);
				return true;
			}
			break;

		case MOUSE_MMB_DBLCLK:
			if (!m_MiddleMouseDoubleClickAction.empty())
			{
				if (!test) m_Rainmeter->ExecuteCommand(m_MiddleMouseDoubleClickAction.c_str(), this);
				return true;
			}
			break;
		}
	}

	return false;
}

/*
** Executes the action if such are defined. Returns true, if meter/window which should be processed still may exist.
**
*/
bool CMeterWindow::DoMoveAction(int x, int y, MOUSE mouse)
{
	bool buttonFound = false;

	// Check if the hitpoint was over some meter
	std::list<CMeter*>::const_reverse_iterator j = m_Meters.rbegin();
	for ( ; j != m_Meters.rend(); ++j)
	{
		if (!(*j)->IsHidden() && (*j)->HitTest(x, y))
		{
			if (mouse == MOUSE_OVER)
			{
				if (!m_MouseOver)
				{
					// If the mouse is over a meter it's also over the main window
					//LogWithArgs(LOG_DEBUG, L"@Enter: %s", m_SkinName.c_str());
					m_MouseOver = true;
					SetMouseLeaveEvent(false);

					if (!m_MouseOverAction.empty())
					{
						UINT currCounter = m_MouseMoveCounter;
						m_Rainmeter->ExecuteCommand(m_MouseOverAction.c_str(), this);
						return (currCounter == m_MouseMoveCounter);
					}
				}

				// Handle button
				CMeterButton* button = NULL;
				if (m_HasButtons)
				{
					button = dynamic_cast<CMeterButton*>(*j);
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
					if (!((*j)->GetMouseOverAction().empty()) ||
						!((*j)->GetMouseLeaveAction().empty()) ||
						button)
					{
						//LogWithArgs(LOG_DEBUG, L"MeterEnter: %s - [%s]", m_SkinName.c_str(), (*j)->GetName());
						(*j)->SetMouseOver(true);

						if (!((*j)->GetMouseOverAction().empty()))
						{
							UINT currCounter = m_MouseMoveCounter;
							m_Rainmeter->ExecuteCommand((*j)->GetMouseOverAction().c_str(), this);
							return (currCounter == m_MouseMoveCounter);
						}
					}
				}
			}
		}
		else
		{
			if (mouse == MOUSE_LEAVE)
			{
				if ((*j)->IsMouseOver())
				{
					// Handle button
					if (m_HasButtons)
					{
						CMeterButton* button = dynamic_cast<CMeterButton*>(*j);
						if (button)
						{
							button->SetFocus(false);
						}
					}

					//LogWithArgs(LOG_DEBUG, L"MeterLeave: %s - [%s]", m_SkinName.c_str(), (*j)->GetName());
					(*j)->SetMouseOver(false);

					if (!((*j)->GetMouseLeaveAction().empty()))
					{
						m_Rainmeter->ExecuteCommand((*j)->GetMouseLeaveAction().c_str(), this);
						return true;
					}
				}
			}
		}
	}

	if (HitTest(x, y))
	{
		// If no meters caused actions, do the default actions
		if (mouse == MOUSE_OVER)
		{
			if (!m_MouseOver)
			{
				//LogWithArgs(LOG_DEBUG, L"Enter: %s", m_SkinName.c_str());
				m_MouseOver = true;
				SetMouseLeaveEvent(false);

				if (!m_MouseOverAction.empty())
				{
					UINT currCounter = m_MouseMoveCounter;
					m_Rainmeter->ExecuteCommand(m_MouseOverAction.c_str(), this);
					return (currCounter == m_MouseMoveCounter);
				}
			}
		}
	}
	else
	{
		if (mouse == MOUSE_LEAVE)
		{
			// Mouse leave happens when the mouse is outside the window
			if (m_MouseOver)
			{
				//LogWithArgs(LOG_DEBUG, L"Leave: %s", m_SkinName.c_str());
				m_MouseOver = false;
				SetMouseLeaveEvent(true);

				if (!m_MouseLeaveAction.empty())
				{
					m_Rainmeter->ExecuteCommand(m_MouseLeaveAction.c_str(), this);
					return true;
				}
			}
		}
	}

	return false;
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
	m_ScreenX = (SHORT)LOWORD(lParam);
	m_ScreenY = (SHORT)HIWORD(lParam);

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
	MESSAGE(OnPaint, WM_PAINT)
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
	MESSAGE(OnWindowPosChanging, WM_WINDOWPOSCHANGING)
	MESSAGE(OnCopyData, WM_COPYDATA)
	MESSAGE(OnDelayedRefresh, WM_METERWINDOW_DELAYED_REFRESH)
	MESSAGE(OnDelayedMove, WM_METERWINDOW_DELAYED_MOVE)
	MESSAGE(OnDwmColorChange, WM_DWMCOLORIZATIONCOLORCHANGED)
	MESSAGE(OnDwmCompositionChange, WM_DWMCOMPOSITIONCHANGED)
	MESSAGE(OnSettingChange, WM_SETTINGCHANGE)
	MESSAGE(OnDisplayChange, WM_DISPLAYCHANGE)
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
	try
	{
		Refresh(false);
	}
	catch (CError& error)
	{
		LogError(error);
	}
	return 0;
}

/*
** Handles delayed move
**
*/
LRESULT CMeterWindow::OnDelayedMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_NativeTransparency)
	{
		m_Parser.ResetMonitorVariables(this);

		// Move the window to correct position
		ResizeWindow(true);

		if (m_KeepOnScreen)
		{
			MapCoordsToScreen(m_ScreenX, m_ScreenY, m_WindowW, m_WindowH);
		}

		SetWindowPos(m_Window, NULL, m_ScreenX, m_ScreenY, m_WindowW, m_WindowH, SWP_NOZORDER | SWP_NOACTIVATE);
	}
	else
	{
		// With copy transparency we'll do a full refresh
		PostMessage(m_Window, WM_METERWINDOW_DELAYED_REFRESH, (WPARAM)NULL, (LPARAM)NULL);
	}

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
		std::map<std::wstring, CMeterWindow*>::const_iterator iter = m_Rainmeter->GetAllMeterWindows().begin();
		for ( ; iter != m_Rainmeter->GetAllMeterWindows().end(); ++iter)
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
			m_Rainmeter->ExecuteCommand(command, this);
		}
		else
		{
			// This meterwindow has been deactivated
			Log(LOG_WARNING, L"Unable to bang a deactivated config");
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
		absolute.reserve(m_Rainmeter->GetSkinPath().size() + m_SkinName.size() + 1 + path.size());
		absolute = m_Rainmeter->GetSkinPath();
		absolute += m_SkinName;
		absolute += L'\\';
		absolute += path;
		absolute.swap(path);
	}
}

std::wstring CMeterWindow::GetSkinRootPath()
{
	std::wstring path = m_Rainmeter->GetSkinPath();

	std::wstring::size_type loc;
	if ((loc = m_SkinName.find_first_of(L'\\')) != std::wstring::npos)
	{
		path.append(m_SkinName, 0, loc + 1);
	}
	else
	{
		path += m_SkinName;
		path += L'\\';
	}

	return path;
}

CMeter* CMeterWindow::GetMeter(const std::wstring& meterName)
{
	const WCHAR* name = meterName.c_str();
	std::list<CMeter*>::const_iterator j = m_Meters.begin();
	for ( ; j != m_Meters.end(); ++j)
	{
		if (_wcsicmp((*j)->GetName(), name) == 0)
		{
			return (*j);
		}
	}
	return NULL;
}

CMeasure* CMeterWindow::GetMeasure(const std::wstring& measureName)
{
	const WCHAR* name = measureName.c_str();
	std::list<CMeasure*>::const_iterator i = m_Measures.begin();
	for ( ; i != m_Measures.end(); ++i)
	{
		if (_wcsicmp((*i)->GetName(), name) == 0)
		{
			return (*i);
		}
	}
	return NULL;
}