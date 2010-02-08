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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "StdAfx.h"
#include <windows.h>
#include <gdiplus.h>
#include "MeterWindow.h"
#include "Rainmeter.h"
#include "Error.h"
#include <math.h>
#include <time.h>
#include "Meter.h"
#include "Measure.h"
#include "AboutDialog.h"
#include "resource.h"
#include "Litestep.h"
#include <Mmsystem.h>
#include <assert.h>
#include <tchar.h>
#include "MeasureCalc.h"
#include "MeasureNet.h"
#include "MeasurePlugin.h"
#include "MeterButton.h"

using namespace Gdiplus;

#define ULW_ALPHA               0x00000002
#define WS_EX_LAYERED           0x00080000

#define METERTIMER 1
#define MOUSETIMER 2
#define FADETIMER 3
#define TRANSITIONTIMER 4

#define SNAPDISTANCE 10

int CMeterWindow::c_InstanceCount = 0;

MULTIMONITOR_INFO CMeterWindow::c_Monitors = { 0 };

extern CRainmeter* Rainmeter;

/* 
** CMeterWindow
**
** Constructor
**
*/
CMeterWindow::CMeterWindow(std::wstring& path, std::wstring& config, std::wstring& iniFile)
{
	m_Background = NULL;
	m_Window = NULL;
	m_ChildWindow = false;

	m_DoubleBuffer = NULL;

	m_WindowX = L"0";
	m_WindowY = L"0";
	m_ScreenX = 0;
	m_ScreenY = 0;
	m_WindowW = 0;
	m_WindowH = 0;
	m_WindowXPercentage = false;
	m_WindowYPercentage = false;
	m_WindowXFromRight = false;
	m_WindowYFromBottom = false;
	m_WindowXScreen = 1;
	m_WindowYScreen = 1;
	m_WindowXScreenDefined = false;
	m_WindowYScreenDefined = false;
	m_AnchorXFromRight = false;
	m_AnchorYFromBottom = false;
	m_AnchorXPercentage = false;
	m_AnchorYPercentage = false;
	m_AnchorScreenX = 0; 
	m_AnchorScreenY = 0;
	m_WindowZPosition = ZPOSITION_NORMAL;
	m_WindowDraggable = true;
	m_WindowUpdate = 1000;
	m_TransitionUpdate = 100;
	m_ActiveTransition = false;
	m_WindowHide = HIDEMODE_NONE;
	m_WindowStartHidden = false;
	m_SnapEdges = true;
	m_Rainmeter = NULL;
	m_ResetRegion = false;
	m_Refreshing = false;
	m_NativeTransparency = true;
	m_MeasuresToVariables = false;
	m_SavePosition = false;			// Must be false
	m_AlphaValue = 255;
	m_FadeDuration = 250;
	m_ClickThrough = false;
	m_DynamicWindowSize = false;
	m_KeepOnScreen = true;
	m_AutoSelectScreen = false;
	m_Dragging = false;
	m_Dragged = false;
	m_PreventMoving = false;

	m_BackgroundSize.cx = 0;
	m_BackgroundSize.cy = 0;

	m_FadeStartTime = 0;
	m_FadeStartValue = 0;
	m_FadeEndValue = 0;
	m_TransparencyValue = 0;

	m_MouseOver = false;

	m_BackgroundMode = BGMODE_IMAGE;
	m_SolidBevel = BEVELTYPE_NONE;

	m_SkinPath = path;
	m_SkinName = config;
	m_SkinIniFile = iniFile;

	m_User32Library = LoadLibrary(L"user32.dll");

	m_UpdateCounter = 0;
	m_FontCollection = NULL;

	c_InstanceCount++;
}

/* 
** ~CMeterWindow
**
** Destructor
**
*/
CMeterWindow::~CMeterWindow()
{
	WriteConfig();

	// Destroy the meters
	std::list<CMeter*>::iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); j++)
	{
		delete (*j);
	}

	// Destroy the measures
	std::list<CMeasure*>::iterator i = m_Measures.begin();
	for( ; i != m_Measures.end(); i++)
	{
		delete (*i);
	}

	if(m_Background) delete m_Background;
	if(m_DoubleBuffer) delete (m_DoubleBuffer);

	if(m_Window) DestroyWindow(m_Window);

	if(m_FontCollection) delete m_FontCollection;

	FreeLibrary(m_User32Library);

	c_InstanceCount--;

	if (c_InstanceCount == 0)
	{
		BOOL Result;
		int counter = 0;
		do {
			// Wait for the window to die
			Result = UnregisterClass(L"RainmeterMeterWindow", m_Rainmeter->GetInstance());
			Sleep(100);
			counter += 1;
		} while(!Result && counter < 10);
	}
}

/* 
** Initialize
**
** Initializes the window, creates the class and the window.
**
*/
int CMeterWindow::Initialize(CRainmeter& Rainmeter)
{
	WNDCLASSEX wc;

	m_Rainmeter = &Rainmeter;

	// Register the windowclass
	memset(&wc, 0, sizeof(WNDCLASSEX));
	wc.style = CS_NOCLOSE | CS_DBLCLKS;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = m_Rainmeter->GetInstance();
	wc.lpszClassName = L"RainmeterMeterWindow";
	
	if(!RegisterClassEx(&wc))
	{
		DWORD err = GetLastError();

		if (err != 0 && ERROR_CLASS_ALREADY_EXISTS != err)
		{
			throw CError(CError::ERROR_REGISTER_WINDOWCLASS, __LINE__, __FILE__);
		}
	}

	m_Window = CreateWindowEx(WS_EX_TOOLWINDOW,
							L"RainmeterMeterWindow", 
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

	if(m_Window == NULL) 
	{ 
		throw CError(CError::ERROR_CREATE_WINDOW, __LINE__, __FILE__);
	}

#ifndef _WIN64
	SetWindowLong(m_Window, GWL_USERDATA, magicDWord);
#endif

	setlocale(LC_NUMERIC, "C");

	// Mark the window to ignore the Aero peek
	typedef HRESULT (WINAPI * FPDWMSETWINDOWATTRIBUTE)(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
	#define DWMWA_EXCLUDED_FROM_PEEK 12
	HINSTANCE h = LoadLibrary(L"dwmapi.dll");
	if (h)
	{
		FPDWMSETWINDOWATTRIBUTE DwmSetWindowAttribute = (FPDWMSETWINDOWATTRIBUTE)GetProcAddress(h, "DwmSetWindowAttribute");
		BOOL bValue = TRUE;
		if (DwmSetWindowAttribute)
		{
			DwmSetWindowAttribute(m_Window, DWMWA_EXCLUDED_FROM_PEEK, &bValue, sizeof(bValue));
		}
		FreeLibrary(h);
	}

	// Gotta have some kind of buffer during initialization
	m_DoubleBuffer = new Bitmap(1, 1, PixelFormat32bppARGB);

	Refresh(true);
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

	LSLog(LOG_DEBUG, L"Rainmeter", L"Initialization successful.");

	return 0;
}

/*
** Refresh
**
** This deletes everything and rebuilds the config again.
**
*/
void CMeterWindow::Refresh(bool init)
{
	assert(m_Rainmeter != NULL);

	m_Rainmeter->SetCurrentParser(&m_Parser);

	std::wstring dbg = L"Refreshing (Name: \"" + m_SkinName;
	dbg += L"\" Ini: \"" + m_SkinIniFile; 
	dbg += L"\")"; 
	LSLog(LOG_DEBUG, L"Rainmeter", dbg.c_str());
	
	m_Refreshing = true;

	if(!init)
	{
		// First destroy everything
		// WriteConfig(); //Not clear why this is needed and it messes up resolution changes

		KillTimer(m_Window, METERTIMER);	// Kill the timer
		KillTimer(m_Window, MOUSETIMER);	// Kill the timer
		KillTimer(m_Window, FADETIMER);	// Kill the timer

		if (!m_ChildWindow && GetAncestor(m_Window, GA_PARENT) != GetDesktopWindow())
		{
			// Reset to the top-level window
			m_PreventMoving = true;  // Prevent moving the window by SetParent
			SetParent(m_Window, NULL);
		}

		std::list<CMeasure*>::iterator i = m_Measures.begin();
		for( ; i != m_Measures.end(); i++)
		{
			delete (*i);
		}
		m_Measures.clear();

		std::list<CMeter*>::iterator j = m_Meters.begin();
		for( ; j != m_Meters.end(); j++)
		{
			delete (*j);
		}
		m_Meters.clear();

		if(m_Background) delete m_Background;
		m_Background = NULL;

		m_BackgroundSize.cx = m_BackgroundSize.cy = 0;

		m_BackgroundName.erase();

		if (m_FontCollection) delete m_FontCollection;
		m_FontCollection = NULL;
	}

	//TODO: Should these be moved to a Reload command instead of hitting the disk on every refresh
	ReadConfig();	// Read the general settings 
	ReadSkin();

	InitializeMeters();

	// Remove transparent flag
	LONG style = GetWindowLong(m_Window, GWL_EXSTYLE);
	if ((style & WS_EX_TRANSPARENT) != 0)
	{
		SetWindowLong(m_Window, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
	}

	// Set the window region
	CreateRegion(true);	// Clear the region
	Update(false);
	CreateRegion(false);

	if (m_KeepOnScreen) 
	{
		MapCoordsToScreen(m_ScreenX, m_ScreenY, m_WindowW, m_WindowH);
	}

	ZPOSITION zPos = m_WindowZPosition;
	m_WindowZPosition = ZPOSITION_NORMAL;  // Handles as the normal window temporarily
	SetWindowPos(m_Window, NULL, m_ScreenX, m_ScreenY, m_WindowW, m_WindowH, SWP_NOZORDER | SWP_NOACTIVATE);
	m_WindowZPosition = zPos;

	ScreenToWindow();

	// Start the timers
	if(0 == SetTimer(m_Window, METERTIMER, m_WindowUpdate, NULL))
	{
		throw CError(L"Unable to create a timer!", __LINE__, __FILE__);
	}

	if(0 == SetTimer(m_Window, MOUSETIMER, 500, NULL))	// Mouse position is checked twice per sec
	{
		throw CError(L"Unable to create a timer!", __LINE__, __FILE__);
	}

	UpdateTransparency(m_AlphaValue, true);

	ChangeZPos(m_WindowZPosition);

	m_Rainmeter->SetCurrentParser(NULL);

	m_Refreshing = false;

	if (!m_OnRefreshAction.empty())
	{
		m_Rainmeter->ExecuteCommand(m_OnRefreshAction.c_str(), this);
	}
}

void CMeterWindow::MapCoordsToScreen(int& x, int& y, int w, int h)
{
	// Check that the window is inside the screen area
	HMONITOR hMonitor;
	MONITORINFO mi;

	POINT pt = {x, y};
	for (int i = 0; i < 5; i++) 
	{
		switch(i) 
		{
		case 0:
			pt.x = x + w / 2;
			pt.y = y + h / 2;
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

		hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);

		if(hMonitor != NULL)
		{
			mi.cbSize = sizeof(mi);
			GetMonitorInfo(hMonitor, &mi);

			x = max(x, mi.rcMonitor.left);
			x = min(x, mi.rcMonitor.right - m_WindowW);
			y = max(y, mi.rcMonitor.top);
			y = min(y, mi.rcMonitor.bottom - m_WindowH);
			return;
		}
	}

	// No monitor found for the window -> Use the default work area
	RECT workArea;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);	// Store the old value
	x = max(x, workArea.left);
	x = min(x, workArea.right - m_WindowW);
	y = max(y, workArea.top);
	y = min(y, workArea.bottom - m_WindowH);
}

/*
** MoveWindow
**
** Moves the window to a new place (on the virtual screen)
**
*/
void CMeterWindow::MoveWindow(int x, int y)
{
	// Convert the screen coordinates to the client coordinates of the shell window
	if (!m_ChildWindow && !m_NativeTransparency && m_WindowZPosition == ZPOSITION_ONDESKTOP)
	{
		POINT pos = {x, y};
		if (ScreenToClient(GetAncestor(m_Window, GA_PARENT), &pos))
		{
			x = pos.x;
			y = pos.y;
		}
	}

	SetWindowPos(m_Window, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

	ScreenToWindow();

	if (m_SavePosition)
	{
		WriteConfig();
	}
}

/*
** ChangeZPos
**
** Sets the window's z-position
**
*/
void CMeterWindow::ChangeZPos(ZPOSITION zPos)
{
	if(!m_ChildWindow)
	{
		HWND parent = GetAncestor(m_Window, GA_PARENT);
		HWND winPos = HWND_NOTOPMOST;
		m_WindowZPosition = zPos;

		switch (zPos)
		{
		case ZPOSITION_ONTOPMOST:
		case ZPOSITION_ONTOP:
			winPos = HWND_TOPMOST;
 			break;

		case ZPOSITION_ONBOTTOM:
			 winPos = HWND_BOTTOM;
			 break;

		case ZPOSITION_ONDESKTOP:
			// Set the window's parent to progman, so it stays always on desktop
			HWND ProgmanHwnd = FindWindowEx(FindWindowEx(FindWindow(L"Progman", L"Program Manager"), NULL, L"SHELLDLL_DefView", L""), NULL, L"SysListView32", L"FolderView");
			if (!ProgmanHwnd)
			{
				HWND WorkerWHwnd = NULL;
				while ((WorkerWHwnd = FindWindowEx(NULL, WorkerWHwnd, L"WorkerW", L"")) != NULL)
				{
					ProgmanHwnd = FindWindowEx(FindWindowEx(WorkerWHwnd, NULL, L"SHELLDLL_DefView", L""), NULL, L"SysListView32", L"FolderView");
					if (ProgmanHwnd) break;
				}
			}
			if (ProgmanHwnd && (parent != ProgmanHwnd))
			{
				m_PreventMoving = true;  // Prevent moving the window by SetParent
				SetParent(m_Window, ProgmanHwnd);
			}
			else
			{
				return;		// The window is already on desktop
			}
			break;
		}

		if (zPos != ZPOSITION_ONDESKTOP && (parent != GetDesktopWindow()))
		{
			m_PreventMoving = true;  // Prevent moving the window by SetParent
			SetParent(m_Window, NULL);
		}

		bool refresh = m_Refreshing;
		m_Refreshing = true;	// Fake refreshing so that the z-position can be changed
		SetWindowPos(m_Window, winPos, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		m_Refreshing = refresh;
	}
}

/*
** RunBang
**
** Runs the bang command
**
*/
void CMeterWindow::RunBang(BANGCOMMAND bang, const WCHAR* arg)
{
	const WCHAR* pos = NULL;
	const WCHAR* pos2 = NULL;

	if (!m_Window) return;

	switch(bang) 
	{
	case BANG_REFRESH:
		// Refresh needs to be delayed since it crashes if done during Update()
		PostMessage(m_Window, WM_DELAYED_REFRESH, (WPARAM)NULL, (LPARAM)NULL);
		break;

	case BANG_REDRAW:
		Redraw();
		break;

	case BANG_TOGGLEMETER:
		ToggleMeter(arg);
		break;

	case BANG_SHOWMETER:
		ShowMeter(arg);
		break;

	case BANG_HIDEMETER:
		HideMeter(arg);
		break;

	case BANG_TOGGLEMEASURE:
		ToggleMeasure(arg);
		break;

	case BANG_ENABLEMEASURE:
		EnableMeasure(arg);
		break;

	case BANG_DISABLEMEASURE:
		DisableMeasure(arg);
		break;

	case BANG_SHOW:
		ShowWindow(m_Window, SW_SHOWNOACTIVATE);
		break;

	case BANG_HIDE:
		ShowWindow(m_Window, SW_HIDE);
		break;

	case BANG_TOGGLE:
		if (IsWindowVisible(m_Window))
		{
			ShowWindow(m_Window, SW_HIDE);
		}
		else
		{
			ShowWindow(m_Window, SW_SHOWNOACTIVATE);
		}
		break;

	case BANG_MOVE:
		pos = wcschr(arg, ' ');
		if (pos != NULL)
		{
			MoveWindow(_wtoi(arg), _wtoi(pos));
		}
		else
		{
			DebugLog(L"Cannot parse parameters for !RainmeterMove");
		}
		break;

	case BANG_ZPOS:
		ChangeZPos((ZPOSITION)_wtoi(arg));
		break;

	case BANG_LSHOOK:
		{
			pos = wcsrchr(arg, ' ');
			if (pos != NULL)
			{
#ifdef _WIN64
				HWND hWnd = (HWND)_wtoi64(pos);
#else
				HWND hWnd = (HWND)_wtoi(pos);
#endif
				if (hWnd) 
				{
					// Disable native transparency
					m_NativeTransparency = false;
					UpdateTransparency(m_AlphaValue, true);
					SetWindowLong(m_Window, GWL_STYLE, (GetWindowLong(m_Window, GWL_STYLE) &~ WS_POPUP) | WS_CHILD);
					SetParent(m_Window, hWnd);
					m_ChildWindow = true;
				}
			}
			else
			{
				DebugLog(L"Cannot parse parameters for !RainmeterLsBoxHook (%s)", arg);
			}
		}
		break;

	case BANG_ABOUT:
		OpenAboutDialog(m_Window, m_Rainmeter->GetInstance());
		break;

	case BANG_MOVEMETER:
		pos = wcschr(arg, ' ');
		if (pos != NULL)
		{
			pos2 = wcschr(pos + 1, ' ');
			if (pos2 != NULL)
			{
				MoveMeter(_wtoi(arg), _wtoi(pos), pos2 + 1);
			}
			else
			{
				DebugLog(L"Cannot parse the for !RainmeterMoveMeter");
			}
		}
		else
		{
			DebugLog(L"Cannot parse the coordinates for !RainmeterMoveMeter");
		}
		break;

	case BANG_PLUGIN:
		{
			std::wstring args = arg;
			std::wstring measure = arg;
			size_t pos = std::wstring::npos;
			do 
			{
				pos = args.find('\"');
				if (pos != std::wstring::npos)
				{
					args.erase(pos, 1);
				}
				
			} while(pos != std::wstring::npos);

			pos = args.find(' ');
			if (pos != std::wstring::npos)
			{
				measure = args.substr(0, pos);
				args.erase(0, pos + 1);
			}

			if (!measure.empty())
			{
				std::list<CMeasure*>::iterator iter = m_Measures.begin();
				for( ; iter != m_Measures.end(); iter++)
				{
					if (wcsicmp((*iter)->GetName(), measure.c_str()) == 0)
					{
						(*iter)->ExecuteBang(args.c_str());
						return;
					}
				}

				DebugLog(L"Unable to find [%s] for !RainmeterPluginBang", measure.c_str());
			}
			else
			{
				DebugLog(L"Cannot parse the !RainmeterPluginBang");
			}
		}
		break;

	case BANG_QUIT:
		// Quit needs to be delayed since it crashes if done during Update()
		PostMessage(m_Window, WM_DELAYED_QUIT, (WPARAM)NULL, (LPARAM)NULL);
		break;

	case BANG_SETVARIABLE:
		pos = wcschr(arg, ' ');
		if (pos != NULL)
		{
			std::wstring strVariable(arg, pos - arg);
			std::wstring strValue(pos + 1);
			m_Parser.SetVariable(strVariable, strValue);
		}
		else
		{
			DebugLog(L"Cannot parse parameters for !RainmeterSetVariable");
		}
		break;
	}
}

/*
** ShowMeter
**
** Shows the given meter
**
*/
void CMeterWindow::ShowMeter(const WCHAR* name)
{
	if (name == NULL || wcslen(name) == 0) return;

	std::list<CMeter*>::iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); j++)
	{
		if (wcsicmp((*j)->GetName(), name) == 0)
		{
			(*j)->Show();
			m_ResetRegion = true;	// Need to recalculate the window region
			return;
		}
	}

	DebugLog(L"Unable to show the meter %s (there is no such thing in %s)", name, m_SkinName.c_str());
}

/*
** HideMeter
**
** Hides the given meter
**
*/
void CMeterWindow::HideMeter(const WCHAR* name)
{
	if (name == NULL || wcslen(name) == 0) return;

	std::list<CMeter*>::iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); j++)
	{
		if (wcsicmp((*j)->GetName(), name) == 0)
		{
			(*j)->Hide();
			m_ResetRegion = true;	// Need to recalculate the windowregion
			return;
		}
	}

	DebugLog(L"Unable to hide the meter %s (there is no such thing in %s)", name, m_SkinName.c_str());
}

/*
** ToggleMeter
**
** Toggles the given meter
**
*/
void CMeterWindow::ToggleMeter(const WCHAR* name)
{
	if (name == NULL || wcslen(name) == 0) return;

	std::list<CMeter*>::iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); j++)
	{
		if (wcsicmp((*j)->GetName(), name) == 0)
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
			return;
		}
	}

	DebugLog(L"Unable to toggle the meter %s (there is no such thing in %s)", name, m_SkinName.c_str());
}

/*
** MoveMeter
**
** Moves the given meter
**
*/
void CMeterWindow::MoveMeter(int x, int y, const WCHAR* name)
{
	if (name == NULL || wcslen(name) == 0) return;

	std::list<CMeter*>::iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); j++)
	{
		if (wcsicmp((*j)->GetName(), name) == 0)
		{
			(*j)->SetX(x);
				(*j)->SetY(y);
			m_ResetRegion = true;
			return;
		}
	}

	DebugLog(L"Unable to move the meter %s (there is no such thing in %s)", name, m_SkinName.c_str());
}
/*
** EnableMeasure
**
** Enables the given measure
**
*/
void CMeterWindow::EnableMeasure(const WCHAR* name)
{
	if (name == NULL || wcslen(name) == 0) return;

	std::list<CMeasure*>::iterator i = m_Measures.begin();
	for( ; i != m_Measures.end(); i++)
	{
		if (wcsicmp((*i)->GetName(), name) == 0)
		{
			(*i)->Enable();
			return;
		}
	}

	DebugLog(L"Unable to enable the measure %s (there is no such thing in %s)", name, m_SkinName.c_str());
}


/*
** DisableMeasure
**
** Disables the given measure
**
*/
void CMeterWindow::DisableMeasure(const WCHAR* name)
{
	if (name == NULL || wcslen(name) == 0) return;

	std::list<CMeasure*>::iterator i = m_Measures.begin();
	for( ; i != m_Measures.end(); i++)
	{
		if (wcsicmp((*i)->GetName(), name) == 0)
		{
			(*i)->Disable();
			return;
		}
	}

	DebugLog(L"Unable to disable the measure %s (there is no such thing in %s)", name, m_SkinName.c_str());
}


/*
** ToggleMeasure
**
** Toggless the given measure
**
*/
void CMeterWindow::ToggleMeasure(const WCHAR* name)
{
	if (name == NULL || wcslen(name) == 0) return;

	std::list<CMeasure*>::iterator i = m_Measures.begin();
	for( ; i != m_Measures.end(); i++)
	{
		if (wcsicmp((*i)->GetName(), name) == 0)
		{
			if ((*i)->IsDisabled())
			{
				(*i)->Enable();
			}
			else
			{
				(*i)->Disable();
			}
			return;
		}
	}

	DebugLog(L"Unable to toggle the measure %s (there is no such thing in %s)", name, m_SkinName.c_str());
}

/* MyInfoEnumProc
**
** Retrieves the multi-monitor information
**
*/
BOOL CALLBACK MyInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	MULTIMONITOR_INFO* m = (MULTIMONITOR_INFO*)dwData;

	MONITORINFOEX info;
	info.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(hMonitor, &info);

	// for debug
	DebugLog(info.szDevice);
	DebugLog(L"  Flags    : %s(0x%08X)", (info.dwFlags & MONITORINFOF_PRIMARY) ? L"PRIMARY " : L"", info.dwFlags);
	DebugLog(L"  Handle   : 0x%08X", hMonitor);
	DebugLog(L"  ScrArea  : L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
		lprcMonitor->left, lprcMonitor->top, lprcMonitor->right, lprcMonitor->bottom,
		lprcMonitor->right - lprcMonitor->left, lprcMonitor->bottom - lprcMonitor->top);
	DebugLog(L"  WorkArea : L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
		info.rcWork.left, info.rcWork.top, info.rcWork.right, info.rcWork.bottom,
		info.rcWork.right - info.rcWork.left, info.rcWork.bottom - info.rcWork.top);
	if (m == NULL) return TRUE;

	if (m->useEnumDisplayDevices)
	{
		for (size_t i = 0; i < m->monitors.size(); i++)
		{
			if (m->monitors[i].handle == NULL && _wcsnicmp(info.szDevice, m->monitors[i].deviceName, 32) == 0)
			{
				m->monitors[i].handle = hMonitor;
				m->monitors[i].screen = *lprcMonitor;
				m->monitors[i].work = info.rcWork;
				break;
			}
		}
	}
	else  // use only EnumDisplayMonitors
	{
		MONITOR_INFO monitor = {0};
		monitor.active = true;

		monitor.handle = hMonitor;
		monitor.screen = *lprcMonitor;
		monitor.work = info.rcWork;

		wcsncpy(monitor.deviceName, info.szDevice, 32);  // E.g. "\\.\DISPLAY1"

		// Get the monitor name (E.g. "Generic Non-PnP Monitor")
		DISPLAY_DEVICE ddm = {0};
		ddm.cb = sizeof(DISPLAY_DEVICE);
		DWORD dwMon = 0;
		while (EnumDisplayDevices(info.szDevice, dwMon++, &ddm, 0))
		{
			if (ddm.StateFlags & DISPLAY_DEVICE_ACTIVE && ddm.StateFlags & DISPLAY_DEVICE_ATTACHED)
			{
				wcsncpy(monitor.monitorName, ddm.DeviceString, 128);
				break;
			}
		}

		m->monitors.push_back(monitor);

		if (info.dwFlags & MONITORINFOF_PRIMARY)
		{
			// It's primary monitor!
			m->primary = (int)m->monitors.size();
		}
	}

	return TRUE;
}

/* GetMonitorCount
**
** Returns the number of monitors
**
*/
size_t CMeterWindow::GetMonitorCount()
{
	if (c_Monitors.monitors.size() == 0)
	{
		SetMultiMonitorInfo();
	}
	return c_Monitors.monitors.size();
}

/* SetMultiMonitorInfo
**
** Sets the multi-monitor information
**
*/
void CMeterWindow::SetMultiMonitorInfo()
{
	std::vector<MONITOR_INFO>& monitors = c_Monitors.monitors;

	if (monitors.capacity() < 16) { monitors.reserve(16); }

	c_Monitors.vsT = GetSystemMetrics(SM_YVIRTUALSCREEN);
	c_Monitors.vsL = GetSystemMetrics(SM_XVIRTUALSCREEN);
	c_Monitors.vsH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	c_Monitors.vsW = GetSystemMetrics(SM_CXVIRTUALSCREEN);

	c_Monitors.primary = 1;  // If primary screen is not found, 1st screen is assumed as primary screen.

	c_Monitors.useEnumDisplayDevices = true;
	c_Monitors.useEnumDisplayMonitors = false;

	DebugLog(L"------------------------------");
	DebugLog(L"* EnumDisplayDevices / EnumDisplaySettings API");

	DISPLAY_DEVICE dd = {0};
	dd.cb = sizeof(DISPLAY_DEVICE);

	if (EnumDisplayDevices(NULL, 0, &dd, 0))
	{
		DWORD dwDevice = 0;

		do
		{
			DebugLog(dd.DeviceName);

			std::wstring msg;
			if (dd.StateFlags & DISPLAY_DEVICE_ACTIVE)
			{
				msg += L"ACTIVE ";
			}
			if (dd.StateFlags & DISPLAY_DEVICE_MULTI_DRIVER)
			{
				msg += L"MULTI ";
			}
			if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
			{
				msg += L"PRIMARY ";
			}
			if (dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
			{
				msg += L"MIRROR ";
			}
			if (dd.StateFlags & DISPLAY_DEVICE_VGA_COMPATIBLE)
			{
				msg += L"VGA ";
			}
			if (dd.StateFlags & DISPLAY_DEVICE_REMOVABLE)
			{
				msg += L"REMOVABLE ";
			}
			if (dd.StateFlags & DISPLAY_DEVICE_MODESPRUNED)
			{
				msg += L"PRUNED ";
			}
			if (dd.StateFlags & DISPLAY_DEVICE_REMOTE)
			{
				msg += L"REMOTE ";
			}
			if (dd.StateFlags & DISPLAY_DEVICE_DISCONNECT)
			{
				msg += L"DISCONNECT ";
			}

			if ((dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) == 0)
			{
				MONITOR_INFO monitor = {0};

				monitor.handle = NULL;
				wcsncpy(monitor.deviceName, dd.DeviceName, 32);  // E.g. "\\.\DISPLAY1"

				// Get the monitor name (E.g. "Generic Non-PnP Monitor")
				DISPLAY_DEVICE ddm = {0};
				ddm.cb = sizeof(DISPLAY_DEVICE);
				DWORD dwMon = 0;
				while (EnumDisplayDevices(dd.DeviceName, dwMon++, &ddm, 0))
				{
					if (ddm.StateFlags & DISPLAY_DEVICE_ACTIVE && ddm.StateFlags & DISPLAY_DEVICE_ATTACHED)
					{
						wcsncpy(monitor.monitorName, ddm.DeviceString, 128);
						DebugLog(L"  Name     : %s", ddm.DeviceString);
						break;
					}
				}
				DebugLog(L"  Adapter  : %s", dd.DeviceString);
				DebugLog(L"  Flags    : %s(0x%08X)", msg.c_str(), dd.StateFlags);

				if (dd.StateFlags & DISPLAY_DEVICE_ACTIVE)
				{
					monitor.active = true;

					DEVMODE dm = {0};
					dm.dmSize = sizeof(DEVMODE);

					if (EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm))
					{
						POINT pos = {dm.dmPosition.x, dm.dmPosition.y};
						monitor.handle = MonitorFromPoint(pos, MONITOR_DEFAULTTONULL);
						DebugLog(L"  Handle   : 0x%08X", monitor.handle);
					}

					if (monitor.handle != NULL)
					{
						MONITORINFO info = {0};
						info.cbSize = sizeof(MONITORINFO);
						GetMonitorInfo(monitor.handle, &info);

						monitor.screen = info.rcMonitor;
						monitor.work = info.rcWork;

						DebugLog(L"  ScrArea  : L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
							info.rcMonitor.left, info.rcMonitor.top, info.rcMonitor.right, info.rcMonitor.bottom,
							info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top);
						DebugLog(L"  WorkArea : L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
							info.rcWork.left, info.rcWork.top, info.rcWork.right, info.rcWork.bottom,
							info.rcWork.right - info.rcWork.left, info.rcWork.bottom - info.rcWork.top);
					}
					else  // monitor not found
					{
						c_Monitors.useEnumDisplayMonitors = true;
					}
				}
				else
				{
					monitor.active = false;
				}

				monitors.push_back(monitor);

				if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
				{
					// It's primary monitor!
					c_Monitors.primary = (int)monitors.size();
				}
			}
			else
			{
				DebugLog(L"  Adapter  : %s", dd.DeviceString);
				DebugLog(L"  Flags    : %s(0x%08X)", msg.c_str(), dd.StateFlags);
			}
			dwDevice++;
		} while (EnumDisplayDevices(NULL, dwDevice, &dd, 0));
	}

	if (monitors.empty())  // Failed to enumerate the non-mirroring monitors
	{
		DebugLog(L"Failed to enumerate the non-mirroring monitors. Only EnumDisplayMonitors is used instead.");
		c_Monitors.useEnumDisplayDevices = false;
		c_Monitors.useEnumDisplayMonitors = true;
	}

	DebugLog(L"------------------------------");
	DebugLog(L"* EnumDisplayMonitors API");

	if (c_Monitors.useEnumDisplayMonitors)
	{
		EnumDisplayMonitors(NULL, NULL, MyInfoEnumProc, (LPARAM)(&c_Monitors));

		if (monitors.empty())  // Failed to enumerate the monitors
		{
			DebugLog(L"Failed to enumerate the monitors. Prepares the dummy monitor information.");
			c_Monitors.useEnumDisplayMonitors = false;

			MONITOR_INFO monitor = {0};
			wcscpy(monitor.deviceName, L"DUMMY");
			POINT pos = {0, 0};
			monitor.handle = MonitorFromPoint(pos, MONITOR_DEFAULTTOPRIMARY);
			monitor.screen.left = 0;
			monitor.screen.top = 0;
			monitor.screen.right = GetSystemMetrics(SM_CXSCREEN);
			monitor.screen.bottom = GetSystemMetrics(SM_CYSCREEN);
			SystemParametersInfo(SPI_GETWORKAREA, 0, &(monitor.work), 0);
			monitor.active = true;

			monitors.push_back(monitor);

			c_Monitors.primary = 1;
		}
	}
	else
	{
		EnumDisplayMonitors(NULL, NULL, MyInfoEnumProc, (LPARAM)NULL);  // Only logging
	}

	DebugLog(L"------------------------------");
	std::wstring method = L"* METHOD: ";
	if (c_Monitors.useEnumDisplayDevices)
	{
		method += L"EnumDisplayDevices + ";
		method += c_Monitors.useEnumDisplayMonitors ? L"EnumDisplayMonitors Mode" : L"EnumDisplaySettings Mode";
	}
	else
	{
		method += c_Monitors.useEnumDisplayMonitors ? L"EnumDisplayMonitors Mode" : L"Dummy Mode";
	}
	DebugLog(method.c_str());

	DebugLog(L"* MONITORS: Count=%i, Primary=@%i", monitors.size(), c_Monitors.primary);
	DebugLog(L"@0: Virtual screen");
	DebugLog(L"  L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
		c_Monitors.vsL, c_Monitors.vsT, c_Monitors.vsL + c_Monitors.vsW, c_Monitors.vsT + c_Monitors.vsH,
		c_Monitors.vsW, c_Monitors.vsH);

	for (size_t i = 0; i < monitors.size(); i++)
	{
		if (monitors[i].active)
		{
			DebugLog(L"@%i: %s (active), MonitorName: %s", i + 1, monitors[i].deviceName, monitors[i].monitorName);
			DebugLog(L"  L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
				monitors[i].screen.left, monitors[i].screen.top, monitors[i].screen.right, monitors[i].screen.bottom,
				monitors[i].screen.right - monitors[i].screen.left, monitors[i].screen.bottom - monitors[i].screen.top);
		}
		else
		{
			DebugLog(L"@%i: %s (inactive), MonitorName: %s", i + 1, monitors[i].deviceName, monitors[i].monitorName);
		}
	}
	DebugLog(L"------------------------------");
}

/* UpdateWorkareaInfo
**
** Updates the workarea information
**
*/
void CMeterWindow::UpdateWorkareaInfo()
{
	std::vector<MONITOR_INFO>& monitors = c_Monitors.monitors;

	if (monitors.empty())
	{
		SetMultiMonitorInfo();
		return;
	}

	for (size_t i = 0; i < monitors.size(); i++)
	{
		if (monitors[i].active && monitors[i].handle != NULL)
		{
			MONITORINFO info = {0};
			info.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo(monitors[i].handle, &info);

			monitors[i].work = info.rcWork;

			DebugLog(L"WorkArea @%i : L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
				i + 1,
				info.rcWork.left, info.rcWork.top, info.rcWork.right, info.rcWork.bottom,
				info.rcWork.right - info.rcWork.left, info.rcWork.bottom - info.rcWork.top);
		}
	}
}

/* WindowToScreen
**
** Calculates the screen cordinates from the WindowX/Y config
**
*/
void CMeterWindow::WindowToScreen()
{
	std::wstring::size_type index, index2;
	int pixel = 0;
	float num;
	int screenx, screeny, screenh, screenw;

	std::vector<MONITOR_INFO>& monitors = c_Monitors.monitors;

	if (monitors.empty())
	{
		// Retrieve the multi-monitor information
		SetMultiMonitorInfo();

		if (monitors.empty())
		{
			DebugLog(L"There are no monitors. WindowToScreen function fails.");
			return;
		}
	}

	// Clear position flags
	m_WindowXScreen = m_WindowYScreen = c_Monitors.primary; // Default to primary screen
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
		std::wstring screenStr;
		if (index2 != std::wstring::npos)
		{
			screenStr = m_WindowX.substr(index, index2 - index);
		}
		else
		{
			screenStr = m_WindowX.substr(index);
		}
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
		screenx = c_Monitors.vsL;
		screenw = c_Monitors.vsW;
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
		std::wstring screenStr;
		if (index2 != std::wstring::npos)
		{
			screenStr = m_WindowY.substr(index, index2 - index);
		}
		else
		{
			screenStr = m_WindowY.substr(index);
		}
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
		screeny = c_Monitors.vsT;
		screenh = c_Monitors.vsH;
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

	std::vector<MONITOR_INFO>& monitors = c_Monitors.monitors;

	if (monitors.empty())
	{
		DebugLog(L"There are no monitors. ScreenToWindow function fails.");
		return;
	}

	// Correct to auto-selected screen
	if (m_AutoSelectScreen)
	{
		RECT rect = {m_ScreenX, m_ScreenY, m_ScreenX + m_WindowW, m_ScreenY + m_WindowH};
		HMONITOR hMonitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);

		if (hMonitor != NULL)
		{
			for (size_t i = 0; i < monitors.size(); i++)
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
						m_Parser.ResetVariables(m_Rainmeter, this);  // Set present monitor variables
					}
					break;
				}
			}
		}
	}

	// --- Calculate WindowX ---

	if (m_WindowXScreen == 0)
	{
		screenx = c_Monitors.vsL;
		screenw = c_Monitors.vsW;
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
		swprintf(buffer, L"%.5f%%", num);
	}
	else
	{
		wsprintf(buffer, L"%i", pixel);
	}
	if (m_WindowXFromRight == true)
	{
		wsprintf(buffer, L"%sR", buffer);
	}
	if (m_WindowXScreenDefined == true)
	{
		wsprintf(buffer, L"%s@%i", buffer, m_WindowXScreen);
	}
	m_WindowX = buffer;

	// --- Calculate WindowY ---

	if (m_WindowYScreen == 0)
	{
		screeny = c_Monitors.vsT;
		screenh = c_Monitors.vsH;
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
		swprintf(buffer, L"%.5f%%", num);
	}
	else
	{
		wsprintf(buffer, L"%i", pixel);
	}
	if (m_WindowYFromBottom == true)
	{
		wsprintf(buffer, L"%sB", buffer);
	}
	if (m_WindowYScreenDefined == true)
	{
		wsprintf(buffer, L"%s@%i", buffer, m_WindowYScreen);
	}
	m_WindowY = buffer;
}

/*
** ReadConfig
**
** Reads the current config
**
*/
void CMeterWindow::ReadConfig()
{
	std::wstring iniFile = m_Rainmeter->GetIniFile();
	const WCHAR* section = L"Rainmeter";

	m_SavePosition = true;		// Default value

	CConfigParser parser;
	parser.Initialize(iniFile.c_str(), m_Rainmeter);

	for (int i = 0; i < 2; i++)
	{
		m_WindowX = parser.ReadString(section, _T("WindowX"), m_WindowX.c_str());
		m_WindowY = parser.ReadString(section, _T("WindowY"), m_WindowY.c_str());
		m_AnchorX = parser.ReadString(section, _T("AnchorX"), m_AnchorX.c_str());
		m_AnchorY = parser.ReadString(section, _T("AnchorY"), m_AnchorY.c_str());

		if (!m_Rainmeter->GetDummyLitestep())
		{
			char tmpSz[MAX_LINE_LENGTH];
			// Check if step.rc has overrides these values
			if (GetRCString("RainmeterWindowX", tmpSz, ConvertToAscii(m_WindowX.c_str()).c_str(), MAX_LINE_LENGTH - 1))
			{
				m_WindowX = ConvertToWide(tmpSz);
			}
			if (GetRCString("RainmeterWindowY", tmpSz, ConvertToAscii(m_WindowY.c_str()).c_str(), MAX_LINE_LENGTH - 1))
			{
				m_WindowY = ConvertToWide(tmpSz);
			}
		}
		
		// Check if the window position should be read as a formula
		if (!m_WindowX.empty() && m_WindowX[0] == L'(' && m_WindowX[m_WindowX.size() - 1] == L')')
		{
			double value = parser.ReadFormula(section, _T("WindowX"), 0.0);
			TCHAR buffer[256];
			swprintf(buffer, L"%i", (int)value);
			m_WindowX = buffer;
		}
		if (!m_WindowY.empty() && m_WindowY[0] == L'(' && m_WindowY[m_WindowY.size() - 1] == L')')
		{
			double value = parser.ReadFormula(section, _T("WindowY"), 0.0);
			TCHAR buffer[256];
			swprintf(buffer, L"%i", (int)value);
			m_WindowY = buffer;
		}

		int zPos = parser.ReadInt(section, L"AlwaysOnTop", m_WindowZPosition);
		if (zPos == -1)
		{
			m_WindowZPosition = ZPOSITION_ONBOTTOM;
		}
		else if (zPos == -2)
		{
			m_WindowZPosition = ZPOSITION_ONDESKTOP;
		}
		else if (zPos == 1)
		{
			m_WindowZPosition = ZPOSITION_ONTOP;
		}
		else if (zPos == 2)
		{
			m_WindowZPosition = ZPOSITION_ONTOPMOST;
		}
		else
		{
			m_WindowZPosition = ZPOSITION_NORMAL;
		}

		m_WindowDraggable = 0!=parser.ReadInt(section, L"Draggable", m_WindowDraggable);
		m_WindowHide = (HIDEMODE)parser.ReadInt(section, L"HideOnMouseOver", m_WindowHide);
		m_WindowStartHidden = 0!=parser.ReadInt(section, L"StartHidden", m_WindowStartHidden);
		m_SavePosition = 0!=parser.ReadInt(section, L"SavePosition", m_SavePosition);
		m_SnapEdges = 0!=parser.ReadInt(section, L"SnapEdges", m_SnapEdges);
		m_MeasuresToVariables = 0!=parser.ReadInt(section, L"MeasuresToVariables", m_MeasuresToVariables);
		m_NativeTransparency = 0!=parser.ReadInt(section, L"NativeTransparency", m_NativeTransparency);
		m_ClickThrough = 0!=parser.ReadInt(section, L"ClickThrough", m_ClickThrough);
		m_KeepOnScreen = 0!=parser.ReadInt(section, L"KeepOnScreen", m_KeepOnScreen);
		m_AutoSelectScreen = 0!=parser.ReadInt(section, L"AutoSelectScreen", m_AutoSelectScreen);

		m_AlphaValue = parser.ReadInt(section, L"AlphaValue", m_AlphaValue);
		m_AlphaValue = min(255, m_AlphaValue);
		m_AlphaValue = max(0, m_AlphaValue);

		m_FadeDuration = parser.ReadInt(section, L"FadeDuration", m_FadeDuration);

		// Disable native transparency if not 2K/XP
		if(CRainmeter::IsNT() == PLATFORM_9X || CRainmeter::IsNT() == PLATFORM_NT4)
		{
			m_NativeTransparency = 0;
		}

		// On the second loop override settings from the skin's section
		section = m_SkinName.c_str();
	}

	// Set WindowXScreen/WindowYScreen temporarily
	WindowToScreen();
}

/*
** WriteConfig
**
** Writes the new settings to the config
**
*/
void CMeterWindow::WriteConfig()
{
	WCHAR buffer[256];
	std::wstring iniFile = m_Rainmeter->GetIniFile();
	const WCHAR* section = m_SkinName.c_str();

	if(!iniFile.empty())
	{
		// If position needs to be save, do so.
		if(m_SavePosition)
		{
			ScreenToWindow();
			WritePrivateProfileString(section, L"WindowX", m_WindowX.c_str(), iniFile.c_str());
			WritePrivateProfileString(section, L"WindowY", m_WindowY.c_str(), iniFile.c_str());
		}

		wsprintf(buffer, L"%i", m_AlphaValue);
		WritePrivateProfileString(section, L"AlphaValue", buffer, iniFile.c_str());

		wsprintf(buffer, L"%i", m_FadeDuration);
		WritePrivateProfileString(section, L"FadeDuration", buffer, iniFile.c_str());
		
		wsprintf(buffer, L"%i", m_ClickThrough);
		WritePrivateProfileString(section, L"ClickThrough", buffer, iniFile.c_str());
		wsprintf(buffer, L"%i", m_WindowDraggable);
		WritePrivateProfileString(section, L"Draggable", buffer, iniFile.c_str());
		wsprintf(buffer, L"%i", m_WindowHide);
		WritePrivateProfileString(section, L"HideOnMouseOver", buffer, iniFile.c_str());
		wsprintf(buffer, L"%i", m_SavePosition);
		WritePrivateProfileString(section, L"SavePosition", buffer, iniFile.c_str());
		wsprintf(buffer, L"%i", m_SnapEdges);
		WritePrivateProfileString(section, L"SnapEdges", buffer, iniFile.c_str());
		wsprintf(buffer, L"%i", m_KeepOnScreen);
		WritePrivateProfileString(section, L"KeepOnScreen", buffer, iniFile.c_str());
		wsprintf(buffer, L"%i", m_AutoSelectScreen);
		WritePrivateProfileString(section, L"AutoSelectScreen", buffer, iniFile.c_str());

		wsprintf(buffer, L"%i", m_WindowZPosition);
		WritePrivateProfileString(section, L"AlwaysOnTop", buffer, iniFile.c_str());
	}
}


/*
** ReadSkin
**
** Reads the skin config, creates the meters and measures and does the bindings.
**
*/
void CMeterWindow::ReadSkin()
{
	std::wstring iniFile = m_SkinPath;
	iniFile += m_SkinName;
	iniFile += L"\\";
	iniFile += m_SkinIniFile;

	m_Parser.Initialize(iniFile.c_str(), m_Rainmeter, this);

	// Global settings

	// Check the version 
	int appVersion = m_Parser.ReadInt(L"Rainmeter", L"AppVersion", 0);
	if (appVersion > RAINMETER_VERSION)
	{
		WCHAR buffer[256];
		std::wstring text;
		if (appVersion % 1000 != 0)
		{
			wsprintf(buffer, L"%i.%i.%i", appVersion / 1000000, (appVersion / 1000) % 1000, appVersion % 1000);
		}
		else
		{
			wsprintf(buffer, L"%i.%i", appVersion / 1000000, (appVersion / 1000) % 1000);
		}
		text = L"This skin needs Rainmeter version ";
		text += buffer;
		text += L" or newer.\nIt might not function as well as it should.\nYou should get an updated version or Rainmeter\nfrom http://www.rainmeter.net";
		MessageBox(m_Window, text.c_str(), L"Rainmeter", MB_OK);
	}

	m_Author = m_Parser.ReadString(L"Rainmeter", L"Author", L"");
	m_BackgroundName = m_Parser.ReadString(L"Rainmeter", L"Background", L"");
	m_BackgroundName = MakePathAbsolute(m_BackgroundName);

	std::wstring margins = m_Parser.ReadString(L"Rainmeter", L"BackgroundMargins", L"0, 0, 0, 0");
	int left = 0, top = 0, right = 0, bottom = 0;
	swscanf(margins.c_str(), L"%i, %i, %i, %i", &left, &top, &right, &bottom);
	m_BackgroundMargins.X = left;
	m_BackgroundMargins.Width = right - left;
	m_BackgroundMargins.Y = top;
	m_BackgroundMargins.Height = bottom - top;

	margins = m_Parser.ReadString(L"Rainmeter", L"DragMargins", L"0, 0, 0, 0");
	left = 0, top = 0, right = 0, bottom = 0;
	swscanf(margins.c_str(), L"%i, %i, %i, %i", &left, &top, &right, &bottom);
	m_DragMargins.X = left;
	m_DragMargins.Width = right - left;
	m_DragMargins.Y = top;
	m_DragMargins.Height = bottom - top;

	m_BackgroundMode = (BGMODE)m_Parser.ReadInt(L"Rainmeter", L"BackgroundMode", 0);
	m_SolidBevel = (BEVELTYPE)m_Parser.ReadInt(L"Rainmeter", L"BevelType", m_SolidBevel);

	m_SolidColor = m_Parser.ReadColor(L"Rainmeter", L"SolidColor", Color::Gray);
	m_SolidColor2 = m_Parser.ReadColor(L"Rainmeter", L"SolidColor2", m_SolidColor);
	m_SolidAngle = (Gdiplus::REAL)m_Parser.ReadFloat(L"Rainmeter", L"GradientAngle", 0.0);

	m_DynamicWindowSize = 0!=m_Parser.ReadInt(L"Rainmeter", L"DynamicWindowSize", m_DynamicWindowSize);

	if ((m_BackgroundMode == BGMODE_IMAGE || m_BackgroundMode == BGMODE_SCALED_IMAGE) && m_BackgroundName.empty())
	{
		m_BackgroundMode = BGMODE_COPY;
	}

	m_RightMouseDownAction = m_Parser.ReadString(L"Rainmeter", L"RightMouseDownAction", L"");
	m_LeftMouseDownAction = m_Parser.ReadString(L"Rainmeter", L"LeftMouseDownAction", L"");
	m_MiddleMouseDownAction = m_Parser.ReadString(L"Rainmeter", L"MiddleMouseDownAction", L"");
	m_RightMouseUpAction = m_Parser.ReadString(L"Rainmeter", L"RightMouseUpAction", L"");
	m_LeftMouseUpAction = m_Parser.ReadString(L"Rainmeter", L"LeftMouseUpAction", L"");
	m_MiddleMouseUpAction = m_Parser.ReadString(L"Rainmeter", L"MiddleMouseUpAction", L"");
	m_RightMouseDoubleClickAction = m_Parser.ReadString(L"Rainmeter", L"RightMouseDoubleClickAction", L"");
	m_LeftMouseDoubleClickAction = m_Parser.ReadString(L"Rainmeter", L"LeftMouseDoubleClickAction", L"");
	m_MiddleMouseDoubleClickAction = m_Parser.ReadString(L"Rainmeter", L"MiddleMouseDoubleClickAction", L"");
	m_MouseOverAction = m_Parser.ReadString(L"Rainmeter", L"MouseOverAction", L"");
	m_MouseLeaveAction = m_Parser.ReadString(L"Rainmeter", L"MouseLeaveAction", L"");
	m_OnRefreshAction = m_Parser.ReadString(L"Rainmeter", L"OnRefreshAction", L"");

	m_WindowUpdate = m_Parser.ReadInt(L"Rainmeter", L"Update", m_WindowUpdate);
	m_TransitionUpdate = m_Parser.ReadInt(L"Rainmeter", L"TransitionUpdate", m_TransitionUpdate);
	m_MouseActionCursor = 0 != m_Parser.ReadInt(L"Rainmeter", L"MouseActionCursor", 1);

	// Checking for localfonts
	std::wstring localFont1 = m_Parser.ReadString(L"Rainmeter", L"LocalFont", L"");
	// If there is a local font we want to load it
	if(!localFont1.empty())
	{
		// We want to check the fonts folder first
		// !!!!!!! - We may want to fix the method in which I get the path to
		// Rainmeter/fonts
		std::wstring szFontFile = m_Rainmeter->GetPath().c_str();
		
		m_FontCollection = new Gdiplus::PrivateFontCollection();
		int nResults = 0;
		nResults = m_FontCollection->AddFontFile(szFontFile.c_str());
		
		// It wasn't found in the fonts folder, check the local folder
		if(nResults != Ok)
		{
			szFontFile = m_SkinPath; // Get the local path	
			szFontFile += m_SkinName;
			szFontFile += L"\\";
			szFontFile += localFont1;

			nResults = m_FontCollection->AddFontFile(szFontFile.c_str());
			
			// The font wasn't found, check full path.
			if(nResults != Ok)
			{
				szFontFile = localFont1.c_str();
				nResults = m_FontCollection->AddFontFile(szFontFile.c_str());
				if(nResults != Ok)
				{
					std::wstring error = L"Error: Couldn't load font file: ";
					error += localFont1;
					DebugLog(error.c_str());
				}
			}

		}
		// Here we are checking to see if there are more than one local font
		// to be loaded. They will be named LocalFont2, LocalFont 3, etc.
		WCHAR tmpName[256];
		int i = 2;
		bool loop = true;
		do 
		{
			swprintf(tmpName, L"LocalFont%i", i);
			std::wstring LocalFont = m_Parser.ReadString(L"Rainmeter", tmpName, L"");
			// There is a key called LocalFont%i
			if (!LocalFont.empty())
			{
				// We want to check the fonts folder first
				// !!!!!!! - We may want to fix the method in which I get the path to
				// Rainmeter/fonts
				std::wstring szFontFile = m_Rainmeter->GetPath().c_str();
				szFontFile  += L"Fonts\\";
				szFontFile  += LocalFont;
				
				int nResults = 0;
				nResults = m_FontCollection->AddFontFile(szFontFile.c_str());
				
				// It wasn't found in the fonts folder, check the local folder
				if(nResults != Ok)
				{
					szFontFile = m_SkinPath; // Get the local path
					szFontFile += m_SkinName;
					szFontFile += LocalFont;
					nResults = m_FontCollection->AddFontFile(szFontFile.c_str());
					
					// The font wasn't found, check full path.
					if(nResults != Ok)
					{
						szFontFile = LocalFont.c_str();
						nResults = m_FontCollection->AddFontFile(szFontFile.c_str());
						// The font file wasn't found anywhere, log the error
						if(nResults != Ok)
						{
							std::wstring error = L"Error: Couldn't load font file: ";
							error += LocalFont;
							DebugLog(error.c_str());
						}
					} 
				}
			}
			// There were no extra Local Fonts found: exit loop.
			else
			{
				loop = false;
			}
			i++;
		} while(loop);
	}

	// Create the meters and measures

	// Get all the sections (i.e. different meters, measures and the other stuff)
	std::vector<std::wstring> arraySections = m_Parser.GetSections();

	for (size_t i = 0; i < arraySections.size(); i++)
	{
		std::wstring strSection = arraySections[i];

		if(wcsicmp(L"Rainmeter", strSection.c_str()) != 0 && 
			wcsicmp(L"Variables", strSection.c_str()) != 0 &&
			wcsicmp(L"Metadata", strSection.c_str()) != 0)
		{
			std::wstring meterName, measureName;

			// Check if the item is a meter or a measure (or perhaps something else)
			measureName = m_Parser.ReadString(strSection.c_str(), L"Measure", L"");
			meterName = m_Parser.ReadString(strSection.c_str(), L"Meter", L"");
			if (measureName.length() > 0)
			{
				try
				{
					// It's a measure
					CMeasure* measure = CMeasure::Create(measureName.c_str(), this);
					if (measure)
					{
						measure->SetName(strSection.c_str());
						measure->ReadConfig(m_Parser, strSection.c_str());
						m_Measures.push_back(measure);

						m_Parser.AddMeasure(measure);
					}
				}
				catch (CError& error)
				{
					MessageBox(m_Window, error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
				}
			}
			else if (meterName.length() > 0)
			{
				try
				{
					// It's a meter
					CMeter* meter = CMeter::Create(meterName.c_str(), this);
					if (meter)
					{
						meter->SetName(strSection.c_str());
						
						if(m_MouseActionCursor == false)
						{
							meter->SetMouseActionCursor(false);
						}

						meter->ReadConfig(strSection.c_str());
						m_Meters.push_back(meter);
					}
				}
				catch (CError& error)
				{
					MessageBox(m_Window, error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
				}
			}
			// If it's not a meter or measure it will be ignored
		}
	}

	if (m_Meters.empty())
	{
		MessageBox(m_Window, L"Your configuration file doesn't contain any meters!\nYour skin's ini-file might be out of date.", APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
	}
	else
	{
		// Bind the meters to the measures
		std::list<CMeter*>::iterator j = m_Meters.begin();
		for( ; j != m_Meters.end(); j++)
		{
			try
			{
				(*j)->BindMeasure(m_Measures);
			}
			catch (CError& error)
			{
				MessageBox(m_Window, error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
			}
		}
	}
}

/*
** InitializeMeters
**
** Initializes all the meters and the background
**
*/
void CMeterWindow::InitializeMeters()
{
	// Initalize all meters
	std::list<CMeter*>::iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); j++)
	{
		try
		{
			(*j)->Initialize();
		}
		catch (CError& error)
		{
			MessageBox(m_Window, error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
		}
	}

	Update(true);
	ResizeWindow(true);
}

/*
** ResizeWindow
** 
** Changes the size of the window and re-adjusts the background
*/
bool CMeterWindow::ResizeWindow(bool reset)
{
	int w = m_BackgroundMargins.GetLeft();
	int h = m_BackgroundMargins.GetTop();

	// Get the largest meter point
	std::list<CMeter*>::iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); j++)
	{
		w = max(w, (*j)->GetX() + (*j)->GetW());
		h = max(h, (*j)->GetY() + (*j)->GetH());
	}

	w += m_BackgroundMargins.GetRight();
	h += m_BackgroundMargins.GetBottom();

	w = max(w, m_BackgroundSize.cx);
	h = max(h, m_BackgroundSize.cy);

	if (!reset && m_WindowW == w && m_WindowH == h)
	{
		WindowToScreen();
		return false;		// The window is already correct size
	}

	// Reset size (this is calculated below)
	m_WindowW = 0;
	m_WindowH = 0;

	if (m_Background)
	{
		delete m_Background;
		m_Background = NULL;
	}

	if ((m_BackgroundMode == BGMODE_IMAGE || m_BackgroundMode == BGMODE_SCALED_IMAGE) && !m_BackgroundName.empty())
	{
		// Load the background
		m_Background = new Bitmap(m_BackgroundName.c_str());
		Status status = m_Background->GetLastStatus();
		if(Ok != status)
		{
			std::wstring err = L"Unable to load background: " + m_BackgroundName;
			MessageBox(m_Window, err.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
			delete m_Background;
			m_Background = NULL;
			m_BackgroundSize.cx = 0;
			m_BackgroundSize.cy = 0;
		}

		// Calculate the window dimensions
		if(m_Background)
		{
			m_BackgroundSize.cx = m_Background->GetWidth();
			m_BackgroundSize.cy = m_Background->GetHeight();

			w = max(w, m_BackgroundSize.cx);
			h = max(h, m_BackgroundSize.cy);

			if (m_BackgroundMode == BGMODE_SCALED_IMAGE)
			{
				// Scale the background to fill the whole window
				Bitmap* scaledBackground = new Bitmap(w, h, PixelFormat32bppARGB);

				Graphics graphics(scaledBackground);

				if (m_BackgroundMargins.GetTop() > 0) 
				{
					if (m_BackgroundMargins.GetLeft() > 0) 
					{
						// Top-Left
						Rect r(0, 0, m_BackgroundMargins.GetLeft(), m_BackgroundMargins.GetTop());
						graphics.DrawImage(m_Background, r, 0, 0, m_BackgroundMargins.GetLeft(), m_BackgroundMargins.GetTop(), UnitPixel);
					}

					// Top
					Rect r(m_BackgroundMargins.GetLeft(), 0, w - m_BackgroundMargins.GetLeft() - m_BackgroundMargins.GetRight(), m_BackgroundMargins.GetTop());
					graphics.DrawImage(m_Background, r, m_BackgroundMargins.GetLeft(), 0, m_Background->GetWidth() - m_BackgroundMargins.GetLeft() - m_BackgroundMargins.GetRight(), m_BackgroundMargins.GetTop(), UnitPixel);

					if (m_BackgroundMargins.GetRight() > 0) 
					{
						// Top-Right
						Rect r(w - m_BackgroundMargins.GetRight(), 0, m_BackgroundMargins.GetRight(), m_BackgroundMargins.GetTop());
						graphics.DrawImage(m_Background, r, m_Background->GetWidth() - m_BackgroundMargins.GetRight(), 0, m_BackgroundMargins.GetRight(), m_BackgroundMargins.GetTop(), UnitPixel);
					}
				}

				if (m_BackgroundMargins.GetLeft() > 0) 
				{
					// Left
					Rect r(0, m_BackgroundMargins.GetTop(), m_BackgroundMargins.GetLeft(), h - m_BackgroundMargins.GetTop() - m_BackgroundMargins.GetBottom());
					graphics.DrawImage(m_Background, r, 0, m_BackgroundMargins.GetTop(), m_BackgroundMargins.GetLeft(), m_Background->GetHeight() - m_BackgroundMargins.GetTop() - m_BackgroundMargins.GetBottom(), UnitPixel);
				}

				// Center
				Rect r(m_BackgroundMargins.GetLeft(), m_BackgroundMargins.GetTop(), w - m_BackgroundMargins.GetLeft() - m_BackgroundMargins.GetRight(), h - m_BackgroundMargins.GetTop() - m_BackgroundMargins.GetBottom());
				graphics.DrawImage(m_Background, r, m_BackgroundMargins.GetLeft(), m_BackgroundMargins.GetTop(), m_Background->GetWidth() - m_BackgroundMargins.GetLeft() - m_BackgroundMargins.GetRight(), m_Background->GetHeight() - m_BackgroundMargins.GetTop() - m_BackgroundMargins.GetBottom(), UnitPixel);

				if (m_BackgroundMargins.GetRight() > 0) 
				{
					// Right
					Rect r(w - m_BackgroundMargins.GetRight(), m_BackgroundMargins.GetTop(), m_BackgroundMargins.GetRight(), h - m_BackgroundMargins.GetTop() - m_BackgroundMargins.GetBottom());
					graphics.DrawImage(m_Background, r, m_Background->GetWidth() - m_BackgroundMargins.GetRight(), m_BackgroundMargins.GetTop(), m_BackgroundMargins.GetRight(), m_Background->GetHeight() - m_BackgroundMargins.GetTop() - m_BackgroundMargins.GetBottom(), UnitPixel);
				}
				
				if (m_BackgroundMargins.GetBottom() > 0) 
				{
					if (m_BackgroundMargins.GetLeft() > 0) 
					{
						// Bottom-Left
						Rect r(0, h - m_BackgroundMargins.GetBottom(), m_BackgroundMargins.GetLeft(), m_BackgroundMargins.GetBottom());
						graphics.DrawImage(m_Background, r, 0, m_Background->GetHeight() - m_BackgroundMargins.GetBottom(), m_BackgroundMargins.GetLeft(), m_BackgroundMargins.GetBottom(), UnitPixel);
					}
					// Bottom
					Rect r(m_BackgroundMargins.GetLeft(), h - m_BackgroundMargins.GetBottom(), w - m_BackgroundMargins.GetLeft() - m_BackgroundMargins.GetRight(), m_BackgroundMargins.GetBottom());
					graphics.DrawImage(m_Background, r, m_BackgroundMargins.GetLeft(), m_Background->GetHeight() - m_BackgroundMargins.GetBottom(), m_Background->GetWidth() - m_BackgroundMargins.GetLeft() - m_BackgroundMargins.GetRight(), m_BackgroundMargins.GetBottom(), UnitPixel);

					if (m_BackgroundMargins.GetRight() > 0) 
					{
						// Bottom-Right
						Rect r(w - m_BackgroundMargins.GetRight(), h - m_BackgroundMargins.GetBottom(), m_BackgroundMargins.GetRight(), m_BackgroundMargins.GetBottom());
						graphics.DrawImage(m_Background, r, m_Background->GetWidth() - m_BackgroundMargins.GetRight(), m_Background->GetHeight() - m_BackgroundMargins.GetBottom(), m_BackgroundMargins.GetRight(), m_BackgroundMargins.GetBottom(), UnitPixel);
					}
				}

				delete m_Background;
				m_Background = scaledBackground;
			}

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
	} 
	else
	{
		m_WindowW = w;
		m_WindowH = h;
		WindowToScreen();
	}

	// If Background is not set, take a copy from the desktop
	if(m_Background == NULL) 
	{
		if (m_BackgroundMode == BGMODE_COPY)
		{
			if (!m_NativeTransparency)
			{
				m_Background = GrabDesktop(m_ScreenX, m_ScreenY, m_WindowW, m_WindowH);
			}
		}
		else
		{
			// Create a solid color bitmap for the background
			m_Background = new Bitmap(m_WindowW, m_WindowH, PixelFormat32bppARGB);
			Graphics graphics(m_Background);

			if (m_SolidColor.GetValue() == m_SolidColor2.GetValue())
			{
				SolidBrush solid(m_SolidColor);
				graphics.FillRectangle(&solid, 0, 0, m_WindowW, m_WindowH);
			}
			else
			{
				Rect r(0, 0, m_WindowW, m_WindowH);
				LinearGradientBrush gradient(r, m_SolidColor, m_SolidColor2, m_SolidAngle, TRUE);
				graphics.FillRectangle(&gradient, r);
			}

			if (m_SolidBevel != BEVELTYPE_NONE)
			{
				Pen light(Color(255, 255, 255, 255));
				Pen dark(Color(255, 0, 0, 0));

				if (m_SolidBevel == BEVELTYPE_DOWN)
				{
					light.SetColor(Color(255, 0, 0, 0));
					dark.SetColor(Color(255, 255, 255, 255));
				}
				Rect rect(0, 0, m_WindowW, m_WindowH);	
				CMeter::DrawBevel(graphics, rect, light, dark);
			}
		}
	}
    
	return true;
}

/*
** GrabDesktop
** 
** Grabs a part of the desktop
*/
Bitmap* CMeterWindow::GrabDesktop(int x, int y, int w, int h)
{
	HDC desktopDC = GetDC(GetDesktopWindow());
	HDC dc = CreateCompatibleDC(desktopDC);
	HBITMAP desktopBM = CreateCompatibleBitmap(desktopDC, w, h);
	HBITMAP oldBM = (HBITMAP)SelectObject(dc, desktopBM);
	BitBlt(dc, 0, 0, w, h, desktopDC, x, y, SRCCOPY);
	SelectObject(dc, oldBM);
	DeleteDC(dc);
	ReleaseDC(GetDesktopWindow(), desktopDC);
	Bitmap* background = new Bitmap(desktopBM, NULL);
	DeleteObject(desktopBM);
	return background;
}

/*
** CreateRegion
**
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
		// Set window region if needed
		if(!m_BackgroundName.empty()) 
		{
			HBITMAP background = NULL;
			m_DoubleBuffer->GetHBITMAP(Color(255,0,255), &background);
			if (background)
			{
				HRGN region = BitmapToRegion(background, RGB(255,0,255), 0x101010, 0, 0);
				SetWindowRgn(m_Window, region, TRUE);
				DeleteObject(background);
			}
		}
	}
}

/*
** Redraw
**
** Redraws the meters and paints the window
**
*/
void CMeterWindow::Redraw() 
{
	if (m_ResetRegion)
	{
		ResizeWindow(false);
		CreateRegion(true);
	}

	if (m_DoubleBuffer) delete m_DoubleBuffer;
	m_DoubleBuffer = new Bitmap(m_WindowW, m_WindowH, PixelFormat32bppARGB);

	Graphics graphics(GetDoubleBuffer());

	if (m_Background)
	{
		// Copy the background over the doublebuffer
		Rect r(0, 0, m_WindowW, m_WindowH);
		graphics.DrawImage(m_Background, r, 0, 0, m_Background->GetWidth(), m_Background->GetHeight(), UnitPixel);
	}

	// Draw the meters
	std::list<CMeter*>::iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); j++)
	{
		try
		{
			if (!(*j)->GetTransformationMatrix().IsIdentity())
			{
				// Change the world matrix
				graphics.SetTransform(&((*j)->GetTransformationMatrix()));

				(*j)->Draw(graphics);

				// Set back to identity matrix
				graphics.ResetTransform();
			}
			else
			{
				(*j)->Draw(graphics);
			}
		}
		catch (CError& error)
		{
			MessageBox(m_Window, error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
		}
	}
		
	if (m_ResetRegion) CreateRegion(false);
	m_ResetRegion = false;

	UpdateTransparency(m_TransparencyValue, false);

	if (!m_NativeTransparency)
	{
		InvalidateRect(m_Window, NULL, FALSE);
	}
}


/*
** Update
**
** Updates all the measures and redraws the meters
**
*/
void CMeterWindow::Update(bool nodraw)
{
	m_UpdateCounter++;

	// Pre-updates
	CMeasureNet::UpdateIFTable();
	CMeasureCalc::UpdateVariableMap(*this);

	// Update all measures
	std::list<CMeasure*>::iterator i = m_Measures.begin();
	for( ; i != m_Measures.end(); i++)
	{
		try
		{
			if ((*i)->HasDynamicVariables() && dynamic_cast<CMeasurePlugin*>((*i)) == NULL)		// Plugins are not meant to be reinitialized
			{
				(*i)->ReadConfig(m_Parser, (*i)->GetName());
			}
			(*i)->Update();
		}
		catch (CError& error)
		{
			MessageBox(m_Window, error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
		}
	}

	// Update the meters
	std::list<CMeter*>::iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); j++)
	{
		try
		{
			if ((*j)->HasDynamicVariables())
			{
				(*j)->ReadConfig((*j)->GetName());
			}
			(*j)->Update();
		}
		catch (CError& error)
		{
			MessageBox(m_Window, error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
		}
	}

	// Statistics
	CMeasureNet::UpdateStats();
	Rainmeter->WriteStats(false);

	if (!nodraw)
	{
		if (m_DynamicWindowSize)
		{
			// Resize the window
			m_ResetRegion = true;
		}

		Redraw();
	}

	// Check for transitions and start the timer if necessary
	bool bActiveTransition = false;
	j = m_Meters.begin();
	for( ; j != m_Meters.end(); j++)
	{
		if ((*j)->HasActiveTransition())
		{
			bActiveTransition = true;
			break;
		}
	}

	// Start/stop the transition timer if necessary
	if (!m_ActiveTransition && bActiveTransition)
	{
		SetTimer(m_Window, TRANSITIONTIMER, m_TransitionUpdate, NULL);
	}
	else if (m_ActiveTransition && !bActiveTransition)
	{
		KillTimer(m_Window, TRANSITIONTIMER);
	}

//	if (m_MeasuresToVariables)	// BUG: LSSetVariable doens't seem to work for some reason.
//	{
//		std::list<CMeasure*>::iterator i = m_Measures.begin();
//		for( ; i != m_Measures.end(); i++)
//		{
//			const char* sz = (*i)->GetStringValue(true, 1, 1, false);
//			if (sz && wcslen(sz) > 0)
//			{
//				WCHAR* wideSz = CMeter::ConvertToWide(sz);
//				WCHAR* wideName = CMeter::ConvertToWide((*i)->GetName());
//				LSSetVariable(wideName, wideSz);
//				delete [] wideSz;
//				delete [] wideName;
//			}
//		}
//	}
}

/*
** UpdateTransparency
**
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

		typedef BOOL (WINAPI * FPUPDATELAYEREDWINDOW)(HWND hWnd, HDC hdcDst, POINT *pptDst, SIZE *psize, HDC hdcSrc, POINT *pptSrc, COLORREF crKey, BLENDFUNCTION *pblend, DWORD dwFlags);
		FPUPDATELAYEREDWINDOW UpdateLayeredWindow = (FPUPDATELAYEREDWINDOW)GetProcAddress(m_User32Library, "UpdateLayeredWindow");

		BLENDFUNCTION blendPixelFunction= {AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA};
		POINT ptWindowScreenPosition = {m_ScreenX, m_ScreenY};
		POINT ptSrc = {0, 0};
		SIZE szWindow = {m_WindowW, m_WindowH};

		HDC dcScreen = GetDC(GetDesktopWindow());
		HDC dcMemory = CreateCompatibleDC(dcScreen);

		HBITMAP dbBitmap;
		m_DoubleBuffer->GetHBITMAP(Color(0, 0, 0, 0), &dbBitmap);
		HBITMAP oldBitmap = (HBITMAP)SelectObject(dcMemory, dbBitmap);
		UpdateLayeredWindow(m_Window, dcScreen, &ptWindowScreenPosition, &szWindow, dcMemory, &ptSrc, 0, &blendPixelFunction, ULW_ALPHA);
		ReleaseDC(GetDesktopWindow(), dcScreen);
		SelectObject(dcMemory, oldBitmap);
		DeleteDC(dcMemory);
		DeleteObject(dbBitmap);

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
** OnPaint
**
** Repaints the window. This does not cause update of the measures.
**
*/
LRESULT CMeterWindow::OnPaint(WPARAM wParam, LPARAM lParam) 
{
	PAINTSTRUCT ps;
	HDC winDC = BeginPaint(m_Window, &ps);

	Graphics graphics(winDC);
	graphics.DrawImage(m_DoubleBuffer, 0, 0);

	EndPaint(m_Window, &ps);
	return 0;
}

/*
** OnTimer
**
** Handles the timers. The METERTIMER updates all the measures 
** MOUSETIMER is used to hide/show the window.
**
*/
LRESULT CMeterWindow::OnTimer(WPARAM wParam, LPARAM lParam) 
{
	if(wParam == METERTIMER) 
	{
		Update(false);
		UpdateAboutStatistics();

		//if (m_KeepOnScreen) 
		//{
		//	int x = m_ScreenX;
		//	int y = m_ScreenY;
		//	MapCoordsToScreen(x, y, m_WindowW, m_WindowH);
		//	if (x != m_ScreenX || y != m_ScreenY)
		//	{
		//		MoveWindow(x, y);
		//	}
		//}

		Rainmeter->ClearDeleteLaterList();
	}
	else if(wParam == TRANSITIONTIMER)
	{
		// Redraw only if there is active transition still going
		bool bActiveTransition = false;
		std::list<CMeter*>::iterator j = m_Meters.begin();
		for( ; j != m_Meters.end(); j++)
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
	}
	else if(wParam == MOUSETIMER)
	{
		ShowWindowIfAppropriate();

		POINT pos;
		GetCursorPos(&pos);
		MapWindowPoints(NULL, m_Window, &pos, 1);
		
		if (!m_MouseLeaveAction.empty())
		{
			// Check mouse leave actions
			DoAction(pos.x, pos.y, MOUSE_LEAVE, false);
		}

		if (m_WindowZPosition == ZPOSITION_ONTOPMOST)
		{
			ChangeZPos(ZPOSITION_ONTOPMOST);
		}

		// Handle buttons
		bool redraw = false;
		std::list<CMeter*>::iterator j = m_Meters.begin();
		for( ; j != m_Meters.end(); j++)
		{
			// Hidden meters are ignored
			if ((*j)->IsHidden()) continue;

			CMeterButton* button = dynamic_cast<CMeterButton*>(*j);
			if (button)
			{
				redraw |= button->MouseMove(pos);
			}
		}
		if (redraw)
		{
			Redraw();
		}
	}
	else if(wParam == FADETIMER)
	{
		DWORD ticks = GetTickCount();
		if (m_FadeStartTime == 0)
		{
			m_FadeStartTime = ticks;
		}

		if (ticks - m_FadeStartTime > (DWORD)m_FadeDuration)
		{
			KillTimer(m_Window, FADETIMER);
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
			double value = (ticks - m_FadeStartTime);
			value /= m_FadeDuration;
 			value *= m_FadeEndValue - m_FadeStartValue;
			value += m_FadeStartValue;
			value = min(value, 255);
			value = max(value, 0);

			UpdateTransparency((int)value, false);
		}
	}

//	// TEST
//	if (!m_ChildWindow)
//	{
//		RECT rect;
//		GetWindowRect(m_Window, &rect);
//		if (rect.left != m_WindowX && rect.top != m_WindowY)
//		{
//			DebugLog(L"Window position has been changed. Moving it back to the place it belongs.");
//			SetWindowPos(m_Window, NULL, m_WindowX, m_WindowY, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
//		}
//	}

	// ~TEST

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
				ShowWindow(m_Window, SW_SHOWNOACTIVATE);
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
			ShowWindow(m_Window, SW_SHOWNOACTIVATE);
		}

		SetTimer(m_Window, FADETIMER, 10, NULL);
	}
}

/*
** ShowWindowIfAppropriate
**
** Show the window if it is temporarily hidden.
**
*/
void CMeterWindow::ShowWindowIfAppropriate()
{
	bool inside = false;
	bool keyDown = GetKeyState(VK_CONTROL) & 0x8000 || GetKeyState(VK_SHIFT) & 0x8000 || GetKeyState(VK_MENU) & 0x8000;
	POINT pos;
	RECT rect;

	GetWindowRect(m_Window, &rect);
	GetCursorPos(&pos);

	if(rect.left <= pos.x && rect.right > pos.x &&
	   rect.top <= pos.y && rect.bottom > pos.y) 
	{
		// Check transparent pixels
		if (m_DoubleBuffer)
		{
			Color color;
			m_DoubleBuffer->GetPixel(pos.x - rect.left, pos.y - rect.top, &color);
			if (color.GetA() != 0)
			{
				inside = true;
			}
		}
		else
		{
			inside = true;
		}
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

	if(m_WindowHide)
	{
		if (!inside && !keyDown)
		{
			switch(m_WindowHide) 
			{
			case HIDEMODE_HIDE:
				if (m_TransparencyValue == 0 || !IsWindowVisible(m_Window))
				{
					ShowWindow(m_Window, SW_SHOWNOACTIVATE);
					FadeWindow(0, m_AlphaValue);
				}
				break;

			case HIDEMODE_FADEIN:
				if (m_TransparencyValue == 255)
				{
					FadeWindow(255, m_AlphaValue);
				}
				break;

			case HIDEMODE_FADEOUT:
				if (m_TransparencyValue == m_AlphaValue)
				{
					FadeWindow(m_AlphaValue, 255);
				}
				break;
			}
		}
	}
}


/*
** OnMouseMove
**
** When we get WM_MOUSEMOVE messages, hide the window as the mouse is over it.
**
*/
LRESULT CMeterWindow::OnMouseMove(WPARAM wParam, LPARAM lParam) 
{
	bool keyDown = GetKeyState(VK_CONTROL) & 0x8000 || GetKeyState(VK_SHIFT) & 0x8000 || GetKeyState(VK_MENU) & 0x8000;

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

		// If Alt, shift or control is down, do not hide the window
		switch(m_WindowHide) 
		{
		case HIDEMODE_HIDE:
			if (m_TransparencyValue == m_AlphaValue)
			{
				FadeWindow(m_AlphaValue, 0);
			}
			break;

		case HIDEMODE_FADEIN:
			if (m_TransparencyValue == m_AlphaValue)
			{
				FadeWindow(m_AlphaValue, 255);
			}
			break;

		case HIDEMODE_FADEOUT:
			if (m_TransparencyValue == 255)
			{
				FadeWindow(255, m_AlphaValue);
			}
			break;
		}
	}

	POINT pos;
	pos.x = (SHORT)LOWORD(lParam);
	pos.y = (SHORT)HIWORD(lParam);

	if (m_Message == WM_NCMOUSEMOVE)
	{
		// Map to local window
		MapWindowPoints(GetDesktopWindow(), m_Window, &pos, 1);
	}
	
	DoAction(pos.x, pos.y, MOUSE_OVER, false);

	// Handle buttons
	bool redraw = false;
	bool drawCursor = false;
	std::list<CMeter*>::iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); j++)
	{
		// Hidden meters are ignored
		if ((*j)->IsHidden()) continue;

		CMeterButton* button = dynamic_cast<CMeterButton*>(*j);
		if (button)
		{
			redraw |= button->MouseMove(pos);
		}

		if((*j)->HitTest(pos.x, pos.y) && (*j)->HasMouseActionCursor())
		{
			drawCursor |= ((*j)->HasMouseAction() || button);	
		}	
	}

	if(drawCursor)
	{
		SetCursor(LoadCursor(NULL, IDC_HAND));
	}
	else
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	}

	if (redraw)
	{
		Redraw();
	}

	return 0;
}

/*
** OnCreate
**
** During window creation we do nothing.
**
*/
LRESULT CMeterWindow::OnCreate(WPARAM wParam, LPARAM lParam) 
{
	return 0;
}

/*
** OnCommand
**
** Handle the menu commands.
**
*/
LRESULT CMeterWindow::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	try 
	{
		if(wParam == ID_CONTEXT_SKINMENU_EDITSKIN)
		{
			std::wstring command = Rainmeter->GetConfigEditor();
			command += L" \"";
			command += m_SkinPath + L"\\" + m_SkinName + L"\\" + m_SkinIniFile + L"\"";

			// If the skins are in the program folder start the editor as admin
			if (Rainmeter->GetPath() + L"Skins\\" == Rainmeter->GetSkinPath())
			{
				LSExecuteAsAdmin(NULL, command.c_str(), SW_SHOWNORMAL);
			}
			else
			{
				LSExecute(NULL, command.c_str(), SW_SHOWNORMAL);
			}
		}
		else if(wParam == ID_CONTEXT_SKINMENU_OPENSKINSFOLDER)
		{
			std::wstring command;
			command += L"\"";
			command += m_SkinPath + L"\\" + m_SkinName;
			command += L"\"";
			LSExecute(NULL, command.c_str(), SW_SHOWNORMAL);
		} 
		else if(wParam == ID_CONTEXT_SKINMENU_REFRESH)
		{
			Refresh(false);
		} 
		else if(wParam == ID_CONTEXT_SKINMENU_VERYTOPMOST)
		{
			ChangeZPos(ZPOSITION_ONTOPMOST);
			WriteConfig();
		}
		else if(wParam == ID_CONTEXT_SKINMENU_TOPMOST)
		{
			ChangeZPos(ZPOSITION_ONTOP);
			WriteConfig();
		}
		else if(wParam == ID_CONTEXT_SKINMENU_BOTTOM)
		{
			ChangeZPos(ZPOSITION_ONBOTTOM);
			WriteConfig();
		}
		else if(wParam == ID_CONTEXT_SKINMENU_NORMAL)
		{
			ChangeZPos(ZPOSITION_NORMAL);
			WriteConfig();
		}
		else if(wParam == ID_CONTEXT_SKINMENU_ONDESKTOP)
		{
			ChangeZPos(ZPOSITION_ONDESKTOP);
			WriteConfig();
		}
		else if(wParam == ID_CONTEXT_SKINMENU_KEEPONSCREEN)
		{
			m_KeepOnScreen = !m_KeepOnScreen;
			WriteConfig();

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
		else if(wParam == ID_CONTEXT_SKINMENU_CLICKTHROUGH)
		{
			m_ClickThrough = !m_ClickThrough;
			WriteConfig();
			Refresh(false);
		}
		else if(wParam == ID_CONTEXT_SKINMENU_DRAGGABLE)
		{
			m_WindowDraggable = !m_WindowDraggable;
			WriteConfig();
		}
		else if(wParam == ID_CONTEXT_SKINMENU_HIDEONMOUSE)
		{
			if (m_WindowHide == HIDEMODE_NONE)
			{
				m_WindowHide = HIDEMODE_HIDE;
			}
			else
			{
				m_WindowHide = HIDEMODE_NONE;
			}
			WriteConfig();
			Refresh(false);
		}		
		else if(wParam == ID_CONTEXT_SKINMENU_TRANSPARENCY_FADEIN)
		{
			if (m_WindowHide == HIDEMODE_NONE)
			{
				m_WindowHide = HIDEMODE_FADEIN;
			}
			else
			{
				m_WindowHide = HIDEMODE_NONE;
			}
			WriteConfig();
			Refresh(false);
		}
		else if(wParam == ID_CONTEXT_SKINMENU_TRANSPARENCY_FADEOUT)
		{
			if (m_WindowHide == HIDEMODE_NONE)
			{
				m_WindowHide = HIDEMODE_FADEOUT;
			}
			else
			{
				m_WindowHide = HIDEMODE_NONE;
			}
			WriteConfig();
			Refresh(false);
		}
		else if(wParam == ID_CONTEXT_SKINMENU_REMEMBERPOSITION)
		{
			m_SavePosition = !m_SavePosition;
			WriteConfig();
		}
		else if(wParam == ID_CONTEXT_SKINMENU_SNAPTOEDGES)
		{
			m_SnapEdges = !m_SnapEdges;
			WriteConfig();
		}
		else if(wParam >= ID_CONTEXT_SKINMENU_TRANSPARENCY_0 && wParam <= ID_CONTEXT_SKINMENU_TRANSPARENCY_90)
		{
			m_AlphaValue = (int)(255.0 - 230.0 * (double)(wParam - ID_CONTEXT_SKINMENU_TRANSPARENCY_0) / (double)(ID_CONTEXT_SKINMENU_TRANSPARENCY_90 - ID_CONTEXT_SKINMENU_TRANSPARENCY_0));
			WriteConfig();
			Refresh(false);
		}
		else if(wParam == ID_CONTEXT_CLOSESKIN)
		{
			const std::vector<CRainmeter::CONFIG>& configs = m_Rainmeter->GetAllConfigs();

			for (size_t i = 0; i < configs.size(); i++)
			{
				if (configs[i].config == m_SkinName)
				{
					m_Rainmeter->DeactivateConfig(this, i);
					break;
				}
			}
		}
		else if(wParam == ID_CONTEXT_SKINMENU_FROMRIGHT)
		{
			m_WindowXFromRight = !m_WindowXFromRight;

			ScreenToWindow();

			if (m_SavePosition)
			{
				WriteConfig();
			}
		}
		else if(wParam == ID_CONTEXT_SKINMENU_FROMBOTTOM)
		{
			m_WindowYFromBottom = !m_WindowYFromBottom;

			ScreenToWindow();

			if (m_SavePosition)
			{
				WriteConfig();
			}
		}
		else if(wParam == ID_CONTEXT_SKINMENU_XPERCENTAGE)
		{
			m_WindowXPercentage = !m_WindowXPercentage;

			ScreenToWindow();

			if (m_SavePosition)
			{
				WriteConfig();
			}
		}
		else if(wParam == ID_CONTEXT_SKINMENU_YPERCENTAGE)
		{
			m_WindowYPercentage = !m_WindowYPercentage;

			ScreenToWindow();

			if (m_SavePosition)
			{
				WriteConfig();
			}
		}
		else if (wParam == ID_CONTEXT_SKINMENU_MONITOR_AUTOSELECT)
		{
			m_AutoSelectScreen = !m_AutoSelectScreen;

			ScreenToWindow();

			WriteConfig();
		}
		else if (wParam == ID_CONTEXT_SKINMENU_MONITOR_PRIMARY || wParam >= ID_MONITOR_FIRST && wParam <= ID_MONITOR_LAST)
		{
			std::vector<MONITOR_INFO>& monitors = c_Monitors.monitors;

			int screenIndex;
			bool screenDefined;
			if (wParam == ID_CONTEXT_SKINMENU_MONITOR_PRIMARY)
			{
				screenIndex = c_Monitors.primary;
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

				m_Parser.ResetVariables(m_Rainmeter, this);  // Set present monitor variables
				ScreenToWindow();

				WriteConfig();
			}
		}
		else
		{
			// Forward to tray window, which handles all the other commands
			SendMessage(m_Rainmeter->GetTrayWindow()->GetWindow(), WM_COMMAND, wParam, lParam);
		}
	} 
    catch(CError& error) 
    {
		MessageBox(m_Window, error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
	}

	return 0;
}

/*
** OnSysCommand
**
** Handle dragging the window
**
*/
LRESULT CMeterWindow::OnSysCommand(WPARAM wParam, LPARAM lParam) 
{
	if ((wParam & 0xFFF0) != SC_MOVE)
	{
		return DefWindowProc(m_Window, m_Message, wParam, lParam);
	}

	// --- SC_MOVE ---

	// Prepare the dragging flags
	m_Dragging = true;
	m_Dragged = false;

	std::wstring startWindowX = m_WindowX;
	std::wstring startWindowY = m_WindowY;

	// Run the DefWindowProc so the dragging works
	LRESULT result = DefWindowProc(m_Window, m_Message, wParam, lParam);

	if (m_Dragged)
	{
		ScreenToWindow();

		// Write the new place of the window to config file
		if (m_SavePosition)
		{
			WriteConfig();
		}
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
** OnEnterSizeMove
**
** Starts dragging
**
*/
LRESULT CMeterWindow::OnEnterSizeMove(WPARAM wParam, LPARAM lParam) 
{
	if (m_Dragging)
	{
		m_Dragged = true;  // Don't post the WM_NCLBUTTONUP message!
	}

	return 0;
}

/*
** OnExitSizeMove
**
** Ends dragging
**
*/
LRESULT CMeterWindow::OnExitSizeMove(WPARAM wParam, LPARAM lParam) 
{
	return 0;
}

/*
** OnNcHitTest
**
** This is overwritten so that the window can be dragged
**
*/
LRESULT CMeterWindow::OnNcHitTest(WPARAM wParam, LPARAM lParam) 
{
	if (m_WindowDraggable)
	{
		POINT pos;
		pos.x = (SHORT)LOWORD(lParam);
		pos.y = (SHORT)HIWORD(lParam);
		MapWindowPoints(GetDesktopWindow(), m_Window, &pos, 1);

		int x1 = m_DragMargins.GetLeft();
		int x2 = m_WindowW - m_DragMargins.GetRight();
		int y1 = m_DragMargins.GetTop();
		int y2 = m_WindowH - m_DragMargins.GetBottom();

		if (x1 < 0) x1 += m_WindowW;
		if (y1 < 0) y1 += m_WindowH;
		if (x2 > m_WindowW) x2 -= m_WindowW;
		if (y2 > m_WindowH) y2 -= m_WindowH;

		if (pos.x >= x1 && pos.x < x2) 
		{
			if (pos.y >= y1 && pos.y < y2) 
			{
				return HTCAPTION;
			}
		}
	}
	return HTCLIENT;
}

/*
** OnWindowPosChanging
**
** Called when windows position is about to change
**
*/
LRESULT CMeterWindow::OnWindowPosChanging(WPARAM wParam, LPARAM lParam) 
{
	LPWINDOWPOS wp=(LPWINDOWPOS)lParam;

	if(m_WindowZPosition == ZPOSITION_ONBOTTOM && !m_Refreshing)
	{
		// do not change the z-order. This keeps the window on bottom.
		wp->flags|=SWP_NOZORDER;
	}

	if ((wp->flags & SWP_NOMOVE) == 0) 
	{
		if (m_PreventMoving)
		{
			wp->flags |= SWP_NOMOVE;
			m_PreventMoving = false;

			return DefWindowProc(m_Window, m_Message, wParam, lParam);
		}

		if (m_Dragging)
		{
			// Convert the client coordinates of the shell window to the screen coordinates
			if (!m_ChildWindow && m_WindowZPosition == ZPOSITION_ONDESKTOP)
			{
				POINT pos = {wp->x, wp->y};
				if (ClientToScreen(GetAncestor(m_Window, GA_PARENT), &pos))
				{
					wp->x = pos.x;
					wp->y = pos.y;
				}
			}
		}
		else
		{
			// Convert the client coordinates of the shell window to the screen coordinates
			if (!m_ChildWindow && !m_NativeTransparency && m_WindowZPosition == ZPOSITION_ONDESKTOP)
			{
				POINT pos = {wp->x, wp->y};
				if (ClientToScreen(GetAncestor(m_Window, GA_PARENT), &pos))
				{
					wp->x = pos.x;
					wp->y = pos.y;
				}
			}
		}

		if (m_SnapEdges && !(GetKeyState(VK_CONTROL) & 0x8000 || GetKeyState(VK_SHIFT) & 0x8000))
		{
			// only process movement (ignore anything without winpos values)
			if(wp->cx != 0 && wp->cy != 0)
			{
				RECT workArea;

				//HMONITOR hMonitor = MonitorFromWindow(m_Window, MONITOR_DEFAULTTONULL);  // returns incorrect monitor when the window is "On Desktop"
				RECT windowRect = {wp->x, wp->y, (wp->x + m_WindowW), (wp->y + m_WindowH)};
				HMONITOR hMonitor = MonitorFromRect(&windowRect, MONITOR_DEFAULTTONULL);

				if(hMonitor != NULL)
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
				std::map<std::wstring, CMeterWindow*>& windows = Rainmeter->GetAllMeterWindows();
				std::map<std::wstring, CMeterWindow*>::iterator iter = windows.begin();
				for( ; iter != windows.end(); iter++)
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

		// Convert the screen coordinates to the client coordinates of the shell window
		if (!m_ChildWindow && !m_NativeTransparency && m_WindowZPosition == ZPOSITION_ONDESKTOP)
		{
			POINT pos = {wp->x, wp->y};
			if (ScreenToClient(GetAncestor(m_Window, GA_PARENT), &pos))
			{
				wp->x = pos.x;
				wp->y = pos.y;
			}
		}
	}

	return DefWindowProc(m_Window, m_Message, wParam, lParam);
}

void CMeterWindow::SnapToWindow(CMeterWindow* window, LPWINDOWPOS wp)
{
	int x = window->m_ScreenX;
	int y = window->m_ScreenY;
	int w = window->m_WindowW;
	int h = window->m_WindowH;

	if (wp->y < y + h && wp->y + m_WindowH > y)
	{
		if((wp->x < SNAPDISTANCE + x) && (wp->x > x - SNAPDISTANCE)) wp->x = x;
		if((wp->x < SNAPDISTANCE + x + w) && (wp->x > x + w - SNAPDISTANCE)) wp->x = x + w;

		if((wp->x + m_WindowW < SNAPDISTANCE + x) && (wp->x + m_WindowW > x - SNAPDISTANCE)) wp->x = x - m_WindowW;
		if((wp->x + m_WindowW < SNAPDISTANCE + x + w) && (wp->x + m_WindowW > x + w - SNAPDISTANCE)) wp->x = x + w - m_WindowW;
	}

	if (wp->x < x + w && wp->x + m_WindowW > x)
	{
		if((wp->y < SNAPDISTANCE + y) && (wp->y > y - SNAPDISTANCE)) wp->y = y;
		if((wp->y < SNAPDISTANCE + y + h) && (wp->y > y + h - SNAPDISTANCE)) wp->y = y + h;

		if((wp->y + m_WindowH < SNAPDISTANCE + y) && (wp->y + m_WindowH > y - SNAPDISTANCE)) wp->y = y - m_WindowH;
		if((wp->y + m_WindowH < SNAPDISTANCE + y + h) && (wp->y + m_WindowH > y + h - SNAPDISTANCE)) wp->y = y + h - m_WindowH;
	}
}

/*
** OnDestroy
**
** During destruction of the window do nothing.
**
*/
LRESULT CMeterWindow::OnDestroy(WPARAM wParam, LPARAM lParam) 
{
	return 0;
}

/*
** OnDisplayChange
**
** During resolution changes do nothing.
** (OnDelayedMove function is used instead.)
**
*/
LRESULT CMeterWindow::OnDisplayChange(WPARAM wParam, LPARAM lParam) 
{
	return 0;
}

/*
** OnSettingChange
**
** During setting changes do nothing.
** (OnDelayedMove function is used instead.)
**
*/
LRESULT CMeterWindow::OnSettingChange(WPARAM wParam, LPARAM lParam) 
{
	return 0;
}

/*
** OnLeftButtonDown
**
** Runs the action when left mouse button is down
**
*/
LRESULT CMeterWindow::OnLeftButtonDown(WPARAM wParam, LPARAM lParam) 
{
	POINT pos;
	pos.x = (SHORT)LOWORD(lParam); 
	pos.y = (SHORT)HIWORD(lParam); 

	if (m_Message == WM_NCLBUTTONDOWN)
	{
		// Transform the point to client rect
		RECT rect;
		GetWindowRect(m_Window, &rect);
		pos.x = pos.x - rect.left;
		pos.y = pos.y - rect.top;
	}

	// Handle buttons
	bool redraw = false;
	std::list<CMeter*>::iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); j++)
	{
		// Hidden meters are ignored
		if ((*j)->IsHidden()) continue;

		CMeterButton* button = dynamic_cast<CMeterButton*>(*j);
		if (button)
		{
			redraw |= button->MouseDown(pos);
		}

	}
	if (redraw)
	{
		Redraw();
	}
	else if(!DoAction(pos.x, pos.y, MOUSE_LMB_DOWN, false) && m_WindowDraggable)
	{
		// Run the DefWindowProc so the dragging works
		return DefWindowProc(m_Window, m_Message, wParam, lParam);
	}

	return 0;
}

/*
** OnLeftButtonUp
**
** Runs the action when left mouse button is up
**
*/
LRESULT CMeterWindow::OnLeftButtonUp(WPARAM wParam, LPARAM lParam) 
{
	POINT pos;
	pos.x = (SHORT)LOWORD(lParam); 
	pos.y = (SHORT)HIWORD(lParam); 

	if (m_Message == WM_NCLBUTTONUP)
	{
		// Transform the point to client rect
		RECT rect;
		GetWindowRect(m_Window, &rect);
		pos.x = pos.x - rect.left;
		pos.y = pos.y - rect.top;
	}

	// Handle buttons
	bool redraw = false;
	std::list<CMeter*>::iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); j++)
	{
		// Hidden meters are ignored
		if ((*j)->IsHidden()) continue;

		CMeterButton* button = dynamic_cast<CMeterButton*>(*j);
		if (button)
		{
			redraw |= button->MouseUp(pos, this);
		}
	}
	if (redraw)
	{
		Redraw();
	}

	DoAction(pos.x, pos.y, MOUSE_LMB_UP, false);

	return 0;
}

/*
** OnLeftButtonDoubleClick
**
** Runs the action when left mouse button is double-clicked
**
*/
LRESULT CMeterWindow::OnLeftButtonDoubleClick(WPARAM wParam, LPARAM lParam) 
{
	POINT pos;
	pos.x = (SHORT)LOWORD(lParam); 
	pos.y = (SHORT)HIWORD(lParam); 

	if (m_Message == WM_NCLBUTTONDBLCLK)
	{
		// Transform the point to client rect
		RECT rect;
		GetWindowRect(m_Window, &rect);
		pos.x = pos.x - rect.left;
		pos.y = pos.y - rect.top;
	}

	// Handle buttons
	bool redraw = false;
	std::list<CMeter*>::iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); j++)
	{
		// Hidden meters are ignored
		if ((*j)->IsHidden()) continue;

		CMeterButton* button = dynamic_cast<CMeterButton*>(*j);
		if (button)
		{
			redraw |= button->MouseDown(pos);
		}

	}
	if (redraw)
	{
		Redraw();
	}
	else if (!DoAction(pos.x, pos.y, MOUSE_LMB_DBLCLK, false))
	{
		DoAction(pos.x, pos.y, MOUSE_LMB_DOWN, false);
	}

	return 0;
}

/*
** OnRightButtonDown
**
** Runs the action when right mouse button is down
**
*/
LRESULT CMeterWindow::OnRightButtonDown(WPARAM wParam, LPARAM lParam) 
{
	POINT pos;
	pos.x = (SHORT)LOWORD(lParam); 
	pos.y = (SHORT)HIWORD(lParam); 

	if (m_Message == WM_NCRBUTTONDOWN)
	{
		// Transform the point to client rect
		RECT rect;
		GetWindowRect(m_Window, &rect);
		pos.x = pos.x - rect.left;
		pos.y = pos.y - rect.top;
	}

	DoAction(pos.x, pos.y, MOUSE_RMB_DOWN, false);

	return 0;
}

/*
** OnRightButtonUp
**
** Runs the action when right mouse button is up
**
*/
LRESULT CMeterWindow::OnRightButtonUp(WPARAM wParam, LPARAM lParam) 
{
	if (!DoAction((SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam), MOUSE_RMB_UP, false))
	{
		// Run the DefWindowProc so the context menu works
		return DefWindowProc(m_Window, WM_RBUTTONUP, wParam, lParam);
	}

	return 0;
}

/*
** OnRightButtonDoubleClick
**
** Runs the action when right mouse button is double-clicked
**
*/
LRESULT CMeterWindow::OnRightButtonDoubleClick(WPARAM wParam, LPARAM lParam) 
{
	POINT pos;
	pos.x = (SHORT)LOWORD(lParam); 
	pos.y = (SHORT)HIWORD(lParam); 

	if (m_Message == WM_NCRBUTTONDBLCLK)
	{
		// Transform the point to client rect
		RECT rect;
		GetWindowRect(m_Window, &rect);
		pos.x = pos.x - rect.left;
		pos.y = pos.y - rect.top;
	}

	if (!DoAction(pos.x, pos.y, MOUSE_RMB_DBLCLK, false))
	{
		DoAction(pos.x, pos.y, MOUSE_RMB_DOWN, false);
	}

	return 0;
}

/*
** OnMiddleButtonDown
**
** Runs the action when middle mouse button is down
**
*/
LRESULT CMeterWindow::OnMiddleButtonDown(WPARAM wParam, LPARAM lParam) 
{
	POINT pos;
	pos.x = (SHORT)LOWORD(lParam); 
	pos.y = (SHORT)HIWORD(lParam); 

	if (m_Message == WM_NCMBUTTONDOWN)
	{
		// Transform the point to client rect
		RECT rect;
		GetWindowRect(m_Window, &rect);
		pos.x = pos.x - rect.left;
		pos.y = pos.y - rect.top;
	}

	DoAction(pos.x, pos.y, MOUSE_MMB_DOWN, false);

	return 0;
}

/*
** OnMiddleButtonUp
**
** Runs the action when middle mouse button is up
**
*/
LRESULT CMeterWindow::OnMiddleButtonUp(WPARAM wParam, LPARAM lParam) 
{
	POINT pos;
	pos.x = (SHORT)LOWORD(lParam); 
	pos.y = (SHORT)HIWORD(lParam); 

	if (m_Message == WM_NCMBUTTONUP)
	{
		// Transform the point to client rect
		RECT rect;
		GetWindowRect(m_Window, &rect);
		pos.x = pos.x - rect.left;
		pos.y = pos.y - rect.top;
	}

	DoAction(pos.x, pos.y, MOUSE_MMB_UP, false);

	return 0;
}

/*
** OnMiddleButtonDoubleClick
**
** Runs the action when middle mouse button is double-clicked
**
*/
LRESULT CMeterWindow::OnMiddleButtonDoubleClick(WPARAM wParam, LPARAM lParam) 
{
	POINT pos;
	pos.x = (SHORT)LOWORD(lParam); 
	pos.y = (SHORT)HIWORD(lParam); 

	if (m_Message == WM_NCMBUTTONDBLCLK)
	{
		// Transform the point to client rect
		RECT rect;
		GetWindowRect(m_Window, &rect);
		pos.x = pos.x - rect.left;
		pos.y = pos.y - rect.top;
	}

	if (!DoAction(pos.x, pos.y, MOUSE_MMB_DBLCLK, false))
	{
		DoAction(pos.x, pos.y, MOUSE_MMB_DOWN, false);
	}

	return 0;
}

/*
** OnContextMenu
**
** Handles the context menu. The menu is recreated every time it is shown.
**
*/
LRESULT CMeterWindow::OnContextMenu(WPARAM wParam, LPARAM lParam) 
{
	int xPos = (SHORT)LOWORD(lParam); 
	int yPos = (SHORT)HIWORD(lParam); 

	// Transform the point to client rect
	int x = (INT)(SHORT)LOWORD(lParam); 
	int y = (INT)(SHORT)HIWORD(lParam); 
	RECT rect;
	GetWindowRect(m_Window, &rect);
	x = x - rect.left;
	y = y - rect.top;

	// If RMB up or RMB down or double-click cause actions, do not show the menu!
	if (DoAction(x, y, MOUSE_RMB_UP, false) || DoAction(x, y, MOUSE_RMB_DOWN, true) || DoAction(x, y, MOUSE_RMB_DBLCLK, true))
	{
		return 0;
	}

	POINT pos;
	pos.x = xPos;
	pos.y = yPos;
	m_Rainmeter->ShowContextMenu(pos, this);

	return 0;
}

/*
** DoAction
**
** Executes the action if such are defined. Returns true, if action was executed.
** If the test is true, the action is not executed.
**
*/
bool CMeterWindow::DoAction(int x, int y, MOUSE mouse, bool test) 
{
	// Check if the hitpoint was over some meter
	std::list<CMeter*>::iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); j++)
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

			case MOUSE_OVER:
				if (!(*j)->IsMouseOver())
				{
					(*j)->SetMouseOver(true);

					if (!((*j)->GetMouseOverAction().empty()))
					{
						m_MouseOver = true;		// If the mouse is over a meter it's also over the main window
						if (!test) m_Rainmeter->ExecuteCommand((*j)->GetMouseOverAction().c_str(), this);
						return true;
					}
				}
				break;
			}
		}
		else
		{
			if (mouse == MOUSE_LEAVE || mouse == MOUSE_OVER)
			{
				if ((*j)->IsMouseOver())
				{
					(*j)->SetMouseOver(false);

					if (!((*j)->GetMouseLeaveAction().empty()))
					{
						if (!test) m_Rainmeter->ExecuteCommand((*j)->GetMouseLeaveAction().c_str(), this);
						return true;
					}
				}
			}
		}
	}

	bool inside = false;
	if (x >= 0 && y >= 0 && x < m_WindowW && y < m_WindowH)
	{
		// Check transparent pixels
		if (m_DoubleBuffer)
		{
			Color color;
			m_DoubleBuffer->GetPixel(x, y, &color);
			if (color.GetA() != 0)
			{
				inside = true;
			}
		}
		else
		{
			inside = true;
		}
	}

	if (inside)
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

		case MOUSE_OVER:
			if (!m_MouseOver)
			{
				m_MouseOver = true;
				if (!m_MouseOverAction.empty())
				{
					if (!test) m_Rainmeter->ExecuteCommand(m_MouseOverAction.c_str(), this);
					return true;
				}
			}
			break;
		}
	}
	else
	{
		// Mouse leave happens when the mouse is outside the window
		if (mouse == MOUSE_LEAVE)
		{
			if (m_MouseOver)
			{
				m_MouseOver = false;
				if (!m_MouseLeaveAction.empty())
				{
					if (!test) m_Rainmeter->ExecuteCommand(m_MouseLeaveAction.c_str(), this);
					return true;
				}
			}
		}
	}

	return false;
}


/*
** OnMove
**
** Stores the new place of the window, in screen coordinates.
**
*/
LRESULT CMeterWindow::OnMove(WPARAM wParam, LPARAM lParam) 
{
	// The lParam's x/y parameters are given in screen coordinates for overlapped and pop-up windows
	// and in parent-client coordinates for child windows.

	// Store the new window position
	m_ScreenX = (SHORT)LOWORD(lParam);
	m_ScreenY = (SHORT)HIWORD(lParam);

	// Convert the client coordinates of the shell window to the screen coordinates
	if (!m_ChildWindow && !m_NativeTransparency && m_WindowZPosition == ZPOSITION_ONDESKTOP)
	{
		POINT pos = {m_ScreenX, m_ScreenY};
		if (ClientToScreen(GetAncestor(m_Window, GA_PARENT), &pos))
		{
			m_ScreenX = pos.x;
			m_ScreenY = pos.y;
		}
	}

	if (m_Dragging)
	{
		ScreenToWindow();
	}

	// Redraw itself if the window is "On Desktop"
	if (m_WindowZPosition == ZPOSITION_ONDESKTOP)
	{
		UpdateTransparency(m_TransparencyValue, false);
	}

	return 0;
}

/* 
** WndProc
**
** The window procedure for the Meter
**
*/
LRESULT CALLBACK CMeterWindow::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CMeterWindow* Window = NULL;

	if(uMsg == WM_CREATE) 
	{
		// Fetch this window-object from the CreateStruct
		Window=(CMeterWindow*)((LPCREATESTRUCT)lParam)->lpCreateParams;

		SetProp(hWnd, L"RAINMETER", Window);
	}
	else if(uMsg == WM_DESTROY) 
	{
		RemoveProp(hWnd, L"RAINMETER");
	}
	else
	{
		Window = (CMeterWindow*)GetProp(hWnd, L"RAINMETER");
	}

	if (Window) Window->m_Message = uMsg;

	BEGIN_MESSAGEPROC
	MESSAGE(OnPaint, WM_PAINT)
	MESSAGE(OnMove, WM_MOVE)
	MESSAGE(OnCreate, WM_CREATE)
	MESSAGE(OnDestroy, WM_DESTROY)
	MESSAGE(OnTimer, WM_TIMER)
	MESSAGE(OnCommand, WM_COMMAND)
	MESSAGE(OnSysCommand, WM_SYSCOMMAND)
	MESSAGE(OnEnterSizeMove, WM_ENTERSIZEMOVE)
	MESSAGE(OnExitSizeMove, WM_EXITSIZEMOVE)
	MESSAGE(OnNcHitTest, WM_NCHITTEST)
	MESSAGE(OnMouseMove, WM_MOUSEMOVE)
	MESSAGE(OnMouseMove, WM_NCMOUSEMOVE)
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
	MESSAGE(OnDelayedExecute, WM_DELAYED_EXECUTE)
	MESSAGE(OnDelayedRefresh, WM_DELAYED_REFRESH)
	MESSAGE(OnDelayedMove, WM_DELAYED_MOVE)
	MESSAGE(OnDelayedQuit, WM_DELAYED_QUIT)
	MESSAGE(OnSettingChange, WM_SETTINGCHANGE)
	MESSAGE(OnDisplayChange, WM_DISPLAYCHANGE)
	END_MESSAGEPROC
}

/*
** OnDelayedExecute
**
** Handles delayed executes
**
*/
LRESULT CMeterWindow::OnDelayedExecute(WPARAM wParam, LPARAM lParam)
{
	if (lParam)
	{
		LPCTSTR szExecute = (LPCTSTR)lParam;
		COPYDATASTRUCT copyData;

		copyData.dwData = 1;
		copyData.cbData = (DWORD)((lstrlen(szExecute))* sizeof(WCHAR));
		copyData.lpData = (void*)szExecute;

		OnCopyData(NULL, (LPARAM)&copyData);
	}
	return 0;
}

/*
** OnDelayedRefresh
**
** Handles delayed refresh
**
*/
LRESULT CMeterWindow::OnDelayedRefresh(WPARAM wParam, LPARAM lParam)
{
	try
	{
		Refresh(false);
	}
	catch (CError& error)
	{
		MessageBox(m_Window, error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
	}
	return 0;
}

/*
** OnDelayedMove
**
** Handles delayed move
**
*/
LRESULT CMeterWindow::OnDelayedMove(WPARAM wParam, LPARAM lParam) 
{
	if (m_NativeTransparency)
	{
		m_Parser.ResetVariables(m_Rainmeter, this);

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
		PostMessage(m_Window, WM_DELAYED_REFRESH, (WPARAM)NULL, (LPARAM)NULL);
	}

	return 0;
}

/*
** OnDelayedQuit
**
** Handles delayed quit
**
*/
LRESULT CMeterWindow::OnDelayedQuit(WPARAM wParam, LPARAM lParam)
{
	if (Rainmeter->GetDummyLitestep()) PostQuitMessage(0);
	quitModule(Rainmeter->GetInstance());
	return 0;
}

/*
** OnCopyData
**
** Handles bangs from the exe
**
*/
LRESULT CMeterWindow::OnCopyData(WPARAM wParam, LPARAM lParam)
{
	COPYDATASTRUCT* pCopyDataStruct = (COPYDATASTRUCT*) lParam;

	if (pCopyDataStruct && (pCopyDataStruct->dwData == 1) && (pCopyDataStruct->cbData > 0))
	{
		// Check that we're still alive
		bool found = false;
		std::map<std::wstring, CMeterWindow*>& meters = Rainmeter->GetAllMeterWindows();
		std::map<std::wstring, CMeterWindow*>::iterator iter = meters.begin();

		for ( ; iter != meters.end(); iter++)
		{
			if ((*iter).second == this)
			{
				found = true;
				break;
			}
		}
		if (!found) 
		{
			DebugLog(L"Unable to send the !bang to a deactivated config.");
			return 0;	// This meterwindow has been deactivated
		}

		std::wstring str = (const WCHAR*)pCopyDataStruct->lpData;
		std::wstring bang;
		std::wstring arg;

		// Find the first space
		std::wstring::size_type pos = str.find(' ');
		if (pos != std::wstring::npos)
		{
			bang = str.substr(0, pos);
			str.erase(0, pos + 1);
			arg = str;
		}
		else
		{
			bang = str;
		}

		// Add the current config name to the args. If it's not defined already
		// the bang only affects this config, if there already is a config defined
		// another one doesn't matter.
		arg += L" \"";
		arg += m_SkinName.c_str();
		arg += L"\"";

		return Rainmeter->ExecuteBang(bang, arg, this);
	}
	else
	{
		return FALSE;
	}
}

/*
** MakePathAbsolute
**
** Converts the path to absolute bu adding the skin's path to it (unless it already is absolute).
**
*/
std::wstring CMeterWindow::MakePathAbsolute(std::wstring path)
{
	if (path.empty() || path.find(L':') != std::wstring::npos)
	{
		return path;	// It's already absolute path (or it's empty)
	}

	std::wstring root = m_SkinPath + m_SkinName;

	if (root[root.length() - 1] != L'\\')
	{
		root += L"\\";
	}

	return root + path;
}
