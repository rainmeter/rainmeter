/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "SkinPosition.h"
#include "System.h"

void SkinPosition::ParseAnchorOption(int windowSize, float zoom)
{
	const auto anchorNumberEndPos = anchorOption.find_first_not_of(L"0123456789.");
	auto anchorNumber = (float)_wtof(anchorOption.substr(0, anchorNumberEndPos).c_str());

	anchorPercentage = anchorOption.find_last_of(L'%') != std::wstring::npos;
	anchorNumber = anchorPercentage ? (windowSize * zoom) * (anchorNumber / 100.0f) : (anchorNumber * zoom);

	anchorFromOpposite = anchorOption.find_last_of(oppositeChar) != std::wstring::npos;
	anchorNumber = anchorFromOpposite ? (windowSize * zoom) - anchorNumber : anchorNumber;
	anchorPos = (int)anchorNumber;
}

float SkinPosition::ParseWindowOption(const std::vector<MonitorInfo>& monitors)
{
	monitor.reset();

	const auto numberEndPos = option.find_first_not_of(L"-0123456789.");
	const auto number = (float)_wtof(option.substr(0, numberEndPos).c_str());

	// Accept modifiers only after the hash of potentially present variables.
	const auto percentagePos = option.find_last_of(L'%');
	const auto hashPos = option.find_last_of(L'#');
	percentage = percentagePos != std::wstring::npos && (hashPos == std::wstring::npos || hashPos < percentagePos);

	const auto oppositePos = option.find_last_of(oppositeChar);
	fromOpposite = oppositePos != std::wstring::npos && (hashPos == std::wstring::npos || hashPos < oppositePos);

	const auto atPos = option.find_last_of(L'@');
	if (atPos != std::wstring::npos && (hashPos == std::wstring::npos || hashPos < atPos))
	{
		const auto monitorNumberEndPos = option.find_first_not_of(L"0123456789", atPos + 1);
		const auto monitorNumberString = option.substr(atPos + 1, (monitorNumberEndPos != std::wstring::npos) ? monitorNumberEndPos - atPos - 1 : std::wstring::npos);
		if (!monitorNumberString.empty())
		{
			const int monitorNumber = _wtoi(monitorNumberString.c_str());
			if (monitorNumber >= 0 && (monitorNumber == 0 || monitorNumber <= (int)monitors.size() && monitors[monitorNumber - 1].active))
			{
				monitor = monitorNumber;
			}
		}
	}

	return number;
}

void SkinPosition::UpdateOptionValue(int logicalPos, int referenceOrigin, int referenceExtent)
{
	if (fromOpposite)
	{
		logicalPos = referenceOrigin + referenceExtent - logicalPos;
		logicalPos -= anchorPos;
	}
	else
	{
		logicalPos = logicalPos - referenceOrigin;
		logicalPos += anchorPos;
	}

	WCHAR buffer[64] = { 0 };
	if (percentage)
	{
		const float number = 100.0f * logicalPos / referenceExtent;
		_snwprintf_s(buffer, _TRUNCATE, L"%.5f%%", number);
	}
	else
	{
		_itow_s(logicalPos, buffer, 10);
	}

	option = buffer;

	if (fromOpposite)
	{
		option += oppositeChar;
	}

	if (monitor)
	{
		option += L'@';

		_itow_s(*monitor, buffer, 10);
		option += buffer;
	}
}

int SkinPosition::ResolveLogicalPosition(float parsedValue, int referenceOrigin, int referenceExtent)
{
	const int offsetFromEdge = percentage ? (int)(referenceExtent * (parsedValue / 100.0f)) : (int)parsedValue;
	const int offsetFromOrigin = fromOpposite ? referenceExtent - offsetFromEdge : offsetFromEdge;
	return referenceOrigin + offsetFromOrigin - anchorPos;
}

UINT SkinPosition::ResolvePhysicalPosition(SkinPosition& x, SkinPosition& y, int w, int h, float zoom, const MultiMonitorInfo& monitorsInfo)
{
	const auto& monitors = monitorsInfo.monitors;

	x.ParseAnchorOption(w, zoom);
	y.ParseAnchorOption(h, zoom);
	const float parsedX = x.ParseWindowOption(monitors);
	const float parsedY = y.ParseWindowOption(monitors);

	if (x.monitor.has_value() && !y.monitor.has_value())
	{
		y.monitor = x.monitor;
	}
	else if (!x.monitor.has_value() && y.monitor.has_value())
	{
		x.monitor = y.monitor;
	}

	const int monitorX = x.monitor.value_or(monitorsInfo.primary);
	const auto& monitorRectX = monitorX == 0 ? monitorsInfo.logicalVirtualScreen : monitors[monitorX - 1].logicalScreen;
	const int logicalX = x.ResolveLogicalPosition(parsedX, monitorRectX.left, monitorRectX.right - monitorRectX.left);

	const int monitorY = y.monitor.value_or(monitorsInfo.primary);
	const auto& monitorRectY = monitorY == 0 ? monitorsInfo.logicalVirtualScreen : monitors[monitorY - 1].logicalScreen;
	const int logicalY = y.ResolveLogicalPosition(parsedY, monitorRectY.top, monitorRectY.bottom - monitorRectY.top);

	UINT dpi = 0;
	const auto physicalPos = monitorsInfo.LogicalToPhysical({ logicalX, logicalY }, &dpi);
	x.pos = physicalPos.x;
	y.pos = physicalPos.y;

	return dpi > 0 ? dpi : System::GetSystemDpi();
}
