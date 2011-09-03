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
#include "MeterWindow.h"
#include "MeasureNet.h"
#include "Error.h"

#define DEBUG_VERBOSE  (0)  // Set 1 if you need verbose logging.

#define ZPOS_FLAGS	(SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING)

enum TIMER
{
	TIMER_SHOWDESKTOP = 1,
	TIMER_NETSTATS    = 2,
	TIMER_DELETELATER = 3
};
enum INTERVAL
{
	INTERVAL_SHOWDESKTOP = 250,
	INTERVAL_NETSTATS    = 60000,
	INTERVAL_DELETELATER = 1000
};

MULTIMONITOR_INFO CSystem::c_Monitors = { 0 };

HWND CSystem::c_Window = NULL;
HWND CSystem::c_HelperWindow = NULL;

HWINEVENTHOOK CSystem::c_WinEventHook = NULL;

bool CSystem::c_ShowDesktop = false;

OSPLATFORM CSystem::c_Platform = OSPLATFORM_UNKNOWN;

std::wstring CSystem::c_WorkingDirectory;

extern CRainmeter* Rainmeter;

/*
** Initialize
**
** Creates a helper window to detect changes in the system.
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
		L"SystemWindow",
		WS_POPUP | WS_DISABLED,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		instance,
		NULL);

	c_HelperWindow = CreateWindowEx(
		WS_EX_TOOLWINDOW,
		L"RainmeterSystemClass",
		L"PositioningHelperWindow",
		WS_POPUP | WS_DISABLED,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		instance,
		NULL);

#ifndef _WIN64
	SetWindowLong(c_Window, GWL_USERDATA, magicDWord);
	SetWindowLong(c_HelperWindow, GWL_USERDATA, magicDWord);
#endif

	SetWindowPos(c_Window, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);
	SetWindowPos(c_HelperWindow, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);

	c_Monitors.monitors.reserve(8);
	SetMultiMonitorInfo();

	WCHAR directory[MAX_PATH] = {0};
	GetCurrentDirectory(MAX_PATH, directory);
	c_WorkingDirectory = directory;

	c_WinEventHook = SetWinEventHook(
		EVENT_SYSTEM_FOREGROUND,
		EVENT_SYSTEM_FOREGROUND,
		NULL,
		MyWinEventProc,
		0,
		0,
		WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

	SetTimer(c_Window, TIMER_SHOWDESKTOP, INTERVAL_SHOWDESKTOP, NULL);
	SetTimer(c_Window, TIMER_NETSTATS, INTERVAL_NETSTATS, NULL);
	SetTimer(c_Window, TIMER_DELETELATER, INTERVAL_DELETELATER, NULL);
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
	KillTimer(c_Window, TIMER_NETSTATS);
	KillTimer(c_Window, TIMER_DELETELATER);

	if (c_WinEventHook)
	{
		UnhookWinEvent(c_WinEventHook);
		c_WinEventHook = NULL;
	}

	if (c_HelperWindow)
	{
		DestroyWindow(c_HelperWindow);
		c_HelperWindow = NULL;
	}

	if (c_Window)
	{
		DestroyWindow(c_Window);
		c_Window = NULL;
	}
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
		Log(LOG_DEBUG, info.szDevice);
		LogWithArgs(LOG_DEBUG, L"  Flags    : %s(0x%08X)", (info.dwFlags & MONITORINFOF_PRIMARY) ? L"PRIMARY " : L"", info.dwFlags);
		LogWithArgs(LOG_DEBUG, L"  Handle   : 0x%p", hMonitor);
		LogWithArgs(LOG_DEBUG, L"  ScrArea  : L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
			lprcMonitor->left, lprcMonitor->top, lprcMonitor->right, lprcMonitor->bottom,
			lprcMonitor->right - lprcMonitor->left, lprcMonitor->bottom - lprcMonitor->top);
		LogWithArgs(LOG_DEBUG, L"  WorkArea : L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
			info.rcWork.left, info.rcWork.top, info.rcWork.right, info.rcWork.bottom,
			info.rcWork.right - info.rcWork.left, info.rcWork.bottom - info.rcWork.top);
	}
	if (m == NULL) return TRUE;

	if (m->useEnumDisplayDevices)
	{
		for (size_t i = 0, isize = m->monitors.size(); i < isize; ++i)
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

		wcsncpy_s(monitor.deviceName, info.szDevice, _TRUNCATE);  // E.g. "\\.\DISPLAY1"

		// Get the monitor name (E.g. "Generic Non-PnP Monitor")
		DISPLAY_DEVICE ddm = {sizeof(DISPLAY_DEVICE)};
		DWORD dwMon = 0;
		while (EnumDisplayDevices(info.szDevice, dwMon++, &ddm, 0))
		{
			if (ddm.StateFlags & DISPLAY_DEVICE_ACTIVE && ddm.StateFlags & DISPLAY_DEVICE_ATTACHED)
			{
				wcsncpy_s(monitor.monitorName, ddm.DeviceString, _TRUNCATE);
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
	if (c_Monitors.monitors.empty())
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

	c_Monitors.vsT = GetSystemMetrics(SM_YVIRTUALSCREEN);
	c_Monitors.vsL = GetSystemMetrics(SM_XVIRTUALSCREEN);
	c_Monitors.vsH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	c_Monitors.vsW = GetSystemMetrics(SM_CXVIRTUALSCREEN);

	c_Monitors.primary = 1;  // If primary screen is not found, 1st screen is assumed as primary screen.

	c_Monitors.useEnumDisplayDevices = true;
	c_Monitors.useEnumDisplayMonitors = false;

	if (logging)
	{
		Log(LOG_DEBUG, L"------------------------------");
		Log(LOG_DEBUG, L"* EnumDisplayDevices / EnumDisplaySettings API");
	}

	DISPLAY_DEVICE dd = {sizeof(DISPLAY_DEVICE)};

	if (EnumDisplayDevices(NULL, 0, &dd, 0))
	{
		DWORD dwDevice = 0;

		do
		{
			std::wstring msg;

			if (logging)
			{
				Log(LOG_DEBUG, dd.DeviceName);

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
				wcsncpy_s(monitor.deviceName, dd.DeviceName, _TRUNCATE);  // E.g. "\\.\DISPLAY1"

				// Get the monitor name (E.g. "Generic Non-PnP Monitor")
				DISPLAY_DEVICE ddm = {sizeof(DISPLAY_DEVICE)};
				DWORD dwMon = 0;
				while (EnumDisplayDevices(dd.DeviceName, dwMon++, &ddm, 0))
				{
					if (ddm.StateFlags & DISPLAY_DEVICE_ACTIVE && ddm.StateFlags & DISPLAY_DEVICE_ATTACHED)
					{
						wcsncpy_s(monitor.monitorName, ddm.DeviceString, _TRUNCATE);

						if (logging)
						{
							LogWithArgs(LOG_DEBUG, L"  Name     : %s", ddm.DeviceString);
						}
						break;
					}
				}

				if (logging)
				{
					LogWithArgs(LOG_DEBUG, L"  Adapter  : %s", dd.DeviceString);
					LogWithArgs(LOG_DEBUG, L"  Flags    : %s(0x%08X)", msg.c_str(), dd.StateFlags);
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
							LogWithArgs(LOG_DEBUG, L"  Handle   : 0x%p", monitor.handle);
						}
					}

					if (monitor.handle != NULL)
					{
						MONITORINFO info = {sizeof(MONITORINFO)};
						GetMonitorInfo(monitor.handle, &info);

						monitor.screen = info.rcMonitor;
						monitor.work = info.rcWork;

						if (logging)
						{
							LogWithArgs(LOG_DEBUG, L"  ScrArea  : L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
								info.rcMonitor.left, info.rcMonitor.top, info.rcMonitor.right, info.rcMonitor.bottom,
								info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top);
							LogWithArgs(LOG_DEBUG, L"  WorkArea : L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
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
					LogWithArgs(LOG_DEBUG, L"  Adapter  : %s", dd.DeviceString);
					LogWithArgs(LOG_DEBUG, L"  Flags    : %s(0x%08X)", msg.c_str(), dd.StateFlags);
				}
			}
			++dwDevice;
		} while (EnumDisplayDevices(NULL, dwDevice, &dd, 0));
	}

	if (monitors.empty())  // Failed to enumerate the non-mirroring monitors
	{
		Log(LOG_WARNING, L"Failed to enumerate the non-mirroring monitors. Only EnumDisplayMonitors is used instead.");
		c_Monitors.useEnumDisplayDevices = false;
		c_Monitors.useEnumDisplayMonitors = true;
	}

	if (logging)
	{
		Log(LOG_DEBUG, L"------------------------------");
		Log(LOG_DEBUG, L"* EnumDisplayMonitors API");
	}

	if (c_Monitors.useEnumDisplayMonitors)
	{
		EnumDisplayMonitors(NULL, NULL, MyInfoEnumProc, (LPARAM)(&c_Monitors));

		if (monitors.empty())  // Failed to enumerate the monitors
		{
			Log(LOG_WARNING, L"Failed to enumerate the monitors. Prepares the dummy monitor information.");
			c_Monitors.useEnumDisplayMonitors = false;

			MONITOR_INFO monitor = {0};
			wcsncpy_s(monitor.deviceName, L"DUMMY", _TRUNCATE);
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
		Log(LOG_DEBUG, L"------------------------------");

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
		Log(LOG_DEBUG, method.c_str());

		LogWithArgs(LOG_DEBUG, L"* MONITORS: Count=%i, Primary=@%i", (int)monitors.size(), c_Monitors.primary);
		Log(LOG_DEBUG, L"@0: Virtual screen");
		LogWithArgs(LOG_DEBUG, L"  L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
			c_Monitors.vsL, c_Monitors.vsT, c_Monitors.vsL + c_Monitors.vsW, c_Monitors.vsT + c_Monitors.vsH,
			c_Monitors.vsW, c_Monitors.vsH);

		for (size_t i = 0, isize = monitors.size(); i < isize; ++i)
		{
			if (monitors[i].active)
			{
				LogWithArgs(LOG_DEBUG, L"@%i: %s (active), MonitorName: %s", (int)i + 1, monitors[i].deviceName, monitors[i].monitorName);
				LogWithArgs(LOG_DEBUG, L"  L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
					monitors[i].screen.left, monitors[i].screen.top, monitors[i].screen.right, monitors[i].screen.bottom,
					monitors[i].screen.right - monitors[i].screen.left, monitors[i].screen.bottom - monitors[i].screen.top);
			}
			else
			{
				LogWithArgs(LOG_DEBUG, L"@%i: %s (inactive), MonitorName: %s", (int)i + 1, monitors[i].deviceName, monitors[i].monitorName);
			}
		}
		Log(LOG_DEBUG, L"------------------------------");
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

	for (size_t i = 0, isize = monitors.size(); i < isize; ++i)
	{
		if (monitors[i].active && monitors[i].handle != NULL)
		{
			MONITORINFO info = {sizeof(MONITORINFO)};
			GetMonitorInfo(monitors[i].handle, &info);

			monitors[i].work = info.rcWork;

			if (CRainmeter::GetDebug())
			{
				LogWithArgs(LOG_DEBUG, L"WorkArea@%i : L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
					(int)i + 1,
					info.rcWork.left, info.rcWork.top, info.rcWork.right, info.rcWork.bottom,
					info.rcWork.right - info.rcWork.left, info.rcWork.bottom - info.rcWork.top);
			}
		}
	}
}

/*
** GetDefaultShellWindow
**
** Finds the Default Shell's window.
**
*/
HWND CSystem::GetDefaultShellWindow()
{
	static HWND c_ShellW = NULL;  // cache
	HWND ShellW = GetShellWindow();

	if (ShellW)
	{
		if (ShellW == c_ShellW)
		{
			return ShellW;
		}
		else
		{
			WCHAR className[16];
			if (!(GetClassName(ShellW, className, 16) > 0 &&
				wcscmp(className, L"Progman") == 0))
			{
				ShellW = NULL;
			}
		}
	}

	c_ShellW = ShellW;
	return ShellW;
}

/*
** GetWorkerW
**
** Finds the WorkerW window.
** If the WorkerW window is not active, returns NULL.
**
*/
HWND CSystem::GetWorkerW()
{
	static HWND c_DefView = NULL;  // cache
	HWND ShellW = GetDefaultShellWindow();
	if (!ShellW) return NULL;  // Default Shell (Explorer) not running

	if (c_DefView && IsWindow(c_DefView))
	{
		HWND parent = GetAncestor(c_DefView, GA_PARENT);
		if (parent)
		{
			if (parent == ShellW)
			{
				return NULL;
			}
			else
			{
				WCHAR className[16];
				if (GetClassName(parent, className, 16) > 0 &&
					wcscmp(className, L"WorkerW") == 0)
				{
					return parent;
				}
			}
		}
	}

	HWND WorkerW = NULL, DefView = FindWindowEx(ShellW, NULL, L"SHELLDLL_DefView", L"");
	if (DefView == NULL)
	{
		while (WorkerW = FindWindowEx(NULL, WorkerW, L"WorkerW", L""))
		{
			if (IsWindowVisible(WorkerW) &&
				BelongToSameProcess(ShellW, WorkerW) &&
				(DefView = FindWindowEx(WorkerW, NULL, L"SHELLDLL_DefView", L"")))
			{
				break;
			}
		}
	}

	c_DefView = DefView;
	return WorkerW;
}

/*
** BelongToSameProcess
**
** Checks whether the given windows belong to the same process.
**
*/
bool CSystem::BelongToSameProcess(HWND hwndA, HWND hwndB)
{
	DWORD procAId = 0, procBId = 0;

	GetWindowThreadProcessId(hwndA, &procAId);
	GetWindowThreadProcessId(hwndB, &procBId);

	return (procAId == procBId);
}

/*
** MyEnumWindowsProc
**
** Retrieves the Rainmeter's meter windows in Z-order.
**
*/
BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	bool logging = CRainmeter::GetDebug() && DEBUG_VERBOSE;
	WCHAR className[64];
	CMeterWindow* Window;
	WCHAR flag;

	if (GetClassName(hwnd, className, 64) > 0 &&
		wcscmp(className, METERWINDOW_CLASS_NAME) == 0 &&
		Rainmeter && (Window = Rainmeter->GetMeterWindow(hwnd)))
	{
		ZPOSITION zPos = Window->GetWindowZPosition();
		if (zPos == ZPOSITION_ONDESKTOP || zPos == ZPOSITION_ONBOTTOM)
		{
			if (lParam)
			{
				((std::vector<CMeterWindow*>*)lParam)->push_back(Window);
			}

			if (logging) flag = L'+';
		}
		else
		{
			if (logging) flag = L'-';
		}

		if (logging)
		{
			LogWithArgs(LOG_DEBUG, L"%c [%c] 0x%p : %s (Name: \"%s\", zPos=%i)",
				flag, IsWindowVisible(hwnd) ? L'V' : L'H', hwnd, className, Window->GetSkinName().c_str(), (int)zPos);
		}
	}
	else
	{
		if (logging)
		{
			flag = (hwnd == CSystem::GetHelperWindow()) ? L'o' : ' ';
			LogWithArgs(LOG_DEBUG, L"%c [%c] 0x%p : %s", flag, IsWindowVisible(hwnd) ? L'V' : L'H', hwnd, className);
		}
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
		bool logging = CRainmeter::GetDebug() && DEBUG_VERBOSE;
		std::vector<CMeterWindow*> windowsInZOrder;

		if (logging) Log(LOG_DEBUG, L"1: ----- BEFORE -----");

		// Retrieve the Rainmeter's meter windows in Z-order
		EnumWindows(MyEnumWindowsProc, (LPARAM)(&windowsInZOrder));

		if (!c_ShowDesktop)
		{
			// Reset ZPos in Z-order (Bottom)
			std::vector<CMeterWindow*>::const_iterator iter = windowsInZOrder.begin(), iterEnd = windowsInZOrder.end();
			for ( ; iter != iterEnd; ++iter)
			{
				if ((*iter)->GetWindowZPosition() == ZPOSITION_ONBOTTOM)
				{
					(*iter)->ChangeZPos(ZPOSITION_ONBOTTOM);  // reset
				}
			}
		}

		// Reset ZPos in Z-order (On Desktop)
		std::vector<CMeterWindow*>::const_iterator iter = windowsInZOrder.begin(), iterEnd = windowsInZOrder.end();
		for ( ; iter != iterEnd; ++iter)
		{
			if ((*iter)->GetWindowZPosition() == ZPOSITION_ONDESKTOP)
			{
				(*iter)->ChangeZPos(ZPOSITION_ONDESKTOP);  // reset
			}
		}

		if (logging)
		{
			Log(LOG_DEBUG, L"2: ----- AFTER -----");

			// Log all windows in Z-order
			EnumWindows(MyEnumWindowsProc, (LPARAM)NULL);
		}
	}
}

/*
** PrepareHelperWindow
**
** Moves the helper window to the reference position.
**
*/
void CSystem::PrepareHelperWindow(HWND WorkerW)
{
	bool logging = CRainmeter::GetDebug() && DEBUG_VERBOSE;

	SetWindowPos(c_Window, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);  // always on bottom

	if (c_ShowDesktop && WorkerW)
	{
		// Set WS_EX_TOPMOST flag
		SetWindowPos(c_HelperWindow, HWND_TOPMOST, 0, 0, 0, 0, ZPOS_FLAGS);

		// Find the "backmost" topmost window
		HWND hwnd = WorkerW;
		while (hwnd = ::GetNextWindow(hwnd, GW_HWNDPREV))
		{
			if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
			{
				WCHAR className[64], windowText[64];

				if (logging)
				{
					GetClassName(hwnd, className, 64);
					GetWindowText(hwnd, windowText, 64);

					SetLastError(0);
				}

				// Insert the helper window after the found window
				if (0 != SetWindowPos(c_HelperWindow, hwnd, 0, 0, 0, 0, ZPOS_FLAGS))
				{
					if (logging)
					{
						LogWithArgs(LOG_DEBUG, L"System: HelperWindow: hwnd=0x%p (WorkerW=0x%p), hwndInsertAfter=0x%p (\"%s\" %s) - %s",
							c_HelperWindow, WorkerW, hwnd, windowText, className, (GetWindowLong(c_HelperWindow, GWL_EXSTYLE) & WS_EX_TOPMOST) ? L"TOPMOST" : L"NORMAL");
					}
					return;
				}

				if (logging)
				{
					DWORD err = GetLastError();
					LogWithArgs(LOG_DEBUG, L"System: HelperWindow: hwnd=0x%p (WorkerW=0x%p), hwndInsertAfter=0x%p (\"%s\" %s) - FAILED (ErrorCode=0x%08X)",
						c_HelperWindow, WorkerW, hwnd, windowText, className, err);
				}
			}
		}

		if (logging)
		{
			LogWithArgs(LOG_DEBUG, L"System: HelperWindow: hwnd=0x%p (WorkerW=0x%p), hwndInsertAfter=HWND_TOPMOST - %s",
				c_HelperWindow, WorkerW, (GetWindowLong(c_HelperWindow, GWL_EXSTYLE) & WS_EX_TOPMOST) ? L"TOPMOST" : L"NORMAL");
		}
	}
	else
	{
		// Insert the helper window to the bottom
		SetWindowPos(c_HelperWindow, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);

		if (logging)
		{
			LogWithArgs(LOG_DEBUG, L"System: HelperWindow: hwnd=0x%p (WorkerW=0x%p), hwndInsertAfter=HWND_BOTTOM - %s",
				c_HelperWindow, WorkerW, (GetWindowLong(c_HelperWindow, GWL_EXSTYLE) & WS_EX_TOPMOST) ? L"TOPMOST" : L"NORMAL");
		}
	}
}

/*
** CheckDesktopState
**
** Changes the "Show Desktop" state.
**
*/
bool CSystem::CheckDesktopState(HWND WorkerW)
{
	HWND hwnd = NULL;

	if (WorkerW && IsWindowVisible(WorkerW))
	{
		hwnd = FindWindowEx(NULL, WorkerW, L"RainmeterSystemClass", L"SystemWindow");
	}

	bool stateChanged = (hwnd && !c_ShowDesktop) || (!hwnd && c_ShowDesktop);

	if (stateChanged)
	{
		c_ShowDesktop = !c_ShowDesktop;

		if (CRainmeter::GetDebug())
		{
			LogWithArgs(LOG_DEBUG, L"System: \"%s\" has been detected.",
				c_ShowDesktop ? L"Show the desktop" : L"Show open windows");
		}

		PrepareHelperWindow(WorkerW);

		ChangeZPosInOrder();
	}

	return stateChanged;
}

/*
** MyWinEventHook
**
** The event hook procedure
**
*/
void CALLBACK CSystem::MyWinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
	if (event == EVENT_SYSTEM_FOREGROUND)
	{
		if (!c_ShowDesktop)
		{
			WCHAR className[16];
			if (GetClassName(hwnd, className, 16) > 0 &&
				wcscmp(className, L"WorkerW") == 0 &&
				BelongToSameProcess(GetDefaultShellWindow(), hwnd))
			{
				const int max = 5;
				int loop = 0;
				while (loop < max && FindWindowEx(hwnd, NULL, L"SHELLDLL_DefView", L"") == NULL)
				{
					Sleep(2);  // Wait for 2-16 ms before retrying
					++loop;
				}

				if (loop < max)
				{
					loop = 0;
					while (loop < max && !CheckDesktopState(hwnd))
					{
						Sleep(2);  // Wait for 2-16 ms before retrying
						++loop;
					}
				}
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
	if (hWnd != c_Window)
	{
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
	case WM_WINDOWPOSCHANGING:
		((LPWINDOWPOS)lParam)->flags |= SWP_NOZORDER;
		return 0;

	case WM_TIMER:
		switch (wParam)
		{
		case TIMER_SHOWDESKTOP:
			CheckDesktopState(GetWorkerW());
			return 0;

		case TIMER_NETSTATS:
			CMeasureNet::UpdateIFTable();
			CMeasureNet::UpdateStats();
			if (Rainmeter) Rainmeter->WriteStats(false);

			return 0;

		case TIMER_DELETELATER:
			if (Rainmeter) Rainmeter->ClearDeleteLaterList();
			return 0;
		}
		break;

	case WM_DISPLAYCHANGE:
		Log(LOG_NOTICE, L"System: Display setting has been changed.");
		ClearMultiMonitorInfo();
		CConfigParser::ClearMultiMonitorVariables();
	case WM_SETTINGCHANGE:
		if (uMsg == WM_DISPLAYCHANGE || (uMsg == WM_SETTINGCHANGE && wParam == SPI_SETWORKAREA))
		{
			if (uMsg == WM_SETTINGCHANGE)  // SPI_SETWORKAREA
			{
				Log(LOG_NOTICE, L"System: Work area has been changed.");
				UpdateWorkareaInfo();
				CConfigParser::UpdateWorkareaVariables();
			}

			if (Rainmeter)
			{
				// Deliver WM_DISPLAYCHANGE / WM_SETTINGCHANGE message to all meter windows
				const std::map<std::wstring, CMeterWindow*>& windows = Rainmeter->GetAllMeterWindows();
				std::map<std::wstring, CMeterWindow*>::const_iterator iter = windows.begin();
				for ( ; iter != windows.end(); ++iter)
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
** GetOSPlatform
**
** Checks which OS you are running.
**
*/
OSPLATFORM CSystem::GetOSPlatform()
{
	if (c_Platform == OSPLATFORM_UNKNOWN)
	{
		OSVERSIONINFOEX osvi = {sizeof(OSVERSIONINFOEX)};
		if (!GetVersionEx((OSVERSIONINFO*)&osvi) || osvi.dwPlatformId != VER_PLATFORM_WIN32_NT)
		{
			c_Platform = OSPLATFORM_9X;
		}
		else
		{
			if (osvi.dwMajorVersion <= 4)  // NT4 or older
			{
				c_Platform = OSPLATFORM_NT4;
			}
			else if (osvi.dwMajorVersion == 5)  // 2000 / XP (x64 / Server 2003, R2)
			{
				if (osvi.dwMinorVersion == 0)
				{
					c_Platform = OSPLATFORM_2K;
				}
				else
				{
					c_Platform = OSPLATFORM_XP;
				}
			}
			else if (osvi.dwMajorVersion == 6)  // Vista (Server 2008) / 7 (Server 2008R2)
			{
				if (osvi.dwMinorVersion == 0)
				{
					c_Platform = OSPLATFORM_VISTA;
				}
				else
				{
					c_Platform = OSPLATFORM_7;
				}
			}
			else  // newer OS
			{
				c_Platform = OSPLATFORM_7;
			}
		}
	}

	return c_Platform;
}

/*
** GetTickCount64
**
** Retrieves the number of milliseconds that have elapsed since the system was started.
** In XP, returns the predictive value due to the 32bit limitation.
**
*/
ULONGLONG CSystem::GetTickCount64()
{
	typedef ULONGLONG (WINAPI * FPGETTICKCOUNT64)();
	static FPGETTICKCOUNT64 c_GetTickCount64 = (FPGETTICKCOUNT64)GetProcAddress(GetModuleHandle(L"kernel32"), "GetTickCount64");

	if (c_GetTickCount64)
	{
		return c_GetTickCount64();
	}
	else
	{
		static ULONGLONG lastTicks = 0;
		ULONGLONG ticks = GetTickCount();
		while (ticks < lastTicks) ticks += 0x100000000;
		lastTicks = ticks;
		return ticks;
	}
}

/*
** RmLoadLibrary
**
** This function is a wrapper function for LoadLibrary().
**
** Avoids loading a DLL from current directory.
**
*/
HMODULE CSystem::RmLoadLibrary(LPCWSTR lpLibFileName, DWORD* dwError, bool ignoreErrors)
{
	HMODULE hLib = NULL;
	DWORD err;
	UINT oldMode;

	if (ignoreErrors)
	{
		oldMode = SetErrorMode(0);
		SetErrorMode(oldMode | SEM_FAILCRITICALERRORS);  // Prevent the system from displaying message box
	}

	// Remove current directory from DLL search path
	SetDllDirectory(L"");

	SetLastError(ERROR_SUCCESS);
	hLib = LoadLibrary(lpLibFileName);
	err = GetLastError();

	if (ignoreErrors)
	{
		SetErrorMode(oldMode);  // Reset
	}

	if (dwError)
	{
		*dwError = err;
	}

	return hLib;
}

/*
** ResetWorkingDirectory
**
** Resets working directory to default.
**
*/
void CSystem::ResetWorkingDirectory()
{
	WCHAR directory[MAX_PATH] = {0};
	GetCurrentDirectory(MAX_PATH, directory);

	if (_wcsicmp(directory, c_WorkingDirectory.c_str()) != 0)
	{
		SetCurrentDirectory(c_WorkingDirectory.c_str());
	}
}

/*
** SetClipboardText
**
** Sets clipboard text to given string.
**
*/
void CSystem::SetClipboardText(const std::wstring& text)
{
	if (OpenClipboard(NULL))
	{
		// Include terminating null char
		size_t len = text.length() + 1;

		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(WCHAR));
		if (hMem)
		{
			LPVOID data = GlobalLock(hMem);
			if (data)
			{
				memcpy(data, text.c_str(), len * sizeof(WCHAR));
				GlobalUnlock(hMem);

				EmptyClipboard();
				if (!SetClipboardData(CF_UNICODETEXT, hMem))
				{
					GlobalFree(hMem);
				}
			}
		}

		CloseClipboard();
	}
}

/*
** CopyFiles
**
** Copies files and folders from one location to another.
**
*/
bool CSystem::CopyFiles(const std::wstring& strFrom, const std::wstring& strTo, bool bMove)
{
	std::wstring tmpFrom(strFrom), tmpTo(strTo);

	// The strings must end with double nul
	tmpFrom.append(L"0");
	tmpFrom[tmpFrom.size() - 1] = L'\0';
	tmpTo.append(L"0");
	tmpTo[tmpTo.size() - 1] = L'\0';

	SHFILEOPSTRUCT fo = {0};
	fo.wFunc = bMove ? FO_MOVE : FO_COPY;
	fo.pFrom = tmpFrom.c_str();
	fo.pTo = tmpTo.c_str();
	fo.fFlags = FOF_NO_UI | FOF_NOCONFIRMATION | FOF_ALLOWUNDO;

	int result = SHFileOperation(&fo);
	if (result != 0)
	{
		LogWithArgs(LOG_ERROR, L"Unable to copy files from %s to %s (%i)", strFrom.c_str(), strTo.c_str(), result);
		return false;
	}
	return true;
}

/*
** RemoveFile
**
** Removes a file even if a file is read-only.
**
*/
bool CSystem::RemoveFile(const std::wstring& file)
{
	DWORD attr = GetFileAttributes(file.c_str());
	if (attr == -1 || (attr & FILE_ATTRIBUTE_READONLY))
	{
		// Unset read-only
		SetFileAttributes(file.c_str(), (attr == -1) ? FILE_ATTRIBUTE_NORMAL : attr - FILE_ATTRIBUTE_READONLY);
	}

	return (DeleteFile(file.c_str()) != 0);
}

/*
** RemoveFolder
**
** Recursively removes folder.
**
*/
bool CSystem::RemoveFolder(const std::wstring& strFolder)
{
	std::wstring tmpFolder(strFolder);

	// The strings must end with double nul
	tmpFolder.append(L"0");
	tmpFolder[tmpFolder.size() - 1] = L'\0';

	SHFILEOPSTRUCT fo = {0};
	fo.wFunc = FO_DELETE;
	fo.pFrom = tmpFolder.c_str();
	fo.fFlags = FOF_NO_UI | FOF_NOCONFIRMATION | FOF_ALLOWUNDO;

	int result = SHFileOperation(&fo);
	if (result != 0)
	{
		LogWithArgs(LOG_ERROR, L"Unable to delete folder %s (%i)", strFolder.c_str(), result);
		return false;
	}
	return true;
}

/*
** GetIniFileMappingList
**
** Retrieves the "IniFileMapping" entries from Registry.
**
*/
void CSystem::GetIniFileMappingList(std::vector<std::wstring>& iniFileMappings)
{
	HKEY hKey;
	LONG ret;

	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping", 0, KEY_ENUMERATE_SUB_KEYS, &hKey);
	if (ret == ERROR_SUCCESS)
	{
		WCHAR buffer[MAX_PATH];
		DWORD index = 0, cch = MAX_PATH;

		while (true)
		{
			ret = RegEnumKeyEx(hKey, index++, buffer, &cch, NULL, NULL, NULL, NULL);
			if (ret == ERROR_NO_MORE_ITEMS) break;

			if (ret == ERROR_SUCCESS)
			{
				iniFileMappings.push_back(buffer);
			}
			cch = MAX_PATH;
		}
		RegCloseKey(hKey);
	}
}

/*
** GetTemporaryFile
**
** Prepares a temporary file if iniFile is included in the "IniFileMapping" entries.
** If iniFile is not included, returns a empty string. If error occurred, returns "<>".
** Note that a temporary file must be deleted by caller.
**
*/
std::wstring CSystem::GetTemporaryFile(const std::vector<std::wstring>& iniFileMappings, const std::wstring& iniFile)
{
	std::wstring temporary;

	if (!iniFileMappings.empty())
	{
		std::wstring::size_type pos = iniFile.find_last_of(L'\\');
		std::wstring filename;

		if (pos != std::wstring::npos)
		{
			filename.assign(iniFile, pos + 1, iniFile.length() - (pos + 1));
		}
		else
		{
			filename = iniFile;
		}

		for (size_t i = 0, isize = iniFileMappings.size(); i < isize; ++i)
		{
			if (_wcsicmp(iniFileMappings[i].c_str(), filename.c_str()) == 0)
			{
				WCHAR buffer[MAX_PATH];

				GetTempPath(MAX_PATH, buffer);
				temporary = buffer;
				if (GetTempFileName(temporary.c_str(), L"cfg", 0, buffer) != 0)
				{
					temporary = buffer;

					std::wstring tmp = GetTemporaryFile(iniFileMappings, temporary);
					if (tmp.empty() && CopyFiles(iniFile, temporary))
					{
						return temporary;
					}
					else  // alternate is reserved or failed
					{
						RemoveFile(temporary);
						return tmp.empty() ? L"<>" : tmp;
					}
				}
				else  // failed
				{
					LogWithArgs(LOG_ERROR, L"Unable to create a temporary file to: %s", temporary.c_str());
					return L"<>";
				}
			}
		}
	}

	return temporary;
}
