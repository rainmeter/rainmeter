/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MonitorUtil.h"

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

POINT MultiMonitorInfo::LogicalToPhysical(POINT point) const
{
	for (size_t i = 0; i < monitors.size(); ++i)
	{
		const auto& monitor = monitors[i];
		if (monitor.active && Contains(monitor.logicalScreen, point))
		{
			return {
				monitor.screen.left + MulDiv(point.x - monitor.logicalScreen.left, monitor.dpi, USER_DEFAULT_SCREEN_DPI),
				monitor.screen.top + MulDiv(point.y - monitor.logicalScreen.top, monitor.dpi, USER_DEFAULT_SCREEN_DPI)
			};
		}
	}

	return {
		ConvertLogicalToPhysical(point.x, horizontalSpans),
		ConvertLogicalToPhysical(point.y, verticalSpans)
	};
}
