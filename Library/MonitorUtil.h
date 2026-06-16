/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __RAINMETER_MONITORUTIL_H__
#define __RAINMETER_MONITORUTIL_H__

#include <windows.h>
#include <string>
#include <vector>

struct MonitorInfo
{
	bool active;
	HMONITOR handle;
	UINT dpi;
	RECT screen;
	RECT work;
	std::wstring deviceName;				// Device name (E.g. "\\.\DISPLAY1")
	std::wstring monitorName;				// Monitor name (E.g. "Generic Non-PnP Monitor")

	LONG ToLogical(LONG value) const;
	RECT ToLogical(const RECT& rect) const;
};

struct MultiMonitorInfo
{
	bool useEnumDisplayDevices;				// If true, use EnumDisplayDevices function to obtain the multi-monitor information
	bool useEnumDisplayMonitors;			// If true, use EnumDisplayMonitors function to obtain the multi-monitor information

	int vsT, vsL, vsH, vsW;		// Coordinates of the top-left corner (vsT,vsL) and size (vsH,vsW) of the virtual screen
	int primary;							// Index of the primary monitor
	std::vector<MonitorInfo> monitors;

	RECT GetPhysicalVirtualScreenRect() const;
	RECT GetLogicalVirtualScreenRect() const;
};

#endif
