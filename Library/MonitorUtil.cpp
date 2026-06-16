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

RECT MultiMonitorInfo::GetPhysicalVirtualScreenRect() const
{
	return { vsL, vsT, vsL + vsW, vsT + vsH };
}

RECT MultiMonitorInfo::GetLogicalVirtualScreenRect() const
{
	RECT rect = {};
	bool first = true;
	for (const auto& monitor : monitors)
	{
		if (!monitor.active) continue;

		const RECT monitorRect = monitor.ToLogical(monitor.screen);
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
