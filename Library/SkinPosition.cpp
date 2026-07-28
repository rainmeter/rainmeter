/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "SkinPosition.h"
#include "Rainmeter.h"
#include "System.h"

void SkinPositionOption::ParseAnchorOption(int windowSize, float zoom)
{
	const auto anchorNumberEndPos = anchorOption.find_first_not_of(L"0123456789.");
	auto anchorNumber = (float)_wtof(anchorOption.substr(0, anchorNumberEndPos).c_str());

	anchorPercentage = anchorOption.find_last_of(L'%') != std::wstring::npos;
	anchorNumber = anchorPercentage ? (windowSize * zoom) * (anchorNumber / 100.0f) : (anchorNumber * zoom);

	anchorFromOpposite = anchorOption.find_last_of(oppositeChar) != std::wstring::npos;
	anchorNumber = anchorFromOpposite ? (windowSize * zoom) - anchorNumber : anchorNumber;
	anchorPos = (int)anchorNumber;
}

float SkinPositionOption::ParseWindowOption(const std::vector<MonitorInfo>& monitors)
{
	monitor.reset();

	const auto numberEndPos = windowOption.find_first_not_of(L"-0123456789.");
	const auto number = (float)_wtof(windowOption.substr(0, numberEndPos).c_str());

	// Accept modifiers only after the hash of potentially present variables.
	const auto percentagePos = windowOption.find_last_of(L'%');
	const auto hashPos = windowOption.find_last_of(L'#');
	percentage = percentagePos != std::wstring::npos && (hashPos == std::wstring::npos || hashPos < percentagePos);

	const auto oppositePos = windowOption.find_last_of(oppositeChar);
	fromOpposite = oppositePos != std::wstring::npos && (hashPos == std::wstring::npos || hashPos < oppositePos);

	const auto atPos = windowOption.find_last_of(L'@');
	if (atPos != std::wstring::npos && (hashPos == std::wstring::npos || hashPos < atPos))
	{
		const auto monitorNumberEndPos = windowOption.find_first_not_of(L"0123456789", atPos + 1);
		const auto monitorNumberString = windowOption.substr(atPos + 1, (monitorNumberEndPos != std::wstring::npos) ? monitorNumberEndPos - atPos - 1 : std::wstring::npos);
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

void SkinPositionOption::UpdateOptionValue(int logicalPos, int referenceOrigin, int referenceExtent)
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

	windowOption = buffer;

	if (fromOpposite)
	{
		windowOption += oppositeChar;
	}

	if (monitor)
	{
		windowOption += L'@';

		_itow_s(*monitor, buffer, 10);
		windowOption += buffer;
	}
}

int SkinPositionOption::ResolveLogicalPosition(float parsedValue, int referenceOrigin, int referenceExtent)
{
	const int offsetFromEdge = percentage ? (int)(referenceExtent * (parsedValue / 100.0f)) : (int)parsedValue;
	const int offsetFromOrigin = fromOpposite ? referenceExtent - offsetFromEdge : offsetFromEdge;
	return referenceOrigin + offsetFromOrigin - anchorPos;
}

POINT SkinPosition::ResolveVirtualizedPosition(int w, int h, float zoom, const MultiMonitorInfo& monitorsInfo)
{
	const auto& monitors = monitorsInfo.monitors;

	m_X.ParseAnchorOption(w, zoom);
	m_Y.ParseAnchorOption(h, zoom);
	const float parsedX = m_X.ParseWindowOption(monitors);
	const float parsedY = m_Y.ParseWindowOption(monitors);

	if (m_X.monitor.has_value() && !m_Y.monitor.has_value())
	{
		m_Y.monitor = m_X.monitor;
	}
	else if (!m_X.monitor.has_value() && m_Y.monitor.has_value())
	{
		m_X.monitor = m_Y.monitor;
	}

	const int monitorX = m_X.monitor.value_or(monitorsInfo.primary);
	const auto& monitorRectX = monitorX == 0 ? monitorsInfo.logicalVirtualScreen : monitors[monitorX - 1].logicalScreen;
	const int logicalX = m_X.ResolveLogicalPosition(parsedX, monitorRectX.left, monitorRectX.right - monitorRectX.left);

	const int monitorY = m_Y.monitor.value_or(monitorsInfo.primary);
	const auto& monitorRectY = monitorY == 0 ? monitorsInfo.logicalVirtualScreen : monitors[monitorY - 1].logicalScreen;
	const int logicalY = m_Y.ResolveLogicalPosition(parsedY, monitorRectY.top, monitorRectY.bottom - monitorRectY.top);

	return { logicalX, logicalY };
}

SkinPosition::SkinPosition() :
	m_X(L'R'),
	m_Y(L'B')
{
}

POINT SkinPosition::AsPhysical(SIZE windowSize) const
{
	if (m_Space == SkinPositionSpace::Physical || GetRainmeter().HasExeDpiOverride()) return m_Pos;
	if (!m_ConvertedPos)
	{
		m_ConvertedPos = System::ConvertVirtualizedToPhysicalPosition(m_Pos, windowSize);
	}
	return *m_ConvertedPos;
}

POINT SkinPosition::AsVirtualized(HMONITOR monitor) const
{
	if (m_Space == SkinPositionSpace::Virtualized || GetRainmeter().HasExeDpiOverride()) return m_Pos;
	if (!m_ConvertedPos)
	{
		m_ConvertedPos = System::ConvertPhysicalToVirtualizedPosition(m_Pos, monitor);
	}
	return *m_ConvertedPos;
}

void SkinPosition::SetPhysical(POINT position)
{
	m_Pos = position;
	ResetCache();
	m_Space = SkinPositionSpace::Physical;
}

void SkinPosition::SetVirtualized(POINT position)
{
	m_Pos = position;
	ResetCache();
	m_Space = SkinPositionSpace::Virtualized;
}
