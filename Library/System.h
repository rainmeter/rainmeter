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

#ifndef __RAINMETER_SYSTEM_H__
#define __RAINMETER_SYSTEM_H__

#pragma warning(disable: 4786)

#include <windows.h>
#include <vector>

struct MONITOR_INFO
{
	bool active;
	HMONITOR handle;
	RECT screen;
	RECT work;
	WCHAR deviceName[32];					//Device name (E.g. "\\.\DISPLAY1")
	WCHAR monitorName[128];					//Monitor name (E.g. "Generic Non-PnP Monitor")
};

struct MULTIMONITOR_INFO
{
	bool useEnumDisplayDevices;				//If true, use EnumDisplayDevices function to obtain the multi-monitor information
	bool useEnumDisplayMonitors;			//If true, use EnumDisplayMonitors function to obtain the multi-monitor information

	int vsT, vsL, vsH, vsW;					//Coordinates of the top-left corner (vsT,vsL) and size (vsH,vsW) of the virtual screen
	int primary;							//Index of the primary monitor
	std::vector<MONITOR_INFO> monitors;		//Monitor information
};

class CSystem
{
public:
	static void Initialize(HINSTANCE instance);
	static void Finalize();

	static const MULTIMONITOR_INFO& GetMultiMonitorInfo() { return c_Monitors; }
	static size_t GetMonitorCount();

	static bool GetDwmCompositionEnabled() { return c_DwmCompositionEnabled; }
	static bool GetShowDesktop() { return c_ShowDesktop; }

	static HWND GetShellDesktopWindow();
	static HWND GetWorkerW();

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL DwmIsCompositionEnabled();

	static void SetMultiMonitorInfo();
	static void ClearMultiMonitorInfo() { c_Monitors.monitors.clear(); }
	static void UpdateWorkareaInfo();

	static void ChangeZPosInOrder();

	static HWND c_Window;

	static MULTIMONITOR_INFO c_Monitors;		// Multi-Monitor info

	static bool c_DwmCompositionEnabled;
	static bool c_ShowDesktop;
};

#endif
