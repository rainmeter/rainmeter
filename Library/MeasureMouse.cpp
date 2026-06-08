/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureMouse.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "Rainmeter.h"
#include "Skin.h"

MeasureMouse::MeasureMouse(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_RelativeToSkin(true),
	m_RequireDragging(false),
	m_Capturing(false),
	m_Delay(16),
	m_LastMoveActionTime()
{
}

MeasureMouse::~MeasureMouse()
{
	if (m_Capturing)
	{
		m_Capturing = false;
		GetSkin()->UpdateMouseMeasureCapture();
	}
}

void MeasureMouse::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	const bool wasCapturing = WantsCapture();

	Measure::ReadOptions(parser, section);

	m_Actions[MOUSE_LMB_UP] = parser.ReadString(section, L"LeftMouseUpAction", L"", false);
	m_Actions[MOUSE_LMB_DOWN] = parser.ReadString(section, L"LeftMouseDownAction", L"", false);
	m_Actions[MOUSE_LMB_DBLCLK] = parser.ReadString(section, L"LeftMouseDoubleClickAction", L"", false);
	m_Actions[MOUSE_MMB_UP] = parser.ReadString(section, L"MiddleMouseUpAction", L"", false);
	m_Actions[MOUSE_MMB_DOWN] = parser.ReadString(section, L"MiddleMouseDownAction", L"", false);
	m_Actions[MOUSE_MMB_DBLCLK] = parser.ReadString(section, L"MiddleMouseDoubleClickAction", L"", false);
	m_Actions[MOUSE_RMB_UP] = parser.ReadString(section, L"RightMouseUpAction", L"", false);
	m_Actions[MOUSE_RMB_DOWN] = parser.ReadString(section, L"RightMouseDownAction", L"", false);
	m_Actions[MOUSE_RMB_DBLCLK] = parser.ReadString(section, L"RightMouseDoubleClickAction", L"", false);
	m_Actions[MOUSE_X1MB_UP] = parser.ReadString(section, L"X1MouseUpAction", L"", false);
	m_Actions[MOUSE_X1MB_DOWN] = parser.ReadString(section, L"X1MouseDownAction", L"", false);
	m_Actions[MOUSE_X1MB_DBLCLK] = parser.ReadString(section, L"X1MouseDoubleClickAction", L"", false);
	m_Actions[MOUSE_X2MB_UP] = parser.ReadString(section, L"X2MouseUpAction", L"", false);
	m_Actions[MOUSE_X2MB_DOWN] = parser.ReadString(section, L"X2MouseDownAction", L"", false);
	m_Actions[MOUSE_X2MB_DBLCLK] = parser.ReadString(section, L"X2MouseDoubleClickAction", L"", false);

	m_Actions[MOUSE_MW_UP] = parser.ReadString(section, L"MouseScrollUpAction", L"", false);
	m_Actions[MOUSE_MW_DOWN] = parser.ReadString(section, L"MouseScrollDownAction", L"", false);
	m_Actions[MOUSE_MW_LEFT] = parser.ReadString(section, L"MouseScrollLeftAction", L"", false);
	m_Actions[MOUSE_MW_RIGHT] = parser.ReadString(section, L"MouseScrollRightAction", L"", false);

	m_MouseMoveAction = parser.ReadString(section, L"MouseMoveAction", L"", false);
	m_LeftDragAction = parser.ReadString(section, L"LeftMouseDragAction", L"", false);
	m_MiddleDragAction = parser.ReadString(section, L"MiddleMouseDragAction", L"", false);
	m_RightDragAction = parser.ReadString(section, L"RightMouseDragAction", L"", false);
	m_X1DragAction = parser.ReadString(section, L"X1MouseDragAction", L"", false);
	m_X2DragAction = parser.ReadString(section, L"X2MouseDragAction", L"", false);

	m_RelativeToSkin = parser.ReadBool(section, L"RelativeToSkin", true);
	m_RequireDragging = parser.ReadBool(section, L"RequireDragging", false);
	m_Delay = parser.ReadUInt(section, L"Delay", 16U);

	if (!m_RequireDragging)
	{
		m_Capturing = false;
	}

	if (wasCapturing != WantsCapture())
	{
		GetSkin()->UpdateMouseMeasureCapture();
	}

	if (!m_Actions[MOUSE_MW_UP].empty() ||
		!m_Actions[MOUSE_MW_DOWN].empty() ||
		!m_Actions[MOUSE_MW_LEFT].empty() ||
		!m_Actions[MOUSE_MW_RIGHT].empty())
	{
		GetSkin()->SetHasMouseScrollAction();
	}
}

void MeasureMouse::Command(const std::wstring& command)
{
	if (m_RequireDragging)
	{
		if (_wcsicmp(command.c_str(), L"Start") == 0)
		{
			m_Capturing = true;
			GetSkin()->UpdateMouseMeasureCapture();
			return;
		}
		else if (_wcsicmp(command.c_str(), L"Stop") == 0)
		{
			m_Capturing = false;
			GetSkin()->UpdateMouseMeasureCapture();
			return;
		}
	}

	LogWarningF(this, L"!CommandMeasure: Not supported");
}

bool MeasureMouse::IsActive()
{
	if (IsDisabled() || IsPaused())
	{
		return false;
	}

	return !m_RequireDragging || m_Capturing;
}

bool MeasureMouse::ShouldRunMoveAction()
{
	const ULONGLONG now = GetTickCount64();
	if (m_LastMoveActionTime != 0 && now - m_LastMoveActionTime < m_Delay)
	{
		return false;
	}

	m_LastMoveActionTime = now;
	return true;
}

bool MeasureMouse::ExecuteAction(MOUSEACTION action, POINT logicalPos, POINT screenPos, MOUSEACTION fallback)
{
	if (!IsActive())
	{
		return false;
	}

	std::wstring command = m_Actions[action];
	if (command.empty() && fallback != MOUSEACTION_COUNT)
	{
		command = m_Actions[fallback];
	}

	if (!command.empty())
	{
		ReplaceMouseVariables(command, logicalPos, screenPos);
		GetRainmeter().ExecuteActionCommand(command.c_str(), this);
		return true;
	}

	return false;
}

void MeasureMouse::ExecuteMoveActions(POINT logicalPos, POINT screenPos)
{
	if (!IsActive() || !ShouldRunMoveAction())
	{
		return;
	}

	auto execute = [&](const std::wstring& action)
	{
		if (!action.empty())
		{
			std::wstring command = action;
			ReplaceMouseVariables(command, logicalPos, screenPos);
			GetRainmeter().ExecuteActionCommand(command.c_str(), this);
		}
	};

	execute(m_MouseMoveAction);

	if (GetKeyState(VK_LBUTTON) < 0)
	{
		execute(m_LeftDragAction);
	}
	if (GetKeyState(VK_MBUTTON) < 0)
	{
		execute(m_MiddleDragAction);
	}
	if (GetKeyState(VK_RBUTTON) < 0)
	{
		execute(m_RightDragAction);
	}
	if (GetKeyState(VK_XBUTTON1) < 0)
	{
		execute(m_X1DragAction);
	}
	if (GetKeyState(VK_XBUTTON2) < 0)
	{
		execute(m_X2DragAction);
	}
}

void MeasureMouse::ReplaceMouseVariables(std::wstring& result, POINT logicalPos, POINT screenPos) const
{
	const auto& pos = m_RelativeToSkin ? logicalPos : screenPos;

	WCHAR mouseX[32] = { 0 };
	WCHAR mouseY[32] = { 0 };
	_itow_s(pos.x, mouseX, 10);
	_itow_s(pos.y, mouseY, 10);
	const size_t mouseXLength = wcslen(mouseX);
	const size_t mouseYLength = wcslen(mouseY);

	size_t start = 0;
	do
	{
		start = result.find(L'$', start);
		if (start == std::wstring::npos) break;

		const size_t variableStart = start + 1;
		const size_t end = result.find(L'$', variableStart);
		if (end == std::wstring::npos) break;

		// Escaped variable.
		const size_t variableEnd = end - 1;
		if (variableStart != variableEnd && result[variableStart] == L'*' && result[variableEnd] == L'*')
		{
			result.erase(variableEnd, 1);
			result.erase(variableStart, 1);
			start = variableEnd;
			continue;
		}

		const WCHAR* replacement = nullptr;
		size_t replacementLength = 0;
		const WCHAR* variable = result.c_str() + variableStart;
		if (_wcsnicmp(variable, L"MOUSEX", 6) == 0)
		{
			replacement = mouseX;
			replacementLength = mouseXLength;
		}
		else if (_wcsnicmp(variable, L"MOUSEY", 6) == 0)
		{
			replacement = mouseY;
			replacementLength = mouseYLength;
		}

		if (replacement)
		{
			result.replace(start, end - start + 1, replacement);
			start += replacementLength;
		}
		else
		{
			start = end;
		}
	}
	while (true);
}
