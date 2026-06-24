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
	RECT logicalScreen;
	RECT work;
	RECT logicalWork;
	std::wstring deviceName;				// Device name (E.g. "\\.\DISPLAY1")
	std::wstring monitorName;				// Monitor name (E.g. "Generic Non-PnP Monitor")
};

struct MultiMonitorInfo
{
	int primary;							// Index of the primary monitor
	std::vector<MonitorInfo> monitors;

	RECT virtualScreen;
	RECT logicalVirtualScreen;

	void Clear();

	void UpdateSpans();
	POINT PhysicalToLogical(POINT point) const;
	POINT LogicalToPhysical(POINT point, UINT* dpi = nullptr) const;

private:
	struct Span
	{
		LONG physicalStart;
		LONG physicalEnd;
		LONG logicalStart;
		LONG logicalEnd;
		UINT dpi;
	};

	static LONG ConvertPhysicalToLogical(LONG value, const std::vector<Span>& spans);
	static LONG ConvertLogicalToPhysical(LONG value, const std::vector<Span>& spans);
	static std::vector<Span> CreateLogicalSpans(const std::vector<MonitorInfo>& monitors, int primary, bool horizontal);

	std::vector<Span> horizontalSpans;
	std::vector<Span> verticalSpans;
};

namespace MonitorUtil {

void InitializeMultiMonitorInfo();
void EnableDpiAppCompatMode();
const MultiMonitorInfo& GetMultiMonitorInfo();
void ClearMultiMonitorInfo();
void UpdateWorkareaInfo();

class DpiUnawareScope
{
public:
	DpiUnawareScope() : m_PreviousContext(SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE)) {}
	DpiUnawareScope(const DpiUnawareScope&) = delete;
	DpiUnawareScope& operator=(const DpiUnawareScope&) = delete;

	~DpiUnawareScope()
	{
		SetThreadDpiAwarenessContext(m_PreviousContext);
	}

private:
	DPI_AWARENESS_CONTEXT m_PreviousContext;
};

}  // namespace MonitorUtil

#endif
