/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../Common/DpiUtil.h"
#include "MonitorUtil.h"
#include "System.h"
#include "Logger.h"

namespace MonitorUtil {

static MultiMonitorInfo c_Monitors;

bool g_DpiAppCompatMode = false;

void SetMultiMonitorInfo();

UINT GetDpiForMonitor(HMONITOR monitor)
{
	typedef HRESULT(WINAPI* GetDpiForMonitorProc)(HMONITOR, int, UINT*, UINT*);
	static auto s_GetDpiForMonitor = []() -> GetDpiForMonitorProc
	{
		HMODULE module = GetModuleHandle(L"Shcore");
		if (!module)
		{
			module = LoadLibrary(L"Shcore.dll");
		}

		return module ? (GetDpiForMonitorProc)GetProcAddress(module, "GetDpiForMonitor") : nullptr;
	}();

	// See the note about AppCompatFlags in Rainmeter.cpp.
	if (g_DpiAppCompatMode) return USER_DEFAULT_SCREEN_DPI;

	if (monitor && s_GetDpiForMonitor)
	{
		UINT dpiX = USER_DEFAULT_SCREEN_DPI;
		UINT dpiY = USER_DEFAULT_SCREEN_DPI;
		if (SUCCEEDED(s_GetDpiForMonitor(monitor, 0, &dpiX, &dpiY)) && dpiX > 0)
		{
			return dpiX;
		}
	}

	return System::GetSystemDpi();
}

RECT GetVirtualScreenRect()
{
	RECT rect;
	rect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
	rect.right = rect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);

	rect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
	rect.bottom = rect.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
	return rect;
}

}

static bool Contains(const RECT& rect, POINT point)
{
	return point.x >= rect.left &&
		point.x < rect.right &&
		point.y >= rect.top &&
		point.y < rect.bottom;
}

void MultiMonitorInfo::Clear()
{
	deviceCount = 0;
	displayCount = 0;
	monitors.clear();
}

void MonitorUtil::InitializeMultiMonitorInfo()
{
	c_Monitors.monitors.reserve(4);
	SetMultiMonitorInfo();
}

void MonitorUtil::EnableDpiAppCompatMode()
{
	c_Monitors.Clear();
	g_DpiAppCompatMode = true;
	SetMultiMonitorInfo();
}

const MultiMonitorInfo& MonitorUtil::GetMultiMonitorInfo()
{
	if (c_Monitors.monitors.empty())
	{
		SetMultiMonitorInfo();
	}
	return c_Monitors;
}

void MonitorUtil::ClearMultiMonitorInfo()
{
	c_Monitors.Clear();
}

void MonitorUtil::SetMultiMonitorInfo()
{
	auto& monitors = c_Monitors.monitors;

	c_Monitors.virtualScreen = GetVirtualScreenRect();
	c_Monitors.primary = 0;
	c_Monitors.deviceCount = 0;
	c_Monitors.displayCount = 0;

	// Populate with EnumDisplayDevices first because we also want inactive displays. This will keep
	// the monitor index consistent e.g. when monitors are plugged in/out.
	uint8_t deviceNumber = 0;
	for (DWORD dwDevice = 0; ; ++dwDevice)
	{
		DISPLAY_DEVICE dd = { sizeof(DISPLAY_DEVICE) };
		if (!EnumDisplayDevices(nullptr, dwDevice, &dd, 0)) break;

		if ((dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) != 0) continue;

		MonitorInfo monitor = { 0 };
		monitor.handle = nullptr;
		monitor.deviceName.assign(dd.DeviceName, wcsnlen(dd.DeviceName, _countof(dd.DeviceName)));
		monitor.deviceNumber = ++c_Monitors.deviceCount;

		// Get the monitor name (E.g. "Generic Non-PnP Monitor")
		for (DWORD dwMon = 0; ; ++dwMon)
		{
			DISPLAY_DEVICE ddm = { sizeof(DISPLAY_DEVICE) };
			if (!EnumDisplayDevices(monitor.deviceName.c_str(), dwMon, &ddm, 0)) break;

			if (ddm.StateFlags & DISPLAY_DEVICE_ACTIVE && ddm.StateFlags & DISPLAY_DEVICE_ATTACHED)
			{
				monitor.monitorName.assign(ddm.DeviceString, wcsnlen(ddm.DeviceString, _countof(ddm.DeviceString)));
				break;
			}
		}

		monitors.push_back(monitor);
	}

	// Now use EnumDisplayMonitors and link it up with the previous enumeration.
	EnumDisplayMonitors(nullptr, nullptr,
		[](HMONITOR hMonitor, HDC, RECT* monitorRect, LPARAM) -> BOOL
		{
			auto& monitors = c_Monitors.monitors;

			MONITORINFOEX info = { sizeof(MONITORINFOEX) };
			if (!GetMonitorInfo(hMonitor, &info)) return TRUE;

			auto monitor = std::find_if(monitors.begin(), monitors.end(),
				[&](const MonitorInfo& monitor)
				{
					return monitor.handle == nullptr && _wcsicmp(info.szDevice, monitor.deviceName.c_str()) == 0;
				});

			if (monitor == monitors.end()) return TRUE;

			monitor->active = true;
			monitor->handle = hMonitor;
			monitor->displayNumber = ++c_Monitors.displayCount;
			monitor->screen = *monitorRect;
			monitor->work = info.rcWork;
			monitor->dpi = MonitorUtil::GetDpiForMonitor(hMonitor);

			if (info.dwFlags & MONITORINFOF_PRIMARY)
			{
				c_Monitors.primary = monitor->deviceNumber;
			}

			return TRUE;
		}, 0);

	// Can this really happen...? Leaving it here because the old code had it.
	if (monitors.empty())
	{
		LogWarning(L"Failed to enumerate monitors. Using dummy monitor info.");
		MonitorInfo monitor;
		monitor.deviceNumber = 1;
		monitor.displayNumber = 1;
		monitor.handle = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
		monitor.screen = monitor.work = c_Monitors.virtualScreen;
		monitor.deviceName = L"DUMMY";
		monitor.dpi = MonitorUtil::GetDpiForMonitor(monitor.handle);
		monitors.push_back(monitor);
	}

	if (c_Monitors.primary == 0)
	{
		c_Monitors.primary = 1;
	}

	{
		DpiUtil::DpiUnawareScope dpiUnaware;
		c_Monitors.logicalVirtualScreen = GetVirtualScreenRect();

		for (auto& monitor : monitors)
		{
			if (monitor.handle != nullptr)
			{
				MONITORINFO info = { sizeof(MONITORINFO) };
				GetMonitorInfo(monitor.handle, &info);
				monitor.logicalScreen = info.rcMonitor;
				monitor.logicalWork = info.rcWork;
			}
		}
	}

}

void MonitorUtil::UpdateWorkareaInfo()
{
	std::vector<MonitorInfo>& monitors = c_Monitors.monitors;

	if (monitors.empty())
	{
		SetMultiMonitorInfo();
		return;
	}

	for (auto& monitor : monitors)
	{
		if (monitor.handle != nullptr)
		{
			MONITORINFO info = { sizeof(MONITORINFO) };
			GetMonitorInfo(monitor.handle, &info);

			monitor.work = info.rcWork;
			monitor.dpi = MonitorUtil::GetDpiForMonitor(monitor.handle);
		}
	}
}

const MonitorInfo* MultiMonitorInfo::GetForWindow(HWND window) const
{
	const auto handle = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
	return GetByHandle(handle);
}

const MonitorInfo* MultiMonitorInfo::GetByHandle(HMONITOR monitorHandle) const
{
	for (const auto& monitor : monitors)
	{
		if (monitor.handle == monitorHandle) return &monitor;
	}

	return nullptr;
}

const MonitorInfo* MultiMonitorInfo::GetByDeviceNumber(int deviceNumber) const
{
	return (deviceNumber > 0 && deviceNumber <= deviceCount) ? &monitors[deviceNumber - 1] : nullptr;
}

const MonitorInfo* MultiMonitorInfo::GetByDisplayNumber(int screenNumber) const
{
	for (const auto& monitor : monitors)
	{
		if (monitor.displayNumber == screenNumber) return &monitor;
	}

	return nullptr;
}
