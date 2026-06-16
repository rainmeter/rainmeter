/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MonitorUtil.h"
#include "System.h"
#include "Rainmeter.h"
#include "Logger.h"

namespace MonitorUtil {

void SetMultiMonitorInfo();

static MultiMonitorInfo c_Monitors;

}

RECT MonitorInfo::ToLogical(const RECT& rect) const
{
	return { ToLogical(rect.left), ToLogical(rect.top), ToLogical(rect.right), ToLogical(rect.bottom) };
}

LONG MonitorInfo::ToLogical(LONG value) const
{
	return MulDiv(value, USER_DEFAULT_SCREEN_DPI, dpi);
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

BOOL CALLBACK MyInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	MultiMonitorInfo* m = (MultiMonitorInfo*)dwData;

	MONITORINFOEX info = {};
	info.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(hMonitor, &info);

	if (m == nullptr) return TRUE;

	if (m->useEnumDisplayDevices)
	{
		for (auto iter = m->monitors.begin(); iter != m->monitors.end(); ++iter)
		{
			if ((*iter).handle == nullptr && _wcsicmp(info.szDevice, (*iter).deviceName.c_str()) == 0)
			{
				(*iter).handle = hMonitor;
				(*iter).screen = *lprcMonitor;
				(*iter).work = info.rcWork;
				(*iter).dpi = System::GetDpiForMonitor(hMonitor);
				break;
			}
		}
	}
	else  // use only EnumDisplayMonitors
	{
		MonitorInfo monitor;
		monitor.active = true;

		monitor.handle = hMonitor;
		monitor.screen = *lprcMonitor;
		monitor.work = info.rcWork;
		monitor.dpi = System::GetDpiForMonitor(hMonitor);

		monitor.deviceName = info.szDevice;  // E.g. "\\.\DISPLAY1"

		// Get the monitor name (E.g. "Generic Non-PnP Monitor")
		DISPLAY_DEVICE ddm = {sizeof(DISPLAY_DEVICE)};
		DWORD dwMon = 0;
		while (EnumDisplayDevices(info.szDevice, dwMon++, &ddm, 0))
		{
			if (ddm.StateFlags & DISPLAY_DEVICE_ACTIVE && ddm.StateFlags & DISPLAY_DEVICE_ATTACHED)
			{
				monitor.monitorName.assign(ddm.DeviceString, wcsnlen(ddm.DeviceString, _countof(ddm.DeviceString)));
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

void MonitorUtil::InitializeMultiMonitorInfo()
{
	c_Monitors.monitors.reserve(4);
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
	std::vector<MonitorInfo>& monitors = c_Monitors.monitors;

	c_Monitors.vsT = GetSystemMetrics(SM_YVIRTUALSCREEN);
	c_Monitors.vsL = GetSystemMetrics(SM_XVIRTUALSCREEN);
	c_Monitors.vsH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	c_Monitors.vsW = GetSystemMetrics(SM_CXVIRTUALSCREEN);

	c_Monitors.primary = 1;  // If primary screen is not found, 1st screen is assumed as primary screen.

	c_Monitors.useEnumDisplayDevices = true;
	c_Monitors.useEnumDisplayMonitors = false;

	DISPLAY_DEVICE dd = {sizeof(DISPLAY_DEVICE)};

	if (EnumDisplayDevices(nullptr, 0, &dd, 0))
	{
		DWORD dwDevice = 0;

		do
		{
			if ((dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) != 0) continue;

			std::wstring deviceName(dd.DeviceName, wcsnlen(dd.DeviceName, _countof(dd.DeviceName)));

			MonitorInfo monitor = { 0 };
			monitor.handle = nullptr;
			monitor.deviceName = deviceName;  // E.g. "\\.\DISPLAY1"

			// Get the monitor name (E.g. "Generic Non-PnP Monitor")
			DISPLAY_DEVICE ddm = {sizeof(DISPLAY_DEVICE)};
			DWORD dwMon = 0;
			while (EnumDisplayDevices(deviceName.c_str(), dwMon++, &ddm, 0))
			{
				if (ddm.StateFlags & DISPLAY_DEVICE_ACTIVE && ddm.StateFlags & DISPLAY_DEVICE_ATTACHED)
				{
					monitor.monitorName.assign(ddm.DeviceString, wcsnlen(ddm.DeviceString, _countof(ddm.DeviceString)));
					break;
				}
			}

			if (dd.StateFlags & DISPLAY_DEVICE_ACTIVE)
			{
				monitor.active = true;

				DEVMODE dm = { 0 };
				dm.dmSize = sizeof(DEVMODE);

				if (EnumDisplaySettings(deviceName.c_str(), ENUM_CURRENT_SETTINGS, &dm))
				{
					POINT pos = {dm.dmPosition.x, dm.dmPosition.y};
					monitor.handle = MonitorFromPoint(pos, MONITOR_DEFAULTTONULL);
				}

				if (monitor.handle != nullptr)
				{
					MONITORINFO info = {sizeof(MONITORINFO)};
					GetMonitorInfo(monitor.handle, &info);

					monitor.screen = info.rcMonitor;
					monitor.work = info.rcWork;
					monitor.dpi = System::GetDpiForMonitor(monitor.handle);
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

			++dwDevice;
		}
		while (EnumDisplayDevices(nullptr, dwDevice, &dd, 0));
	}

	if (monitors.empty())  // Failed to enumerate the non-mirroring monitors
	{
		LogWarning(L"Failed to enumerate the non-mirroring monitors. Only EnumDisplayMonitors is used instead.");
		c_Monitors.useEnumDisplayDevices = false;
		c_Monitors.useEnumDisplayMonitors = true;
	}

	if (c_Monitors.useEnumDisplayMonitors)
	{
		EnumDisplayMonitors(nullptr, nullptr, MyInfoEnumProc, (LPARAM)(&c_Monitors));

		if (monitors.empty())  // Failed to enumerate the monitors
		{
			LogWarning(L"Failed to enumerate monitors. Using dummy monitor info.");
			c_Monitors.useEnumDisplayMonitors = false;

			MonitorInfo monitor;
			monitor.active = true;

			POINT pos = {0, 0};
			monitor.handle = MonitorFromPoint(pos, MONITOR_DEFAULTTOPRIMARY);
			monitor.screen.left = 0;
			monitor.screen.top = 0;
			monitor.screen.right = GetSystemMetrics(SM_CXSCREEN);
			monitor.screen.bottom = GetSystemMetrics(SM_CYSCREEN);
			if (SystemParametersInfo(SPI_GETWORKAREA, 0, &(monitor.work), 0) == 0)  // failed
			{
				monitor.work = monitor.screen;
			}

			monitor.deviceName = L"DUMMY";
			monitor.dpi = System::GetDpiForMonitor(monitor.handle);

			monitors.push_back(monitor);

			c_Monitors.primary = 1;
		}
	}

	c_Monitors.UpdateLogicalMonitorInfo();

	if (GetRainmeter().GetDebug())
	{
		LogDebug(L"------------------------------");

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
		LogDebug(method.c_str());

		LogDebugF(L"* MONITORS: Count=%i, Primary=@%i", (int)monitors.size(), c_Monitors.primary);
		LogDebug(L"@0: Virtual screen");
		LogDebugF(L"  L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
			c_Monitors.vsL, c_Monitors.vsT, c_Monitors.vsL + c_Monitors.vsW, c_Monitors.vsT + c_Monitors.vsH,
			c_Monitors.vsW, c_Monitors.vsH);

		int i = 1;
		for (auto iter = monitors.cbegin(); iter != monitors.cend(); ++iter, ++i)
		{
			if ((*iter).active)
			{
				LogDebugF(L"@%i: %s (active), MonitorName: %s", i, (*iter).deviceName.c_str(), (*iter).monitorName.c_str());
				LogDebugF(L"  L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
					(*iter).screen.left, (*iter).screen.top, (*iter).screen.right, (*iter).screen.bottom,
					(*iter).screen.right - (*iter).screen.left, (*iter).screen.bottom - (*iter).screen.top);
				LogDebugF(L"  WorkArea    : L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
					(*iter).work.left, (*iter).work.top, (*iter).work.right, (*iter).work.bottom,
					(*iter).work.right - (*iter).work.left, (*iter).work.bottom - (*iter).work.top);
				LogDebugF(L"  Dpi         : %u", (*iter).dpi);
			}
			else if ((*iter).monitorName.empty())
			{
				LogDebugF(L"@%i: %s (inactive)", i, (*iter).deviceName.c_str());
			}
			else
			{
				LogDebugF(L"@%i: %s (inactive), MonitorName: %s", i, (*iter).deviceName.c_str(), (*iter).monitorName.c_str());
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
		if ((*iter).active && (*iter).handle != nullptr)
		{
			MONITORINFO info = {sizeof(MONITORINFO)};
			GetMonitorInfo((*iter).handle, &info);

			(*iter).work = info.rcWork;
			(*iter).dpi = System::GetDpiForMonitor((*iter).handle);

			if (GetRainmeter().GetDebug())
			{
				LogDebugF(L"WorkArea@%i : L=%i, T=%i, R=%i, B=%i (W=%i, H=%i)",
					i,
					info.rcWork.left, info.rcWork.top, info.rcWork.right, info.rcWork.bottom,
					info.rcWork.right - info.rcWork.left, info.rcWork.bottom - info.rcWork.top);
			}
		}
	}

	c_Monitors.UpdateLogicalMonitorInfo();
}

RECT MultiMonitorInfo::GetPhysicalVirtualScreenRect() const
{
	return { vsL, vsT, vsL + vsW, vsT + vsH };
}

RECT MultiMonitorInfo::GetLogicalVirtualScreenRect() const
{
	RECT rect = {};
	bool first = true;
	for (size_t i = 0; i < monitors.size(); ++i)
	{
		const auto& monitor = monitors[i];
		if (!monitor.active) continue;

		const RECT monitorRect = monitor.logicalScreen;
		if (first)
		{
			rect = monitorRect;
			first = false;
		}
		else
		{
			rect.left = min(rect.left, monitorRect.left);
			rect.top = min(rect.top, monitorRect.top);
			rect.right = max(rect.right, monitorRect.right);
			rect.bottom = max(rect.bottom, monitorRect.bottom);
		}
	}

	return rect;
}

void MultiMonitorInfo::UpdateLogicalMonitorInfo()
{
	horizontalSpans = CreateLogicalSpans(monitors, primary, true);
	verticalSpans = CreateLogicalSpans(monitors, primary, false);

	for (auto& monitor : monitors)
	{
		if (!monitor.active)
		{
			monitor.logicalScreen = {};
			continue;
		}

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
