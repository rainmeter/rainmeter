// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <windows.h>
#include <string>
#include <vector>

struct MonitorInfo
{
	bool active = false;
	HMONITOR handle = nullptr;
	uint8_t deviceNumber = 0;
	uint8_t displayNumber = 0;
	UINT dpi = 0;
	RECT screen = {};
	RECT logicalScreen = {};
	RECT work = {};
	RECT logicalWork = {};
	std::wstring deviceName;				// Device name (E.g. "\\.\DISPLAY1")
	std::wstring monitorName;				// Monitor name (E.g. "Generic Non-PnP Monitor")
};

struct MultiMonitorInfo
{
	int primary = 0;
	int deviceCount = 0;
	int displayCount = 0;
	std::vector<MonitorInfo> monitors;

	RECT virtualScreen = {};
	RECT logicalVirtualScreen = {};

	void Clear();

	int GetDeviceCount() const { return deviceCount; }
	int GetDisplayCount() const { return displayCount; }
	const MonitorInfo* GetByDeviceNumber(int deviceNumber) const;
	const MonitorInfo* GetByDisplayNumber(int activeNumber) const;
	const MonitorInfo* GetByHandle(HMONITOR monitorHandle) const;
	const MonitorInfo* GetForWindow(HWND window) const;
	const MonitorInfo* GetFromPoint(POINT point) const;
};

namespace MonitorUtil {

void InitializeMultiMonitorInfo();
void EnableDpiAppCompatMode();
const MultiMonitorInfo& GetMultiMonitorInfo();
void ClearMultiMonitorInfo();
void UpdateWorkareaInfo();

}  // namespace MonitorUtil
