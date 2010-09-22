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
#include "MeterWindow.h"
#include "Rainmeter.h"
#include "System.h"
#include "Error.h"
#include "Meter.h"
#include "Measure.h"
#include "AboutDialog.h"
#include "resource.h"
#include "Litestep.h"
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
	m_HasNetMeasures = false;
	m_HasButtons = false;
	m_WindowHide = HIDEMODE_NONE;
	m_WindowStartHidden = false;
	m_SnapEdges = true;
	m_Rainmeter = NULL;
	m_Hidden = false;
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

	m_User32Library = GetModuleHandle(L"user32.dll");

	m_UpdateCounter = 0;
	m_FontCollection = NULL;

	++c_InstanceCount;
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

	// Kill the timer
	KillTimer(m_Window, METERTIMER);
	KillTimer(m_Window, MOUSETIMER);
	KillTimer(m_Window, FADETIMER);
	KillTimer(m_Window, TRANSITIONTIMER);

	// Destroy the meters
	std::list<CMeter*>::iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); ++j)
	{
		delete (*j);
	}

	// Destroy the measures
	std::list<CMeasure*>::iterator i = m_Measures.begin();
	for( ; i != m_Measures.end(); ++i)
	{
		delete (*i);
	}

	if(m_Background) delete m_Background;
	if(m_DoubleBuffer) delete (m_DoubleBuffer);

	if(m_Window) DestroyWindow(m_Window);

	if(m_FontCollection) delete m_FontCollection;

	--c_InstanceCount;

	if (c_InstanceCount == 0)
	{
		BOOL Result;
		int counter = 0;
		do {
			// Wait for the window to die
			Result = UnregisterClass(METERWINDOW_CLASS_NAME, m_Rainmeter->GetInstance());
			Sleep(100);
			++counter;
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
	wc.lpszClassName = METERWINDOW_CLASS_NAME;
	
	if(!RegisterClassEx(&wc))
	{
		DWORD err = GetLastError();

		if (err != 0 && ERROR_CLASS_ALREADY_EXISTS != err)
		{
			throw CError(CError::ERROR_REGISTER_WINDOWCLASS, __LINE__, __FILE__);
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

	if(m_Window == NULL) 
	{ 
		throw CError(CError::ERROR_CREATE_WINDOW, __LINE__, __FILE__);
	}

#ifndef _WIN64
	SetWindowLong(m_Window, GWL_USERDATA, magicDWord);
#endif

	setlocale(LC_NUMERIC, "C");

	// Mark the window to ignore the Aero peek
	IgnoreAeroPeek();

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
** IgnoreAeroPeek
**
** Excludes this window from the Aero Peek.
**
*/
void CMeterWindow::IgnoreAeroPeek()
{
	typedef HRESULT (WINAPI * FPDWMSETWINDOWATTRIBUTE)(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
	#define DWMWA_EXCLUDED_FROM_PEEK 12

	if (CSystem::GetOSPlatform() >= OSPLATFORM_VISTA)
	{
		HINSTANCE h = CSystem::RmLoadLibrary(L"dwmapi.dll");
		if (h)
		{
			FPDWMSETWINDOWATTRIBUTE DwmSetWindowAttribute = (FPDWMSETWINDOWATTRIBUTE)GetProcAddress(h, "DwmSetWindowAttribute");
			if (DwmSetWindowAttribute)
			{
				BOOL bValue = TRUE;
				DwmSetWindowAttribute(m_Window, DWMWA_EXCLUDED_FROM_PEEK, &bValue, sizeof(bValue));
			}
			FreeLibrary(h);
		}
	}
}

/*
** Refresh
**
** This deletes everything and rebuilds the config again.
**
*/
void CMeterWindow::Refresh(bool init, bool all)
{
	assert(m_Rainmeter != NULL);

	m_Rainmeter->SetCurrentParser(&m_Parser);

	std::wstring dbg = L"Refreshing skin \"" + m_SkinName;
	dbg += L"\\" + m_SkinIniFile; 
	dbg += L"\""; 
	LSLog(LOG_DEBUG, L"Rainmeter", dbg.c_str());
	
	m_Refreshing = true;

	if(!init)
	{
		// First destroy everything
		// WriteConfig(); //Not clear why this is needed and it messes up resolution changes

		// Kill the timer
		KillTimer(m_Window, METERTIMER);
		KillTimer(m_Window, MOUSETIMER);
		KillTimer(m_Window, FADETIMER);
		KillTimer(m_Window, TRANSITIONTIMER);

		m_MouseOver = false;
		SetMouseLeaveEvent(true);

		std::list<CMeasure*>::iterator i = m_Measures.begin();
		for( ; i != m_Measures.end(); ++i)
		{
			delete (*i);
		}
		m_Measures.clear();

		std::list<CMeter*>::iterator j = m_Meters.begin();
		for( ; j != m_Meters.end(); ++j)
		{
			if ((*j)->GetToolTipHandle() != NULL)
			{
				DestroyWindow((*j)->GetToolTipHandle());
				(*j)->SetToolTipHandle(NULL);
			}
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
	Update(false);
	CreateRegion(false);

	if (m_KeepOnScreen) 
	{
		MapCoordsToScreen(m_ScreenX, m_ScreenY, m_WindowW, m_WindowH);
	}

	SetWindowPos(m_Window, NULL, m_ScreenX, m_ScreenY, m_WindowW, m_WindowH, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

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

	if (init || all || oldZPos != m_WindowZPosition)
	{
		ChangeZPos(m_WindowZPosition, init || all);
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
	HMONITOR hMonitor;
	MONITORINFO mi;

	POINT pt = {x, y};
	for (int i = 0; i < 5; ++i) 
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
** MoveWindow
**
** Moves the window to a new place (on the virtual screen)
**
*/
void CMeterWindow::MoveWindow(int x, int y)
{
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
void CMeterWindow::ChangeZPos(ZPOSITION zPos, bool all)
{
	if(!m_ChildWindow)
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
					// Insert after the tray window temporarily to keep order
					winPos = m_Rainmeter->GetTrayWindow()->GetWindow();
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

		case ZPOSITION_ONDESKTOP:
			if (CSystem::GetShowDesktop())
			{
				// Set WS_EX_TOPMOST flag
				SetWindowPos(m_Window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

				if (all)
				{
					// Insert after the helper window
					SetWindowPos(m_Window, CSystem::GetHelperWindow(), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
				}
				else
				{
					// Find the "backmost" topmost window
					HWND hwnd = CSystem::GetHelperWindow();
					while (hwnd = ::GetNextWindow(hwnd, GW_HWNDPREV))
					{
						if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
						{
							// Insert after the found window
							if (0 != SetWindowPos(m_Window, hwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING))
							{
								break;
							}
						}
					}
				}
				return;
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

		SetWindowPos(m_Window, winPos, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
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

	case BANG_TOGGLEMETERGROUP:
		ToggleMeter(arg, true);
		break;

	case BANG_SHOWMETERGROUP:
		ShowMeter(arg, true);
		break;

	case BANG_HIDEMETERGROUP:
		HideMeter(arg, true);
		break;

	case BANG_TOGGLEMEASURE:
		ToggleMeasure(arg);
		break;

	case BANG_ENABLEMEASURE:
		EnableMeasure(arg);
		break;

	case BANG_DISABLEMEASUREGROUP:
		DisableMeasure(arg, true);
		break;

	case BANG_TOGGLEMEASUREGROUP:
		ToggleMeasure(arg, true);
		break;

	case BANG_ENABLEMEASUREGROUP:
		EnableMeasure(arg, true);
		break;

	case BANG_DISABLEMEASURE:
		DisableMeasure(arg);
		break;

	case BANG_SHOW:
		m_Hidden = false;
		ShowWindow(m_Window, SW_SHOWNOACTIVATE);
		if (m_WindowHide == HIDEMODE_FADEOUT)
		{
			UpdateTransparency(255, false);
		}
		else
		{
			UpdateTransparency(m_AlphaValue, false);
		}
		break;

	case BANG_HIDE:
		m_Hidden = true;
		ShowWindow(m_Window, SW_HIDE);
		break;

	case BANG_TOGGLE:
		RunBang(m_Hidden ? BANG_SHOW : BANG_HIDE, arg);
		break;

	case BANG_SHOWFADE:
		m_Hidden = false;
		if (!IsWindowVisible(m_Window))
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
		break;

	case BANG_HIDEFADE:
		m_Hidden = true;
		if (IsWindowVisible(m_Window))
		{
			FadeWindow(m_AlphaValue, 0);
		}
		break;

	case BANG_TOGGLEFADE:
		RunBang(m_Hidden ? BANG_SHOWFADE : BANG_HIDEFADE, arg);
		break;

	case BANG_MOVE:
		pos = wcschr(arg, ' ');
		if (pos != NULL)
		{
			MoveWindow(_wtoi(arg), _wtoi(pos));
		}
		else
		{
			DebugLog(L"Unable to parse parameters for !RainmeterMove");
		}
		break;

	case BANG_ZPOS:
		ChangeZPos((ZPOSITION)_wtoi(arg));
		break;

	case BANG_CLICKTHROUGH:
		{
			int f = _wtoi(arg);
			if (f == -1)
			{
				SetClickThrough(!m_ClickThrough);
			}
			else
			{
				SetClickThrough(f != 0);
			}
		}
		break;

	case BANG_DRAGGABLE:
		{
			int f = _wtoi(arg);
			if (f == -1)
			{
				SetWindowDraggable(!m_WindowDraggable);
			}
			else
			{
				SetWindowDraggable(f != 0);
			}
		}
		break;

	case BANG_SNAPEDGES:
		{
			int f = _wtoi(arg);
			if (f == -1)
			{
				SetSnapEdges(!m_SnapEdges);
			}
			else
			{
				SetSnapEdges(f != 0);
			}
		}
		break;

	case BANG_KEEPONSCREEN:
		{
			int f = _wtoi(arg);
			if (f == -1)
			{
				SetKeepOnScreen(!m_KeepOnScreen);
			}
			else
			{
				SetKeepOnScreen(f != 0);
			}
		}
		break;

	case BANG_SETTRANSPARENCY:
		if (arg != NULL)
		{
			int alpha = (int)CConfigParser::ParseDouble(arg, 255, true);
			alpha = max(alpha, 0);
			m_AlphaValue = min(alpha, 255);
			UpdateTransparency(m_AlphaValue, false);
		}
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
				DebugLog(L"Unable to parse parameters for !RainmeterLsBoxHook (%s)", arg);
			}
		}
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
				DebugLog(L"Unable to parse coordinates for !RainmeterMoveMeter");
			}
		}
		else
		{
			DebugLog(L"Unable to parse parameters for !RainmeterMoveMeter");
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
				std::list<CMeasure*>::const_iterator iter = m_Measures.begin();
				for( ; iter != m_Measures.end(); ++iter)
				{
					if (_wcsicmp((*iter)->GetName(), measure.c_str()) == 0)
					{
						(*iter)->ExecuteBang(args.c_str());
						return;
					}
				}

				DebugLog(L"Unable to find [%s] for !RainmeterPluginBang", measure.c_str());
			}
			else
			{
				DebugLog(L"Unable to parse parameters for !RainmeterPluginBang");
			}
		}
		break;

	case BANG_SETVARIABLE:
		pos = wcschr(arg, ' ');
		if (pos != NULL)
		{
			std::wstring strVariable(arg, pos - arg);
			std::wstring strValue(pos + 1);
			double value;
			int result = m_Parser.ReadFormula(strValue, &value);
			
			// Formula read fine
			if(result != -1)
			{
				TCHAR buffer[256];
				swprintf(buffer, L"%f", value);

				const std::wstring& resultString = buffer;

				m_Parser.SetVariable(strVariable, resultString);
			}
			else
			{
				m_Parser.SetVariable(strVariable, strValue);
			}
		}
		else
		{
			DebugLog(L"Unable to parse parameters for !RainmeterSetVariable");
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
void CMeterWindow::ShowMeter(const WCHAR* name, bool group)
{
	if (name == NULL || wcslen(name) == 0) return;

	std::list<CMeter*>::const_iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); ++j)
	{
		if (group)
		{
			if (!(*j)->BelongsToGroup(name)) continue;
		}
		else
		{
			if (_wcsicmp((*j)->GetName(), name) != 0) continue;
		}

		(*j)->Show();
		if ((*j)->GetToolTipHandle() != NULL)
		{
			SendMessage((*j)->GetToolTipHandle(), TTM_ACTIVATE, TRUE, NULL);
		}
		m_ResetRegion = true;	// Need to recalculate the window region
		if (!group) return;
	}

	if (!group) DebugLog(L"Unable to show the meter %s (there is no such thing in %s)", name, m_SkinName.c_str());
}

/*
** HideMeter
**
** Hides the given meter
**
*/
void CMeterWindow::HideMeter(const WCHAR* name, bool group)
{
	if (name == NULL || wcslen(name) == 0) return;

	std::list<CMeter*>::const_iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); ++j)
	{
		if (group)
		{
			if (!(*j)->BelongsToGroup(name)) continue;
		}
		else
		{
			if (_wcsicmp((*j)->GetName(), name) != 0) continue;
		}

		(*j)->Hide();
		if ((*j)->GetToolTipHandle() != NULL)
		{
			SendMessage((*j)->GetToolTipHandle(), TTM_ACTIVATE, FALSE, NULL);
		}
		m_ResetRegion = true;	// Need to recalculate the windowregion
		if (!group) return;
	}

	if (!group) DebugLog(L"Unable to hide the meter %s (there is no such thing in %s)", name, m_SkinName.c_str());
}

/*
** ToggleMeter
**
** Toggles the given meter
**
*/
void CMeterWindow::ToggleMeter(const WCHAR* name, bool group)
{
	if (name == NULL || wcslen(name) == 0) return;

	std::list<CMeter*>::const_iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); ++j)
	{
		if (group)
		{
			if (!(*j)->BelongsToGroup(name)) continue;
		}
		else
		{
			if (_wcsicmp((*j)->GetName(), name) != 0) continue;
		}

		if ((*j)->IsHidden())
		{
			(*j)->Show();
			if ((*j)->GetToolTipHandle() != NULL)
			{
				SendMessage((*j)->GetToolTipHandle(), TTM_ACTIVATE, TRUE, NULL);
			}
		}
		else
		{
			(*j)->Hide();
			if ((*j)->GetToolTipHandle() != NULL)
			{
				SendMessage((*j)->GetToolTipHandle(), TTM_ACTIVATE, FALSE, NULL);
			}
		}
		m_ResetRegion = true;	// Need to recalculate the window region
		if (!group) return;
	}

	if (!group) DebugLog(L"Unable to toggle the meter %s (there is no such thing in %s)", name, m_SkinName.c_str());
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

	std::list<CMeter*>::const_iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); ++j)
	{
		if (_wcsicmp((*j)->GetName(), name) == 0)
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
void CMeterWindow::EnableMeasure(const WCHAR* name, bool group)
{
	if (name == NULL || wcslen(name) == 0) return;

	std::list<CMeasure*>::const_iterator i = m_Measures.begin();
	for( ; i != m_Measures.end(); ++i)
	{
		if (group)
		{
			if (!(*i)->BelongsToGroup(name)) continue;
		}
		else
		{
			if (_wcsicmp((*i)->GetName(), name) != 0) continue;
		}

		(*i)->Enable();
		if (!group) return;
	}

	if (!group) DebugLog(L"Unable to enable the measure %s (there is no such thing in %s)", name, m_SkinName.c_str());
}


/*
** DisableMeasure
**
** Disables the given measure
**
*/
void CMeterWindow::DisableMeasure(const WCHAR* name, bool group)
{
	if (name == NULL || wcslen(name) == 0) return;

	std::list<CMeasure*>::const_iterator i = m_Measures.begin();
	for( ; i != m_Measures.end(); ++i)
	{
		if (group)
		{
			if (!(*i)->BelongsToGroup(name)) continue;
		}
		else
		{
			if (_wcsicmp((*i)->GetName(), name) != 0) continue;
		}

		(*i)->Disable();
		if (!group) return;
	}

	if (!group) DebugLog(L"Unable to disable the measure %s (there is no such thing in %s)", name, m_SkinName.c_str());
}


/*
** ToggleMeasure
**
** Toggless the given measure
**
*/
void CMeterWindow::ToggleMeasure(const WCHAR* name, bool group)
{
	if (name == NULL || wcslen(name) == 0) return;

	std::list<CMeasure*>::const_iterator i = m_Measures.begin();
	for( ; i != m_Measures.end(); ++i)
	{
		if (group)
		{
			if (!(*i)->BelongsToGroup(name)) continue;
		}
		else
		{
			if (_wcsicmp((*i)->GetName(), name) != 0) continue;
		}

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

	if (!group) DebugLog(L"Unable to toggle the measure %s (there is no such thing in %s)", name, m_SkinName.c_str());
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
		DebugLog(L"There are no monitors. WindowToScreen function fails.");
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
			for (size_t i = 0; i < monitors.size(); ++i)
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

	// Reset settings to the default value
	m_WindowX = L"0";
	m_WindowY = L"0";
	m_AnchorX = L"0";
	m_AnchorY = L"0";
	m_WindowZPosition = ZPOSITION_NORMAL;
	m_WindowDraggable = true;
	m_WindowHide = HIDEMODE_NONE;
	m_WindowStartHidden = false;
	m_SavePosition = true;
	m_SnapEdges = true;
	m_MeasuresToVariables = false;
	m_NativeTransparency = true;
	m_ClickThrough = false;
	m_KeepOnScreen = true;
	m_AutoSelectScreen = false;
	m_AlphaValue = 255;
	m_FadeDuration = 250;

	std::wstring group = L"";

	CConfigParser parser;
	parser.Initialize(iniFile.c_str(), m_Rainmeter);

	for (int i = 0; i < 2; ++i)
	{
		m_WindowX = parser.ReadString(section, L"WindowX", m_WindowX.c_str());
		m_WindowY = parser.ReadString(section, L"WindowY", m_WindowY.c_str());
		m_AnchorX = parser.ReadString(section, L"AnchorX", m_AnchorX.c_str());
		m_AnchorY = parser.ReadString(section, L"AnchorY", m_AnchorY.c_str());

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
			double value = parser.ReadFormula(section, L"WindowX", 0.0);
			WCHAR buffer[32];
			swprintf(buffer, L"%i", (int)value);
			m_WindowX = buffer;
		}
		if (!m_WindowY.empty() && m_WindowY[0] == L'(' && m_WindowY[m_WindowY.size() - 1] == L')')
		{
			double value = parser.ReadFormula(section, L"WindowY", 0.0);
			WCHAR buffer[32];
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

		group = parser.ReadString(section, L"Group", group.c_str());
		InitializeGroup(group);

		// On the second loop override settings from the skin's section
		section = m_SkinName.c_str();
	}

	// Disable native transparency if older OS
	if (CSystem::GetOSPlatform() < OSPLATFORM_2K)
	{
		m_NativeTransparency = 0;
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
	WCHAR buffer[32];
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
bool CMeterWindow::ReadSkin()
{
	std::wstring iniFile = m_SkinPath;
	iniFile += m_SkinName;
	iniFile += L"\\";
	iniFile += m_SkinIniFile;

	// Verify whether the file exists
	if (_waccess(iniFile.c_str(), 0) == -1)
	{
		std::wstring message = L"Unable to refresh skin \"";
		message += m_SkinName.c_str();
		message += L"\\";
		message += m_SkinIniFile.c_str();
		message += L"\": Ini-file not found.";
		LSLog(LOG_DEBUG, APPNAME, message.c_str());
		MessageBox(m_Window, message.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
		return false;
	}

	m_Parser.Initialize(iniFile.c_str(), m_Rainmeter, this);

	// Global settings

	// Check the version 
	int appVersion = m_Parser.ReadInt(L"Rainmeter", L"AppVersion", 0);
	if (appVersion > RAINMETER_VERSION)
	{
		WCHAR buffer[128];
		std::wstring text;
		if (appVersion % 1000 != 0)
		{
			wsprintf(buffer, L"%i.%i.%i", appVersion / 1000000, (appVersion / 1000) % 1000, appVersion % 1000);
		}
		else
		{
			wsprintf(buffer, L"%i.%i", appVersion / 1000000, (appVersion / 1000) % 1000);
		}

		text = L"The skin \"";
		text += m_SkinName;
		text += L"\\";
		text += m_SkinIniFile;
		text += L"\" needs Rainmeter ";
		text += buffer;
		text += L" or newer.\nDownload the latest version of Rainmeter from www.rainmeter.net.";
		MessageBox(m_Window, text.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
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

	m_BackgroundMode = (BGMODE)m_Parser.ReadInt(L"Rainmeter", L"BackgroundMode", BGMODE_IMAGE);
	m_SolidBevel = (BEVELTYPE)m_Parser.ReadInt(L"Rainmeter", L"BevelType", BEVELTYPE_NONE);

	m_SolidColor = m_Parser.ReadColor(L"Rainmeter", L"SolidColor", Color::Gray);
	m_SolidColor2 = m_Parser.ReadColor(L"Rainmeter", L"SolidColor2", m_SolidColor);
	m_SolidAngle = (Gdiplus::REAL)m_Parser.ReadFloat(L"Rainmeter", L"GradientAngle", 0.0);

	m_DynamicWindowSize = 0!=m_Parser.ReadInt(L"Rainmeter", L"DynamicWindowSize", 0);

	if ((m_BackgroundMode == BGMODE_IMAGE || m_BackgroundMode == BGMODE_SCALED_IMAGE) && m_BackgroundName.empty())
	{
		m_BackgroundMode = BGMODE_COPY;
	}

	m_RightMouseDownAction = m_Parser.ReadString(L"Rainmeter", L"RightMouseDownAction", L"", false);
	m_LeftMouseDownAction = m_Parser.ReadString(L"Rainmeter", L"LeftMouseDownAction", L"", false);
	m_MiddleMouseDownAction = m_Parser.ReadString(L"Rainmeter", L"MiddleMouseDownAction", L"", false);
	m_RightMouseUpAction = m_Parser.ReadString(L"Rainmeter", L"RightMouseUpAction", L"", false);
	m_LeftMouseUpAction = m_Parser.ReadString(L"Rainmeter", L"LeftMouseUpAction", L"", false);
	m_MiddleMouseUpAction = m_Parser.ReadString(L"Rainmeter", L"MiddleMouseUpAction", L"", false);
	m_RightMouseDoubleClickAction = m_Parser.ReadString(L"Rainmeter", L"RightMouseDoubleClickAction", L"", false);
	m_LeftMouseDoubleClickAction = m_Parser.ReadString(L"Rainmeter", L"LeftMouseDoubleClickAction", L"", false);
	m_MiddleMouseDoubleClickAction = m_Parser.ReadString(L"Rainmeter", L"MiddleMouseDoubleClickAction", L"", false);
	m_MouseOverAction = m_Parser.ReadString(L"Rainmeter", L"MouseOverAction", L"", false);
	m_MouseLeaveAction = m_Parser.ReadString(L"Rainmeter", L"MouseLeaveAction", L"", false);
	m_OnRefreshAction = m_Parser.ReadString(L"Rainmeter", L"OnRefreshAction", L"", false);

	m_WindowUpdate = m_Parser.ReadInt(L"Rainmeter", L"Update", 1000);
	m_TransitionUpdate = m_Parser.ReadInt(L"Rainmeter", L"TransitionUpdate", 100);
	m_MouseActionCursor = 0 != m_Parser.ReadInt(L"Rainmeter", L"MouseActionCursor", 1);

	// Checking for localfonts
	std::wstring localFont = m_Parser.ReadString(L"Rainmeter", L"LocalFont", L"");
	// If there is a local font we want to load it
	if(!localFont.empty())
	{
		// We want to check the fonts folder first
		// !!!!!!! - We may want to fix the method in which I get the path to
		// Rainmeter/fonts
		std::wstring szFontFile = m_Rainmeter->GetPath().c_str();
		
		m_FontCollection = new Gdiplus::PrivateFontCollection();

		Status nResults = m_FontCollection->AddFontFile(szFontFile.c_str());
		
		// It wasn't found in the fonts folder, check the local folder
		if(nResults != Ok)
		{
			szFontFile = m_SkinPath; // Get the local path	
			szFontFile += m_SkinName;
			szFontFile += L"\\";
			szFontFile += localFont;

			nResults = m_FontCollection->AddFontFile(szFontFile.c_str());
			
			// The font wasn't found, check full path.
			if(nResults != Ok)
			{
				szFontFile = localFont.c_str();
				nResults = m_FontCollection->AddFontFile(szFontFile.c_str());
				if(nResults != Ok)
				{
					std::wstring error = L"Error: Couldn't load font file: ";
					error += localFont;
					DebugLog(error.c_str());
				}
			}

		}
		// Here we are checking to see if there are more than one local font
		// to be loaded. They will be named LocalFont2, LocalFont 3, etc.
		WCHAR tmpName[64];
		int i = 2;
		bool loop = true;
		do 
		{
			swprintf(tmpName, L"LocalFont%i", i);
			localFont = m_Parser.ReadString(L"Rainmeter", tmpName, L"");
			// There is a key called LocalFont%i
			if (!localFont.empty())
			{
				// We want to check the fonts folder first
				// !!!!!!! - We may want to fix the method in which I get the path to
				// Rainmeter/fonts
				std::wstring szFontFile = m_Rainmeter->GetPath().c_str();
				szFontFile  += L"Fonts\\";
				szFontFile  += localFont;
				
				nResults = m_FontCollection->AddFontFile(szFontFile.c_str());
				
				// It wasn't found in the fonts folder, check the local folder
				if(nResults != Ok)
				{
					szFontFile = m_SkinPath; // Get the local path
					szFontFile += m_SkinName;
					szFontFile += localFont;
					nResults = m_FontCollection->AddFontFile(szFontFile.c_str());
					
					// The font wasn't found, check full path.
					if(nResults != Ok)
					{
						szFontFile = localFont.c_str();
						nResults = m_FontCollection->AddFontFile(szFontFile.c_str());
						// The font file wasn't found anywhere, log the error
						if(nResults != Ok)
						{
							std::wstring error = L"Error: Couldn't load font file: ";
							error += localFont;
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
			++i;
		} while(loop);
	}

	// Create the meters and measures

	m_HasNetMeasures = false;
	m_HasButtons = false;

	// Get all the sections (i.e. different meters, measures and the other stuff)
	std::vector<std::wstring> arraySections = m_Parser.GetSections();

	for (size_t i = 0; i < arraySections.size(); ++i)
	{
		std::wstring strSection = arraySections[i];

		if(_wcsicmp(L"Rainmeter", strSection.c_str()) != 0 && 
			_wcsicmp(L"Variables", strSection.c_str()) != 0 &&
			_wcsicmp(L"Metadata", strSection.c_str()) != 0)
		{
			std::wstring meterName, measureName;

			// Check if the item is a meter or a measure (or perhaps something else)
			measureName = m_Parser.ReadString(strSection.c_str(), L"Measure", L"");
			meterName = m_Parser.ReadString(strSection.c_str(), L"Meter", L"");
			if (measureName.length() > 0)
			{
				// It's a measure
				CMeasure* measure = NULL;

				try
				{
					measure = CMeasure::Create(measureName.c_str(), this);
					if (measure)
					{
						measure->SetName(strSection.c_str());
						measure->ReadConfig(m_Parser, strSection.c_str());
					}
				}
				catch (CError& error)
				{
					delete measure;
					measure = NULL;
					MessageBox(m_Window, error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
				}

				if (measure)
				{
					m_Measures.push_back(measure);

					m_Parser.AddMeasure(measure);

					if (!m_HasNetMeasures && dynamic_cast<CMeasureNet*>(measure))
					{
						m_HasNetMeasures = true;
					}
				}
			}
			else if (meterName.length() > 0)
			{
				// It's a meter
				CMeter* meter = NULL;

				try
				{
					meter = CMeter::Create(meterName.c_str(), this);
					if (meter)
					{
						meter->SetName(strSection.c_str());
						
						if(m_MouseActionCursor == false)
						{
							meter->SetMouseActionCursor(false);
						}

						meter->ReadConfig(strSection.c_str());
					}
				}
				catch (CError& error)
				{
					delete meter;
					meter = NULL;
					MessageBox(m_Window, error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
				}

				if (meter)
				{
					m_Meters.push_back(meter);

					m_Parser.ClearStyleTemplate();

					if (!m_HasButtons && dynamic_cast<CMeterButton*>(meter))
					{
						m_HasButtons = true;
					}
				}
			}
			// If it's not a meter or measure it will be ignored
		}
	}

	if (m_Meters.empty())
	{
		std::wstring text;
		text = L"The skin \"";
		text += m_SkinName;
		text += L"\\";
		text += m_SkinIniFile;
		if (m_Measures.empty())
		{
			text += L"\" does not contain\nany valid meters or measures and will be deactivated.\n\nThe file may be damaged or not a Rainmeter skin file.";
			MessageBox(m_Window, text.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
			m_Rainmeter->DeactivateConfig(this, -1);
		}
		else
		{
			text += L"\" does not contain any meters.\nDo you want to deactivate this skin?";
			if (IDYES == MessageBox(m_Window, text.c_str(), APPNAME, MB_YESNO | MB_TOPMOST | MB_ICONEXCLAMATION))
			{
				m_Rainmeter->DeactivateConfig(this, -1);
			}
		}
	}
	else
	{
		// Bind the meters to the measures
		std::list<CMeter*>::const_iterator j = m_Meters.begin();
		for( ; j != m_Meters.end(); ++j)
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

	return true;
}

/*
** InitializeMeasures
**
** Initializes all the measures
**
*/
void CMeterWindow::InitializeMeasures()
{
	// Initalize all measures
	std::list<CMeasure*>::const_iterator i = m_Measures.begin();
	for( ; i != m_Measures.end(); ++i)
	{
		try
		{
			(*i)->Initialize();
		}
		catch (CError& error)
		{
			MessageBox(m_Window, error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
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
	std::list<CMeter*>::const_iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); ++j)
	{
		try
		{
			(*j)->Initialize();

			if (!(*j)->GetToolTipText().empty())
			{
				(*j)->CreateToolTip(this);
			}
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
	std::list<CMeter*>::const_iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); ++j)
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
	if (m_WindowW == 0 && m_WindowH == 0)
	{
		// Create a dummy bitmap to avoid invalid state
		m_DoubleBuffer = new Bitmap(1, 1, PixelFormat32bppARGB);
	}
	else
	{
		m_DoubleBuffer = new Bitmap(m_WindowW, m_WindowH, PixelFormat32bppARGB);
	}

	Graphics graphics(GetDoubleBuffer());

	if (m_Background)
	{
		// Copy the background over the doublebuffer
		Rect r(0, 0, m_WindowW, m_WindowH);
		graphics.DrawImage(m_Background, r, 0, 0, m_Background->GetWidth(), m_Background->GetHeight(), UnitPixel);
	}

	// Draw the meters
	std::list<CMeter*>::const_iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); ++j)
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
	++m_UpdateCounter;

	// Pre-updates
	if (m_HasNetMeasures) CMeasureNet::UpdateIFTable();
	CMeasureCalc::UpdateVariableMap(*this);

	// Update all measures
	std::list<CMeasure*>::const_iterator i = m_Measures.begin();
	for( ; i != m_Measures.end(); ++i)
	{
		try
		{
			if ((*i)->HasDynamicVariables())
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
	bool bActiveTransition = false;
	std::list<CMeter*>::const_iterator j = m_Meters.begin();
	for( ; j != m_Meters.end(); ++j)
	{
		try
		{
			if ((*j)->HasDynamicVariables())
			{
				(*j)->ReadConfig((*j)->GetName());
				m_Parser.ClearStyleTemplate();
			}
			(*j)->Update();
		}
		catch (CError& error)
		{
			MessageBox(m_Window, error.GetString().c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
		}

		// Update tooltips
		if ((*j)->GetToolTipHandle() == NULL)
		{
			if (!(*j)->GetToolTipText().empty())
			{
				(*j)->CreateToolTip(this);
			}
		}
		else
		{
			(*j)->UpdateToolTip();
		}

		// Check for transitions and start the timer if necessary
		if (!bActiveTransition && (*j)->HasActiveTransition())
		{
			bActiveTransition = true;
		}
	}

	if (!nodraw)
	{
		if (m_DynamicWindowSize)
		{
			// Resize the window
			m_ResetRegion = true;
		}

		Redraw();
	}

	// Start/stop the transition timer if necessary
	if (!m_ActiveTransition && bActiveTransition)
	{
		SetTimer(m_Window, TRANSITIONTIMER, m_TransitionUpdate, NULL);
		m_ActiveTransition = true;
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
		SIZE szWindow;

		if (m_WindowW == 0 && m_WindowH == 0)
		{
			// Set dummy size to avoid invalid state
			szWindow.cx = 1;
			szWindow.cy = 1;
		}
		else
		{
			szWindow.cx = m_WindowW;
			szWindow.cy = m_WindowH;
		}

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
** OnTimer
**
** Handles the timers. The METERTIMER updates all the measures 
** MOUSETIMER is used to hide/show the window.
**
*/
LRESULT CMeterWindow::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	if(wParam == METERTIMER) 
	{
		Update(false);
		UpdateAboutStatistics(m_SkinName.c_str());

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
	}
	else if(wParam == TRANSITIONTIMER)
	{
		// Redraw only if there is active transition still going
		bool bActiveTransition = false;
		std::list<CMeter*>::const_iterator j = m_Meters.begin();
		for( ; j != m_Meters.end(); ++j)
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
					bool keyDown = GetKeyState(VK_CONTROL) & 0x8000 || GetKeyState(VK_SHIFT) & 0x8000 || GetKeyState(VK_MENU) & 0x8000;

					if (!keyDown || GetWindowFromPoint(pos) != m_Window)
					{
						// Run all mouse leave actions
						OnMouseLeave(m_WindowDraggable ? WM_NCMOUSELEAVE : WM_MOUSELEAVE, 0, 0);
					}
				}
			}
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
	GetCursorPos(&pos);
	POINT posScr = pos;

	MapWindowPoints(NULL, m_Window, &pos, 1);
	inside = HitTest(pos.x, pos.y);

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

	if(m_WindowHide)
	{
		if (!m_Hidden && !inside && !keyDown)
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
** GetWindowFromPoint
**
** Retrieves a handle to the window that contains the specified point.
**
*/
HWND CMeterWindow::GetWindowFromPoint(POINT pos)
{
	HWND hwndPos = WindowFromPoint(pos);

	if (hwndPos == m_Window || !m_ClickThrough)
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
** HitTest
**
** Checks if the given point is inside the window.
**
*/
bool CMeterWindow::HitTest(int x, int y)
{
	if (x >= 0 && y >= 0 && x < m_WindowW && y < m_WindowH)
	{
		// Check transparent pixels
		if (m_DoubleBuffer)
		{
			Color color;
			Status status = m_DoubleBuffer->GetPixel(x, y, &color);
			if (status != Ok || color.GetA() > 0)
			{
				return true;
			}
		}
		else
		{
			return true;
		}
	}

	return false;
}

/*
** HandleButtons
**
** Handles all buttons and cursor.
** Note that meterWindow parameter is used if proc is BUTTONPROC_UP.
**
*/
void CMeterWindow::HandleButtons(POINT pos, BUTTONPROC proc, CMeterWindow* meterWindow, bool changeCursor) 
{
	bool redraw = false;
	bool drawCursor = false;

	std::list<CMeter*>::const_reverse_iterator j = m_Meters.rbegin();
	for( ; j != m_Meters.rend(); ++j)
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
					redraw |= button->MouseUp(pos, meterWindow);
					break;

				case BUTTONPROC_MOVE:
				default:
					redraw |= button->MouseMove(pos);
					break;
				}
			}
		}

		if (changeCursor && !drawCursor)
		{
			if ((*j)->HitTest(pos.x, pos.y) && (*j)->HasMouseActionCursor())
			{
				drawCursor = ((*j)->HasMouseAction() || button);	
			}
		}
	}

	if (redraw)
	{
		Redraw();
	}

	if (changeCursor)
	{
		SetCursor(LoadCursor(NULL, drawCursor ? IDC_HAND : IDC_ARROW));
	}
}

/*
** OnMouseMove
**
** When we get WM_MOUSEMOVE messages, hide the window as the mouse is over it.
**
*/
LRESULT CMeterWindow::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam) 
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

		if (!m_Hidden)
		{
			// If Alt, shift or control is down, do not hide the window
			switch(m_WindowHide) 
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

		while (DoMoveAction(pos.x, pos.y, MOUSE_OVER)) ;

		// Handle buttons
		HandleButtons(pos, BUTTONPROC_MOVE, NULL, true);
	}

	return 0;
}

/*
** OnMouseLeave
**
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
		POINT pos = {SHRT_MIN, SHRT_MIN};
		while (DoMoveAction(pos.x, pos.y, MOUSE_LEAVE)) ;  // Leave all forcibly

		// Handle buttons
		HandleButtons(pos, BUTTONPROC_MOVE, NULL, true);
	}

	return 0;
}

/*
** OnCreate
**
** During window creation we do nothing.
**
*/
LRESULT CMeterWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	return 0;
}

/*
** OnCommand
**
** Handle the menu commands.
**
*/
LRESULT CMeterWindow::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	try 
	{
		if(wParam == ID_CONTEXT_SKINMENU_EDITSKIN)
		{
			std::wstring command = m_Rainmeter->GetConfigEditor();
			command += L" \"";
			command += m_SkinPath + L"\\" + m_SkinName + L"\\" + m_SkinIniFile + L"\"";

			// If the skins are in the program folder start the editor as admin
			if (m_Rainmeter->GetPath() + L"Skins\\" == m_Rainmeter->GetSkinPath())
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
			SetKeepOnScreen(!m_KeepOnScreen);
		}
		else if(wParam == ID_CONTEXT_SKINMENU_CLICKTHROUGH)
		{
			SetClickThrough(!m_ClickThrough);
		}
		else if(wParam == ID_CONTEXT_SKINMENU_DRAGGABLE)
		{
			SetWindowDraggable(!m_WindowDraggable);
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
			UpdateTransparency(m_AlphaValue, false);
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
			UpdateTransparency(m_AlphaValue, false);
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
			UpdateTransparency(m_AlphaValue, false);
		}
		else if(wParam == ID_CONTEXT_SKINMENU_REMEMBERPOSITION)
		{
			SetSavePosition(!m_SavePosition);
		}
		else if(wParam == ID_CONTEXT_SKINMENU_SNAPTOEDGES)
		{
			SetSnapEdges(!m_SnapEdges);
		}
		else if(wParam >= ID_CONTEXT_SKINMENU_TRANSPARENCY_0 && wParam <= ID_CONTEXT_SKINMENU_TRANSPARENCY_90)
		{
			m_AlphaValue = (int)(255.0 - 230.0 * (double)(wParam - ID_CONTEXT_SKINMENU_TRANSPARENCY_0) / (double)(ID_CONTEXT_SKINMENU_TRANSPARENCY_90 - ID_CONTEXT_SKINMENU_TRANSPARENCY_0));
			WriteConfig();
			UpdateTransparency(m_AlphaValue, false);
		}
		else if(wParam == ID_CONTEXT_CLOSESKIN)
		{
			m_Rainmeter->DeactivateConfig(this, -1);
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
			const MULTIMONITOR_INFO& multimonInfo = CSystem::GetMultiMonitorInfo();
			const std::vector<MONITOR_INFO>& monitors = multimonInfo.monitors;

			int screenIndex;
			bool screenDefined;
			if (wParam == ID_CONTEXT_SKINMENU_MONITOR_PRIMARY)
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
** SetClickThrough
**
** Helper function for setting ClickThrough
**
*/
void CMeterWindow::SetClickThrough(bool b)
{
	m_ClickThrough = b;
	WriteConfig();

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
** SetKeepOnScreen
**
** Helper function for setting KeepOnScreen
**
*/
void CMeterWindow::SetKeepOnScreen(bool b)
{
	m_KeepOnScreen = b;
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

/*
** SetWindowDraggable
**
** Helper function for setting WindowDraggable
**
*/
void CMeterWindow::SetWindowDraggable(bool b)
{
	m_WindowDraggable = b;
	WriteConfig();
}

/*
** SetSavePosition
**
** Helper function for setting SavePosition
**
*/
void CMeterWindow::SetSavePosition(bool b)
{
	m_SavePosition = b;
	WriteConfig();
}

/*
** SetSnapEdges
**
** Helper function for setting SnapEdges
**
*/
void CMeterWindow::SetSnapEdges(bool b)
{
	m_SnapEdges = b;
	WriteConfig();
}

/*
** OnSysCommand
**
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
			WriteConfig();
		}

		POINT pos;
		GetCursorPos(&pos);
		MapWindowPoints(NULL, m_Window, &pos, 1);

		// Handle buttons
		HandleButtons(pos, BUTTONPROC_UP, NULL, true);  // redraw only
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
** OnExitSizeMove
**
** Ends dragging
**
*/
LRESULT CMeterWindow::OnExitSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	return 0;
}

/*
** OnNcHitTest
**
** This is overwritten so that the window can be dragged
**
*/
LRESULT CMeterWindow::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	if (m_WindowDraggable)
	{
		POINT pos;
		pos.x = (SHORT)LOWORD(lParam);
		pos.y = (SHORT)HIWORD(lParam);
		MapWindowPoints(NULL, m_Window, &pos, 1);

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
LRESULT CMeterWindow::OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	LPWINDOWPOS wp=(LPWINDOWPOS)lParam;

	if (!m_Refreshing)
	{
		if (m_WindowZPosition == ZPOSITION_ONBOTTOM || m_WindowZPosition == ZPOSITION_ONDESKTOP)
		{
			// do not change the z-order. This keeps the window on bottom.
			wp->flags|=SWP_NOZORDER;
		}
	}

	if ((wp->flags & SWP_NOMOVE) == 0) 
	{
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
				std::map<std::wstring, CMeterWindow*>::const_iterator iter = windows.begin();
				for( ; iter != windows.end(); ++iter)
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
LRESULT CMeterWindow::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam) 
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
LRESULT CMeterWindow::OnDisplayChange(UINT uMsg, WPARAM wParam, LPARAM lParam) 
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
LRESULT CMeterWindow::OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	return 0;
}

/*
** OnLeftButtonDown
**
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
		RECT rect;
		GetWindowRect(m_Window, &rect);
		pos.x = pos.x - rect.left;
		pos.y = pos.y - rect.top;
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_DOWN, NULL, true);

	if (!DoAction(pos.x, pos.y, MOUSE_LMB_DOWN, false) && m_WindowDraggable)
	{
		// Cancel the mouse event beforehand
		SetMouseLeaveEvent(true);

		// Run the DefWindowProc so the dragging works
		return DefWindowProc(m_Window, uMsg, wParam, lParam);
	}

	return 0;
}

/*
** OnLeftButtonUp
**
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
		RECT rect;
		GetWindowRect(m_Window, &rect);
		pos.x = pos.x - rect.left;
		pos.y = pos.y - rect.top;
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_UP, this, true);

	DoAction(pos.x, pos.y, MOUSE_LMB_UP, false);

	return 0;
}

/*
** OnLeftButtonDoubleClick
**
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
		RECT rect;
		GetWindowRect(m_Window, &rect);
		pos.x = pos.x - rect.left;
		pos.y = pos.y - rect.top;
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_DOWN, NULL, true);

	if (!DoAction(pos.x, pos.y, MOUSE_LMB_DBLCLK, false))
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
LRESULT CMeterWindow::OnRightButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	POINT pos;
	pos.x = (SHORT)LOWORD(lParam); 
	pos.y = (SHORT)HIWORD(lParam); 

	if (uMsg == WM_NCRBUTTONDOWN)
	{
		// Transform the point to client rect
		RECT rect;
		GetWindowRect(m_Window, &rect);
		pos.x = pos.x - rect.left;
		pos.y = pos.y - rect.top;
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_MOVE, NULL, true);

	DoAction(pos.x, pos.y, MOUSE_RMB_DOWN, false);

	return 0;
}

/*
** OnRightButtonUp
**
** Runs the action when right mouse button is up
**
*/
LRESULT CMeterWindow::OnRightButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	POINT pos;
	pos.x = (SHORT)LOWORD(lParam);
	pos.y = (SHORT)HIWORD(lParam);

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_MOVE, NULL, true);

	if (!DoAction(pos.x, pos.y, MOUSE_RMB_UP, false))
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
LRESULT CMeterWindow::OnRightButtonDoubleClick(UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	POINT pos;
	pos.x = (SHORT)LOWORD(lParam); 
	pos.y = (SHORT)HIWORD(lParam); 

	if (uMsg == WM_NCRBUTTONDBLCLK)
	{
		// Transform the point to client rect
		RECT rect;
		GetWindowRect(m_Window, &rect);
		pos.x = pos.x - rect.left;
		pos.y = pos.y - rect.top;
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_MOVE, NULL, true);

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
LRESULT CMeterWindow::OnMiddleButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	POINT pos;
	pos.x = (SHORT)LOWORD(lParam); 
	pos.y = (SHORT)HIWORD(lParam); 

	if (uMsg == WM_NCMBUTTONDOWN)
	{
		// Transform the point to client rect
		RECT rect;
		GetWindowRect(m_Window, &rect);
		pos.x = pos.x - rect.left;
		pos.y = pos.y - rect.top;
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_MOVE, NULL, true);

	DoAction(pos.x, pos.y, MOUSE_MMB_DOWN, false);

	return 0;
}

/*
** OnMiddleButtonUp
**
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
		RECT rect;
		GetWindowRect(m_Window, &rect);
		pos.x = pos.x - rect.left;
		pos.y = pos.y - rect.top;
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_MOVE, NULL, true);

	DoAction(pos.x, pos.y, MOUSE_MMB_UP, false);

	return 0;
}

/*
** OnMiddleButtonDoubleClick
**
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
		RECT rect;
		GetWindowRect(m_Window, &rect);
		pos.x = pos.x - rect.left;
		pos.y = pos.y - rect.top;
	}

	// Handle buttons
	HandleButtons(pos, BUTTONPROC_MOVE, NULL, true);

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
LRESULT CMeterWindow::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	POINT pos;
	int x = (SHORT)LOWORD(lParam); 
	int y = (SHORT)HIWORD(lParam); 

	RECT rect;
	GetWindowRect(m_Window, &rect);

	if (x == -1 && y == -1)  // WM_CONTEXTMENU is generated from the keyboard (Shift+F10/VK_APPS)
	{
		// Set menu position to (0,0) on the window
		pos.x = rect.left;
		pos.y = rect.top;
	}
	else
	{
		// Transform the point to client rect
		pos.x = x - rect.left;
		pos.y = y - rect.top;

		// Handle buttons
		HandleButtons(pos, BUTTONPROC_MOVE, NULL, true);

		// If RMB up or RMB down or double-click cause actions, do not show the menu!
		if (DoAction(pos.x, pos.y, MOUSE_RMB_UP, false) || DoAction(pos.x, pos.y, MOUSE_RMB_DOWN, true) || DoAction(pos.x, pos.y, MOUSE_RMB_DBLCLK, true))
		{
			return 0;
		}

		// Set menu position to cursor position
		pos.x = x;
		pos.y = y;
	}

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
	std::list<CMeter*>::const_reverse_iterator j = m_Meters.rbegin();
	for( ; j != m_Meters.rend(); ++j)
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
** DoMoveAction
**
** Executes the action if such are defined. Returns true, if meter/window which should be processed still may exist.
**
*/
bool CMeterWindow::DoMoveAction(int x, int y, MOUSE mouse) 
{
	bool buttonFound = false;

	// Check if the hitpoint was over some meter
	std::list<CMeter*>::const_reverse_iterator j = m_Meters.rbegin();
	for( ; j != m_Meters.rend(); ++j)
	{
		if (!(*j)->IsHidden() && (*j)->HitTest(x, y))
		{
			if (mouse == MOUSE_OVER)
			{
				if (!m_MouseOver)
				{
					// If the mouse is over a meter it's also over the main window
					//DebugLog(L"@Enter: %s", m_SkinName.c_str());
					m_MouseOver = true;
					SetMouseLeaveEvent(false);

					if (!m_MouseOverAction.empty())
					{
						m_Rainmeter->ExecuteCommand(m_MouseOverAction.c_str(), this);
						return true;
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
							if (!button->IsExecutable())
							{
								button->SetExecutable(true);
							}
							buttonFound = true;
						}
						else
						{
							if (button->IsExecutable())
							{
								button->SetExecutable(false);
							}
						}
					}
				}

				if (!(*j)->IsMouseOver())
				{
					if (!((*j)->GetMouseOverAction().empty()) ||
						!((*j)->GetMouseLeaveAction().empty()) ||
						button)
					{
						//DebugLog(L"MeterEnter: %s - [%s]", m_SkinName.c_str(), (*j)->GetName());
						(*j)->SetMouseOver(true);

						if (!((*j)->GetMouseOverAction().empty()))
						{
							m_Rainmeter->ExecuteCommand((*j)->GetMouseOverAction().c_str(), this);
							return true;
						}
					}
				}
			}
		}
		else
		{
			if ((*j)->IsMouseOver())
			{
				// Handle button
				if (m_HasButtons)
				{
					CMeterButton* button = dynamic_cast<CMeterButton*>(*j);
					if (button)
					{
						if (button->IsExecutable())
						{
							button->SetExecutable(false);
						}
					}
				}

				//DebugLog(L"MeterLeave: %s - [%s]", m_SkinName.c_str(), (*j)->GetName());
				(*j)->SetMouseOver(false);

				if (!((*j)->GetMouseLeaveAction().empty()))
				{
					m_Rainmeter->ExecuteCommand((*j)->GetMouseLeaveAction().c_str(), this);
					return true;
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
				//DebugLog(L"Enter: %s", m_SkinName.c_str());
				m_MouseOver = true;
				SetMouseLeaveEvent(false);

				if (!m_MouseOverAction.empty())
				{
					m_Rainmeter->ExecuteCommand(m_MouseOverAction.c_str(), this);
					return true;
				}
			}
		}
	}
	else
	{
		// Mouse leave happens when the mouse is outside the window
		if (m_MouseOver)
		{
			//DebugLog(L"Leave: %s", m_SkinName.c_str());
			m_MouseOver = false;
			SetMouseLeaveEvent(true);

			if (!m_MouseLeaveAction.empty())
			{
				m_Rainmeter->ExecuteCommand(m_MouseLeaveAction.c_str(), this);
				return true;
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
LRESULT CMeterWindow::OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	// The lParam's x/y parameters are given in screen coordinates for overlapped and pop-up windows
	// and in parent-client coordinates for child windows.

	// Store the new window position
	m_ScreenX = (SHORT)LOWORD(lParam);
	m_ScreenY = (SHORT)HIWORD(lParam);

	if (m_Dragging)
	{
		ScreenToWindow();
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
	MESSAGE(OnDelayedExecute, WM_DELAYED_EXECUTE)
	MESSAGE(OnDelayedRefresh, WM_DELAYED_REFRESH)
	MESSAGE(OnDelayedMove, WM_DELAYED_MOVE)
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
LRESULT CMeterWindow::OnDelayedExecute(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (lParam)
	{
		LPCTSTR szExecute = (LPCTSTR)lParam;
		COPYDATASTRUCT copyData;

		copyData.dwData = 1;
		copyData.cbData = (DWORD)((wcslen(szExecute) + 1) * sizeof(WCHAR));
		copyData.lpData = (void*)szExecute;

		OnCopyData(WM_COPYDATA, NULL, (LPARAM)&copyData);
	}
	return 0;
}

/*
** OnDelayedRefresh
**
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
		PostMessage(m_Window, WM_DELAYED_REFRESH, (WPARAM)NULL, (LPARAM)NULL);
	}

	return 0;
}

/*
** OnCopyData
**
** Handles bangs from the exe
**
*/
LRESULT CMeterWindow::OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	COPYDATASTRUCT* pCopyDataStruct = (COPYDATASTRUCT*) lParam;

	if (pCopyDataStruct && (pCopyDataStruct->dwData == 1) && (pCopyDataStruct->cbData > 0))
	{
		// Check that we're still alive
		bool found = false;
		std::map<std::wstring, CMeterWindow*>& meters = Rainmeter->GetAllMeterWindows();
		std::map<std::wstring, CMeterWindow*>::const_iterator iter = meters.begin();

		for ( ; iter != meters.end(); ++iter)
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

		if (_wcsnicmp(L"PLAY ", str.c_str(), 5) == 0 ||
			_wcsnicmp(L"PLAYLOOP ", str.c_str(), 9) == 0 ||
			_wcsnicmp(L"PLAYSTOP", str.c_str(), 8) == 0)
		{
			// Audio commands are special cases.
			Rainmeter->ExecuteCommand(str.c_str(), this);
			return TRUE;
		}

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

		if (_wcsicmp(bang.c_str(), L"!RainmeterWriteKeyValue") == 0)
		{
			// !RainmeterWriteKeyValue is a special case.
			if (CRainmeter::ParseString(arg.c_str()).size() < 4)
			{
				// Add the current config filepath to the args
				arg += L" \"";
				arg += m_SkinPath;
				arg += m_SkinName;
				arg += L"\\";
				arg += m_SkinIniFile;
				arg += L"\"";
			}
		}

		// Add the current config name to the args. If it's not defined already
		// the bang only affects this config, if there already is a config defined
		// another one doesn't matter.
		arg += L" \"";
		arg += m_SkinName;
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
** Converts the path to absolute by adding the skin's path to it (unless it already is absolute).
**
*/
std::wstring CMeterWindow::MakePathAbsolute(std::wstring path)
{
	if (path.empty() ||
		path.find(L":\\") != std::wstring::npos || path.find(L":/") != std::wstring::npos ||
		(path.length() >= 2 && (path[0] == L'\\' || path[0] == L'/') && (path[1] == L'\\' || path[1] == L'/')))  // UNC
	{
		return path;	// It's already absolute path (or it's empty)
	}

	std::wstring root = m_SkinPath + m_SkinName;

	WCHAR ch = root[root.length() - 1];
	if (ch != L'\\' && ch != L'/')
	{
		root += L"\\";
	}

	return root + path;
}
