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
	uint8_t deviceNumber;
	uint8_t displayNumber;
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
	int primary;
	int deviceCount;
	int displayCount;
	std::vector<MonitorInfo> monitors;

	RECT virtualScreen;
	RECT logicalVirtualScreen;

	void Clear();

	int GetDeviceCount() const { return deviceCount; }
	int GetDisplayCount() const { return displayCount; }
	const MonitorInfo* GetByDeviceNumber(int deviceNumber) const;
	const MonitorInfo* GetByDisplayNumber(int activeNumber) const;
	const MonitorInfo* GetForWindow(HWND window) const;

	void UpdateSpans();

	POINT PhysicalToLogical(POINT point, UINT dpiOverride = 0) const;
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

	static LONG ConvertPhysicalToLogical(LONG value, const std::vector<Span>& spans, UINT dpiOverride = 0);
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

}  // namespace MonitorUtil

#endif
