/*
  Copyright (C) 2010 spx

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
#include "System.h"
#include "Litestep.h"
#include "Rainmeter.h"
#include "Error.h"

enum TIMER
{
	TIMER_SHOWDESKTOP = 1,
	TIMER_COMPOSITION = 2
};
enum INTERVAL
{
	INTERVAL_SHOWDESKTOP = 250,
	INTERVAL_COMPOSITION = 250
};

MULTIMONITOR_INFO CSystem::c_Monitors = { 0 };

HWND CSystem::c_Window = NULL;
bool CSystem::c_DwmCompositionEnabled = false;
bool CSystem::c_ShowDesktop = false;

extern CRainmeter* Rainmeter;

/*
** Initialize
**
** Creates a window to detect changes in the system.
**
*/
void CSystem::Initialize(HINSTANCE instance)
{
	WNDCLASS wc = {0};
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.hInstance = instance;
	wc.lpszClassName = L"RainmeterSystemClass";

	RegisterClass(&wc);

	c_Window = CreateWindowEx(
		WS_EX_TOOLWINDOW,
		L"RainmeterSystemClass",
		NULL,
		WS_POPUP,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		instance,
		NULL);

	SetWindowPos(c_Window, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

#ifndef _WIN64
	SetWindowLong(c_Window, GWL_USERDATA, magicDWord);
#endif

	SetMultiMonitorInfo();

	c_DwmCompositionEnabled = (DwmIsCompositionEnabled() == TRUE);
	if (c_DwmCompositionEnabled)
	{
		SetTimer(c_Window, TIMER_SHOWDESKTOP, INTERVAL_SHOWDESKTOP, NULL);
	}
}

/*
** Finalize
**
** Destroys a window.
**
*/
void CSystem::Finalize()
{
	KillTimer(c_Window, TIMER_SHOWDESKTOP);
	KillTimer(c_Window, TIMER_COMPOSITION);

	if (c_Window) DestroyWindow(c_Window);
}

/* MyInfoEnumProc
**
** Retrieves the multi-monitor information.
**
*/
BOOL CALLBACK MyInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	MULTIMONITOR_INFO* m = (MULTIMONITOR_INFO*)dwData;

	MONITORINFOEX info;
	info.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(hMonitor, &info);

	if (CRainmeter::GetDebug())
	{
		DebugLog(info.szDevice);
		DebugLog(L"  Flags    : %s(0x%08X)", (info.dwFlags & MONITORINFOF_PRIMARY) ? L"PRIMARY " : L"", info.dwFlags);
		DebugLog(L"  Handle   : 0x%08X", hMonitor);
		DebugLog(L"  ScrArea  : L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
			lprcMonitor->left, lprcMonitor->top, lprcMonitor->right, lprcMonitor->bottom,
			lprcMonitor->right - lprcMonitor->left, lprcMonitor->bottom - lprcMonitor->top);
		DebugLog(L"  WorkArea : L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
			info.rcWork.left, info.rcWork.top, info.rcWork.right, info.rcWork.bottom,
			info.rcWork.right - info.rcWork.left, info.rcWork.bottom - info.rcWork.top);
	}
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
** Returns the number of monitors.
**
*/
size_t CSystem::GetMonitorCount()
{
	if (c_Monitors.monitors.size() == 0)
	{
		SetMultiMonitorInfo();
	}
	return c_Monitors.monitors.size();
}

/* SetMultiMonitorInfo
**
** Sets the multi-monitor information.
**
*/
void CSystem::SetMultiMonitorInfo()
{
	std::vector<MONITOR_INFO>& monitors = c_Monitors.monitors;
	bool logging = CRainmeter::GetDebug();

	if (monitors.capacity() < 16) { monitors.reserve(16); }

	c_Monitors.vsT = GetSystemMetrics(SM_YVIRTUALSCREEN);
	c_Monitors.vsL = GetSystemMetrics(SM_XVIRTUALSCREEN);
	c_Monitors.vsH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	c_Monitors.vsW = GetSystemMetrics(SM_CXVIRTUALSCREEN);

	c_Monitors.primary = 1;  // If primary screen is not found, 1st screen is assumed as primary screen.

	c_Monitors.useEnumDisplayDevices = true;
	c_Monitors.useEnumDisplayMonitors = false;

	if (logging)
	{
		DebugLog(L"------------------------------");
		DebugLog(L"* EnumDisplayDevices / EnumDisplaySettings API");
	}

	DISPLAY_DEVICE dd = {0};
	dd.cb = sizeof(DISPLAY_DEVICE);

	if (EnumDisplayDevices(NULL, 0, &dd, 0))
	{
		DWORD dwDevice = 0;

		do
		{
			std::wstring msg;

			if (logging)
			{
				DebugLog(dd.DeviceName);

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

						if (logging)
						{
							DebugLog(L"  Name     : %s", ddm.DeviceString);
						}
						break;
					}
				}

				if (logging)
				{
					DebugLog(L"  Adapter  : %s", dd.DeviceString);
					DebugLog(L"  Flags    : %s(0x%08X)", msg.c_str(), dd.StateFlags);
				}

				if (dd.StateFlags & DISPLAY_DEVICE_ACTIVE)
				{
					monitor.active = true;

					DEVMODE dm = {0};
					dm.dmSize = sizeof(DEVMODE);

					if (EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm))
					{
						POINT pos = {dm.dmPosition.x, dm.dmPosition.y};
						monitor.handle = MonitorFromPoint(pos, MONITOR_DEFAULTTONULL);

						if (logging)
						{
							DebugLog(L"  Handle   : 0x%08X", monitor.handle);
						}
					}

					if (monitor.handle != NULL)
					{
						MONITORINFO info = {0};
						info.cbSize = sizeof(MONITORINFO);
						GetMonitorInfo(monitor.handle, &info);

						monitor.screen = info.rcMonitor;
						monitor.work = info.rcWork;

						if (logging)
						{
							DebugLog(L"  ScrArea  : L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
								info.rcMonitor.left, info.rcMonitor.top, info.rcMonitor.right, info.rcMonitor.bottom,
								info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top);
							DebugLog(L"  WorkArea : L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
								info.rcWork.left, info.rcWork.top, info.rcWork.right, info.rcWork.bottom,
								info.rcWork.right - info.rcWork.left, info.rcWork.bottom - info.rcWork.top);
						}
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
				if (logging)
				{
					DebugLog(L"  Adapter  : %s", dd.DeviceString);
					DebugLog(L"  Flags    : %s(0x%08X)", msg.c_str(), dd.StateFlags);
				}
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

	if (logging)
	{
		DebugLog(L"------------------------------");
		DebugLog(L"* EnumDisplayMonitors API");
	}

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
		if (logging)
		{
			EnumDisplayMonitors(NULL, NULL, MyInfoEnumProc, (LPARAM)NULL);  // Only logging
		}
	}

	if (logging)
	{
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
}

/* UpdateWorkareaInfo
**
** Updates the workarea information.
**
*/
void CSystem::UpdateWorkareaInfo()
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

			if (CRainmeter::GetDebug())
			{
				DebugLog(L"WorkArea @%i : L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
					i + 1,
					info.rcWork.left, info.rcWork.top, info.rcWork.right, info.rcWork.bottom,
					info.rcWork.right - info.rcWork.left, info.rcWork.bottom - info.rcWork.top);
			}
		}
	}
}

/*
** GetShellDesktopWindow
**
** Finds the Shell's desktop window.
**
*/
HWND CSystem::GetShellDesktopWindow()
{
	HWND DesktopW = NULL;

	HWND hwnd = FindWindow(L"Progman", L"Program Manager");
	if (!hwnd) return NULL;  // Default Shell (Explorer) not started

	if (!(hwnd = FindWindowEx(hwnd, NULL, L"SHELLDLL_DefView", L"")) ||
		!(DesktopW = FindWindowEx(hwnd, NULL, L"SysListView32", L"FolderView")))  // for Windows 7 (with Aero)
	{
		HWND WorkerW = NULL;
		while (WorkerW = FindWindowEx(NULL, WorkerW, L"WorkerW", L""))
		{
			if ((hwnd = FindWindowEx(WorkerW, NULL, L"SHELLDLL_DefView", L"")) &&
				(DesktopW = FindWindowEx(hwnd, NULL, L"SysListView32", L"FolderView"))) break;
		}
	}

	return DesktopW;
}

/*
** GetWorkerW
**
** Finds the WorkerW window.
** If the Progman or WorkerW window is not found, this function returns NULL.
** 
** In Windows Vista / 7 (without Aero):
** This function returns a topmost window handle which is visible.
**
** In Windows 7 (with Aero):
** This function returns a window handle which has the "SHELLDLL_DefView".
**
*/
HWND CSystem::GetWorkerW()
{
	HWND WorkerW = NULL;

	HWND hwnd = FindWindow(L"Progman", L"Program Manager");
	if (!hwnd) return NULL;  // Default Shell (Explorer) not started

	if ((hwnd = FindWindowEx(hwnd, NULL, L"SHELLDLL_DefView", L"")) &&
		(hwnd = FindWindowEx(hwnd, NULL, L"SysListView32", L"FolderView")))  // for Windows Vista / 7 (without Aero)
	{
		while (WorkerW = FindWindowEx(NULL, WorkerW, L"WorkerW", L""))
		{
			if (IsWindowVisible(WorkerW))
			{
				// Check whether WorkerW covers whole of the screens
				WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
				GetWindowPlacement(WorkerW, &wp);

				if (wp.rcNormalPosition.left == c_Monitors.vsL &&
					wp.rcNormalPosition.top == c_Monitors.vsT &&
					(wp.rcNormalPosition.right - wp.rcNormalPosition.left) == c_Monitors.vsW &&
					(wp.rcNormalPosition.bottom - wp.rcNormalPosition.top) == c_Monitors.vsH) break;
			}
		}
	}
	else  // for Windows 7 (with Aero)
	{
		while (WorkerW = FindWindowEx(NULL, WorkerW, L"WorkerW", L""))
		{
			if ((hwnd = FindWindowEx(WorkerW, NULL, L"SHELLDLL_DefView", L"")) &&
				FindWindowEx(hwnd, NULL, L"SysListView32", L"FolderView")) break;
		}
	}

	return WorkerW;
}

/*
** MyEnumWindowsProc
**
** Retrieves the Rainmeter meter window pinned on desktop in Z-order.
**
*/
BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	bool logging = false;  // Set true if you need verbose logging.
	WCHAR className[128] = {0};
	CMeterWindow* Window;

	if (GetClassName(hwnd, className, 128) > 0 &&
		wcscmp(className, L"RainmeterMeterWindow") == 0 &&
		(Window = (CMeterWindow*)GetProp(hwnd, L"RAINMETER")))
	{
		if (Window->GetWindowZPosition() == ZPOSITION_ONDESKTOP)
		{
			if (logging) DebugLog(L"+ [%c] 0x%08X : %s (Name: \"%s\")", IsWindowVisible(hwnd) ? L'V' : L'H', hwnd, className, Window->GetSkinName().c_str());

			if (lParam)
			{
				((std::vector<CMeterWindow*>*)lParam)->push_back(Window);
			}
		}
		else
		{
			if (logging) DebugLog(L"- [%c] 0x%08X : %s (Name: \"%s\")", IsWindowVisible(hwnd) ? L'V' : L'H', hwnd, className, Window->GetSkinName().c_str());
		}
	}
	else
	{
		if (logging) DebugLog(L"  [%c] 0x%08X : %s", IsWindowVisible(hwnd) ? L'V' : L'H', hwnd, className);
	}

	return TRUE;
}

/*
** ChangeZPosInOrder
**
** Arranges the meter window in Z-order.
**
*/
void CSystem::ChangeZPosInOrder()
{
	if (Rainmeter)
	{
		bool logging = false;  // Set true if you need verbose logging.
		std::vector<CMeterWindow*> windowsInZOrder;

		if (logging) LSLog(LOG_DEBUG, L"Rainmeter", L"1: -----");

		// Retrieve the Rainmeter meter window in Z-order
		if (logging) LSLog(LOG_DEBUG, L"Rainmeter", L" [Top-level window]");
		EnumWindows(MyEnumWindowsProc, (LPARAM)(&windowsInZOrder));

		HWND DesktopW = GetShellDesktopWindow();
		if (DesktopW)
		{
			if (logging) LSLog(LOG_DEBUG, L"Rainmeter", L" [Child of Shell's desktop window]");
			EnumChildWindows(DesktopW, MyEnumWindowsProc, (LPARAM)(&windowsInZOrder));
		}

		if (logging) LSLog(LOG_DEBUG, L"Rainmeter", L"2: -----");

		// Reset ZPos in Z-order
		for (size_t i = 0; i < windowsInZOrder.size(); i++)
		{
			windowsInZOrder[i]->ChangeZPos(ZPOSITION_ONDESKTOP);  // reset
		}

		if (logging)
		{
			LSLog(LOG_DEBUG, L"Rainmeter", L"3: -----");
			LSLog(LOG_DEBUG, L"Rainmeter", L" [Top-level window]");

			// Log all windows in Z-order
			EnumWindows(MyEnumWindowsProc, (LPARAM)NULL);

			if (DesktopW)
			{
				LSLog(LOG_DEBUG, L"Rainmeter", L" [Child of Shell's desktop window]");
				EnumChildWindows(DesktopW, MyEnumWindowsProc, (LPARAM)NULL);
			}
		}
	}
}

/*
** WndProc
**
** The window procedure
**
*/
LRESULT CALLBACK CSystem::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static int DesktopCompositionCheckCount = 0;

	switch (uMsg)
	{
	case WM_WINDOWPOSCHANGING:
		((LPWINDOWPOS)lParam)->flags |= SWP_NOZORDER;
		return 0;

	case WM_TIMER:
		switch (wParam)
		{
		case TIMER_SHOWDESKTOP:
			{
				HWND WorkerW = GetWorkerW(), hwnd = NULL;

				if (WorkerW)
				{
					hwnd = FindWindowEx(NULL, WorkerW, L"RainmeterSystemClass", L"");
				}

				if ((hwnd && !c_ShowDesktop) || (!hwnd && c_ShowDesktop))
				{
					c_ShowDesktop = !c_ShowDesktop;

					if (CRainmeter::GetDebug())
					{
						DebugLog(L"System: %s",
							c_ShowDesktop ? L"\"Show the desktop\" has been detected." : L"\"Show open windows\" has been detected.");
					}

					ChangeZPosInOrder();
				}
			}
			return 0;

		case TIMER_COMPOSITION:
			{
				if (GetShellDesktopWindow() || DesktopCompositionCheckCount >= 10)  // 250ms * 10 = 2.5s
				{
					KillTimer(c_Window, TIMER_COMPOSITION);

					ChangeZPosInOrder();

					if (c_DwmCompositionEnabled)
					{
						SetTimer(c_Window, TIMER_SHOWDESKTOP, INTERVAL_SHOWDESKTOP, NULL);
					}
				}
				else
				{
					DesktopCompositionCheckCount++;
				}
			}
			return 0;
		}
		break;

	case WM_DWMCOMPOSITIONCHANGED:
		DebugLog(L"System: DWM desktop composition has been changed.");

		KillTimer(c_Window, TIMER_SHOWDESKTOP);
		KillTimer(c_Window, TIMER_COMPOSITION);

		c_DwmCompositionEnabled = (DwmIsCompositionEnabled() == TRUE);

		DesktopCompositionCheckCount = 0;
		SetTimer(c_Window, TIMER_COMPOSITION, INTERVAL_COMPOSITION, NULL);

		return 0;

	case WM_DISPLAYCHANGE:
		DebugLog(L"System: Display setting has been changed.");
		ClearMultiMonitorInfo();
		CConfigParser::ClearMultiMonitorVariables();
	case WM_SETTINGCHANGE:
		if (uMsg == WM_DISPLAYCHANGE || (uMsg == WM_SETTINGCHANGE && wParam == SPI_SETWORKAREA))
		{
			if (uMsg == WM_SETTINGCHANGE)  // SPI_SETWORKAREA
			{
				DebugLog(L"System: Work area has been changed.");
				UpdateWorkareaInfo();
				CConfigParser::UpdateWorkareaVariables();
			}

			if (Rainmeter)
			{
				// Deliver WM_DISPLAYCHANGE / WM_SETTINGCHANGE message to all meter windows
				std::map<std::wstring, CMeterWindow*>& windows = Rainmeter->GetAllMeterWindows();
				std::map<std::wstring, CMeterWindow*>::const_iterator iter = windows.begin();
				for( ; iter != windows.end(); iter++)
				{
					PostMessage((*iter).second->GetWindow(), WM_DELAYED_MOVE, (WPARAM)uMsg, (LPARAM)0);
				}
			}
		}
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
** DwmIsCompositionEnabled
**
** Returns TRUE if the DWM desktop composition is enabled.
**
*/
BOOL CSystem::DwmIsCompositionEnabled()
{
	BOOL fEnabled = FALSE;

	typedef HRESULT (WINAPI * FPDWMISCOMPOSITIONENABLED)(BOOL* pfEnabled);
	HINSTANCE h = LoadLibrary(L"dwmapi.dll");
	if (h)
	{
		FPDWMISCOMPOSITIONENABLED DwmIsCompositionEnabled = (FPDWMISCOMPOSITIONENABLED)GetProcAddress(h, "DwmIsCompositionEnabled");
		if (DwmIsCompositionEnabled)
		{
			if (DwmIsCompositionEnabled(&fEnabled) != S_OK)
			{
				fEnabled = FALSE;
			}
		}
		FreeLibrary(h);
	}
	return fEnabled;
}
