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
#include "Rainmeter.h"
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
	monitors.clear();
	horizontalSpans.clear();
	verticalSpans.clear();
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

	// Populate with EnumDisplayDevices first because we also want inactive displays. This will keep
	// the monitor index consistent e.g. when monitors are plugged in/out.
	for (DWORD dwDevice = 0; ; ++dwDevice)
	{
		DISPLAY_DEVICE dd = { sizeof(DISPLAY_DEVICE) };
		if (!EnumDisplayDevices(nullptr, dwDevice, &dd, 0)) break;

		if ((dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) != 0) continue;

		MonitorInfo monitor = { 0 };
		monitor.handle = nullptr;
		monitor.active = (dd.StateFlags & DISPLAY_DEVICE_ACTIVE);
		monitor.deviceName.assign(dd.DeviceName, wcsnlen(dd.DeviceName, _countof(dd.DeviceName)));

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

		if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
		{
			c_Monitors.primary = (int)monitors.size();
		}
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

			// This should never happen, but keeping this here to match older code.
			if (monitor == monitors.end())
			{
				monitors.emplace_back();
				monitor = monitors.end() - 1;
				monitor->active = true;
				monitor->deviceName = info.szDevice;
			}

			monitor->handle = hMonitor;
			monitor->screen = *monitorRect;
			monitor->work = info.rcWork;
			monitor->dpi = MonitorUtil::GetDpiForMonitor(hMonitor);

			return TRUE;
		}, 0);

	int firstActive = 0;
	bool primaryActive = false;
	for (auto iter = monitors.begin(); iter != monitors.end(); ++iter)
	{
		MonitorInfo& monitor = *iter;
		if (monitor.active && monitor.handle == nullptr)
		{
			LogWarningF(L"Failed to get monitor info for: %s", monitor.deviceName.c_str());
			monitor.active = false;
		}

		if (monitor.active)
		{
			const int index = (int)(iter - monitors.begin()) + 1;
			if (firstActive == 0) firstActive = index;
			if (c_Monitors.primary == index) primaryActive = true;
		}
	}

	if (firstActive == 0)
	{
		monitors.clear();
	}
	else if (!primaryActive)
	{
		c_Monitors.primary = firstActive;
	}

	// Can this really happen...? Leaving it here because the old code had it.
	if (monitors.empty())
	{
		LogWarning(L"Failed to enumerate monitors. Using dummy monitor info.");
		MonitorInfo monitor;
		monitor.active = true;
		monitor.handle = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
		monitor.screen = monitor.work = c_Monitors.virtualScreen;
		monitor.deviceName = L"DUMMY";
		monitor.dpi = MonitorUtil::GetDpiForMonitor(monitor.handle);
		monitors.push_back(monitor);
		c_Monitors.primary = 1;
	}

	{
		DpiUtil::DpiUnawareScope dpiUnaware;
		c_Monitors.logicalVirtualScreen = GetVirtualScreenRect();

		for (auto& monitor : monitors)
		{
			if (monitor.active && monitor.handle != nullptr)
			{
				MONITORINFO info = { sizeof(MONITORINFO) };
				GetMonitorInfo(monitor.handle, &info);
				monitor.logicalScreen = info.rcMonitor;
				monitor.logicalWork = info.rcWork;
			}
		}
	}

	c_Monitors.UpdateSpans();

	if (GetRainmeter().GetDebug())
	{
		LogDebug(L"------------------------------");
		LogDebugF(L"* MONITORS: Count=%i, Primary=@%i", (int)monitors.size(), c_Monitors.primary);
		LogDebug(L"@0: Virtual screen");

		RECT r = c_Monitors.virtualScreen;
		LogDebugF(L"    L=%i, T=%i, W=%i, H=%i", r.left, r.top, r.right - r.left, r.bottom - r.top);

		int i = 1;
		for (auto iter = monitors.cbegin(); iter != monitors.cend(); ++iter, ++i)
		{
			if (iter->active)
			{
				LogDebugF(L"@%i: %s (active) - %s", i, iter->deviceName.c_str(), iter->monitorName.c_str());

				r = iter->screen;
				LogDebugF(L"    ScreenArea: X=%i, Y=%i, W=%i, H=%i", r.left, r.top, r.right - r.left, r.bottom - r.top);
				r = iter->work;
				LogDebugF(L"    WorkArea:   X=%i, Y=%i, W=%i, H=%i", r.left, r.top, r.right - r.left, r.bottom - r.top);
				LogDebugF(L"    Dpi:        %u", iter->dpi);
			}
			else if (iter->monitorName.empty())
			{
				LogDebugF(L"@%i: %s (inactive)", i, iter->deviceName.c_str());
			}
			else
			{
				LogDebugF(L"@%i: %s (inactive) - %s", i, iter->deviceName.c_str(), iter->monitorName.c_str());
			}
		}
		LogDebug(L"------------------------------");
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

	int i = 1;
	for (auto iter = monitors.begin(); iter != monitors.end(); ++iter, ++i)
	{
		if (iter->active && iter->handle != nullptr)
		{
			MONITORINFO info = { sizeof(MONITORINFO) };
			GetMonitorInfo(iter->handle, &info);

			iter->work = info.rcWork;
			iter->dpi = MonitorUtil::GetDpiForMonitor((*iter).handle);

			if (GetRainmeter().GetDebug())
			{
				RECT r = info.rcWork;
				LogDebugF(L"WorkArea@%i: L=%i, T=%i, W=%i, H=%i", i, r.left, r.top, r.right - r.left, r.bottom - r.top);
			}
		}
	}

	c_Monitors.UpdateSpans();
}

void MultiMonitorInfo::UpdateSpans()
{
	horizontalSpans = CreateLogicalSpans(monitors, primary, true);
	verticalSpans = CreateLogicalSpans(monitors, primary, false);

	for (auto& monitor : monitors)
	{
		if (!monitor.active) continue;
		if (monitor.logicalScreen.left != monitor.logicalScreen.right) continue;

		const LONG left = ConvertPhysicalToLogical(monitor.screen.left, horizontalSpans);
		const LONG top = ConvertPhysicalToLogical(monitor.screen.top, verticalSpans);
		monitor.logicalScreen = {
			left,
			top,
			left + MulDiv(monitor.screen.right - monitor.screen.left, USER_DEFAULT_SCREEN_DPI, monitor.dpi),
			top + MulDiv(monitor.screen.bottom - monitor.screen.top, USER_DEFAULT_SCREEN_DPI, monitor.dpi)
		};
	}
}

int MultiMonitorInfo::MonitorIndexForWindow(HWND window) const
{
	const auto windowMonitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);

	int index = 1;
	for (const auto& monitor : monitors)
	{
		if (monitor.active && monitor.handle == windowMonitor)
		{
			return index;
		}

		++index;
	}

	return primary;
}

auto MultiMonitorInfo::CreateLogicalSpans(const std::vector<MonitorInfo>& monitors, int primary, bool horizontal) -> std::vector<Span>
{
	const auto& getAxisStart = [&](const MonitorInfo& monitor) -> LONG
	{
		return horizontal ? monitor.screen.left : monitor.screen.top;
	};

	const auto& getAxisEnd = [&](const MonitorInfo& monitor) -> LONG
	{
		return horizontal ? monitor.screen.right : monitor.screen.bottom;
	};

	std::vector<LONG> boundaries;
	for (const auto& monitor : monitors)
	{
		if (!monitor.active) continue;

		boundaries.push_back(getAxisStart(monitor));
		boundaries.push_back(getAxisEnd(monitor));
	}

	std::sort(boundaries.begin(), boundaries.end());
	boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());

	std::vector<Span> spans;
	if (boundaries.size() < 2) return spans;

	// Split the virtual screen into non-overlapping axis spans so each span can be converted with
	// the DPI of the monitor that covers the entire segment.
	for (size_t i = 0; i + 1 < boundaries.size(); ++i)
	{
		const LONG start = boundaries[i];
		const LONG end = boundaries[i + 1];
		if (start == end) continue;

		int monitorIndex = -1;
		for (size_t j = 0; j < monitors.size(); ++j)
		{
			const auto& monitor = monitors[j];
			if (monitor.active &&
				getAxisStart(monitor) <= start &&
				getAxisEnd(monitor) >= end)
			{
				if ((int)j + 1 == primary)
				{
					monitorIndex = (int)j;
					break;
				}

				if (monitorIndex == -1)
				{
					monitorIndex = (int)j;
				}
			}
		}

		if (monitorIndex != -1)
		{
			spans.push_back({ start, end, 0, 0, monitors[monitorIndex].dpi });
		}
	}

	if (spans.empty()) return spans;

	// Keep the primary monitor's physical origin as the logical origin, then accumulate neighboring
	// spans outward so mixed-DPI layouts stay continuous.
	const LONG primaryStart =
		primary > 0 && primary <= (int)monitors.size() ?
		getAxisStart(monitors[primary - 1]) :
		spans.front().physicalStart;

	size_t anchor = 0;
	for (size_t i = 0; i < spans.size(); ++i)
	{
		if (primaryStart >= spans[i].physicalStart && primaryStart < spans[i].physicalEnd)
		{
			anchor = i;
			break;
		}
	}

	spans[anchor].logicalStart =
		primaryStart == spans[anchor].physicalStart ?
		primaryStart :
		primaryStart - MulDiv(primaryStart - spans[anchor].physicalStart, USER_DEFAULT_SCREEN_DPI, spans[anchor].dpi);
	spans[anchor].logicalEnd =
		spans[anchor].logicalStart + MulDiv(spans[anchor].physicalEnd - spans[anchor].physicalStart, USER_DEFAULT_SCREEN_DPI, spans[anchor].dpi);

	for (size_t i = anchor + 1; i < spans.size(); ++i)
	{
		spans[i].logicalStart = spans[i - 1].logicalEnd;
		spans[i].logicalEnd = spans[i].logicalStart + MulDiv(spans[i].physicalEnd - spans[i].physicalStart, USER_DEFAULT_SCREEN_DPI, spans[i].dpi);
	}

	for (size_t i = anchor; i > 0; --i)
	{
		spans[i - 1].logicalEnd = spans[i].logicalStart;
		spans[i - 1].logicalStart = spans[i - 1].logicalEnd - MulDiv(spans[i - 1].physicalEnd - spans[i - 1].physicalStart, USER_DEFAULT_SCREEN_DPI, spans[i - 1].dpi);
	}

	return spans;
}

LONG MultiMonitorInfo::ConvertPhysicalToLogical(LONG value, const std::vector<Span>& spans)
{
	if (spans.empty()) return value;

	for (const auto& span : spans)
	{
		if (value >= span.physicalStart && value <= span.physicalEnd)
		{
			return span.logicalStart + MulDiv(value - span.physicalStart, USER_DEFAULT_SCREEN_DPI, span.dpi);
		}
	}

	const auto& span = value < spans.front().physicalStart ? spans.front() : spans.back();
	return span.logicalStart + MulDiv(value - span.physicalStart, USER_DEFAULT_SCREEN_DPI, span.dpi);
}

LONG MultiMonitorInfo::ConvertLogicalToPhysical(LONG value, const std::vector<Span>& spans)
{
	if (spans.empty()) return value;

	for (const auto& span : spans)
	{
		if (value >= span.logicalStart && value <= span.logicalEnd)
		{
			return span.physicalStart + MulDiv(value - span.logicalStart, span.dpi, USER_DEFAULT_SCREEN_DPI);
		}
	}

	const auto& span = value < spans.front().logicalStart ? spans.front() : spans.back();
	return span.physicalStart + MulDiv(value - span.logicalStart, span.dpi, USER_DEFAULT_SCREEN_DPI);
}

POINT MultiMonitorInfo::PhysicalToLogical(POINT point) const
{
	for (size_t i = 0; i < monitors.size(); ++i)
	{
		const auto& monitor = monitors[i];
		if (monitor.active && Contains(monitor.screen, point))
		{
			return {
				monitor.logicalScreen.left + MulDiv(point.x - monitor.screen.left, USER_DEFAULT_SCREEN_DPI, monitor.dpi),
				monitor.logicalScreen.top + MulDiv(point.y - monitor.screen.top, USER_DEFAULT_SCREEN_DPI, monitor.dpi)
			};
		}
	}

	// Points outside any monitor still convert against the nearest edge span so off-screen
	// positions remain stable when skins are dragged past a boundary.
	return {
		ConvertPhysicalToLogical(point.x, horizontalSpans),
		ConvertPhysicalToLogical(point.y, verticalSpans)
	};
}

POINT MultiMonitorInfo::LogicalToPhysical(POINT point, UINT* dpi) const
{
	for (size_t i = 0; i < monitors.size(); ++i)
	{
		const auto& monitor = monitors[i];
		if (monitor.active && Contains(monitor.logicalScreen, point))
		{
			if (dpi) *dpi = monitor.dpi;

			return {
				monitor.screen.left + MulDiv(point.x - monitor.logicalScreen.left, monitor.dpi, USER_DEFAULT_SCREEN_DPI),
				monitor.screen.top + MulDiv(point.y - monitor.logicalScreen.top, monitor.dpi, USER_DEFAULT_SCREEN_DPI)
			};
		}
	}

	if (dpi) *dpi = 0;

	return {
		ConvertLogicalToPhysical(point.x, horizontalSpans),
		ConvertLogicalToPhysical(point.y, verticalSpans)
	};
}
