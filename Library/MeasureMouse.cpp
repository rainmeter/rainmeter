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
#include "MonitorUtil.h"
#include "Rainmeter.h"
#include "Skin.h"

namespace {

HHOOK g_MouseHook = nullptr;
std::vector<MeasureMouse*> g_Measures;

POINT GetLogicalScreenPos(POINT screenPos)
{
	return MonitorUtil::GetMultiMonitorInfo().PhysicalToLogical(screenPos);
}

POINT GetLogicalSkinPos(Skin* skin, POINT screenPos)
{
	POINT clientPos = screenPos;
	ScreenToClient(skin->GetWindow(), &clientPos);
	return skin->PhysicalToLogical(clientPos);
}

template<typename Func>
void ForEachMeasure(HWND window, Func&& func)
{
	const auto measuresCopy = g_Measures;
	for (auto* measure : measuresCopy)
	{
		if (std::find(g_Measures.begin(), g_Measures.end(), measure) == g_Measures.end())
		{
			continue;
		}

		Skin* skin = measure->GetSkin();
		if (skin->GetWindow() == window)
		{
			func(measure, skin);
		}
	}
}

void ExecuteRegularAction(HWND window, POINT screenPos, MOUSEACTION action)
{
	const POINT logicalScreenPos = GetLogicalScreenPos(screenPos);
	ForEachMeasure(window, [&](MeasureMouse* measure, Skin* skin)
	{
		measure->ExecuteAction(action, GetLogicalSkinPos(skin, screenPos), logicalScreenPos);
	});
}

void ExecuteDoubleClickAction(HWND window, POINT screenPos, MOUSEACTION action, MOUSEACTION fallback)
{
	const POINT logicalScreenPos = GetLogicalScreenPos(screenPos);
	ForEachMeasure(window, [&](MeasureMouse* measure, Skin* skin)
	{
		measure->ExecuteAction(action, GetLogicalSkinPos(skin, screenPos), logicalScreenPos, fallback);
	});
}

void ExecuteMoveActions(HWND window, POINT screenPos)
{
	const POINT logicalScreenPos = GetLogicalScreenPos(screenPos);
	ForEachMeasure(window, [&](MeasureMouse* measure, Skin* skin)
	{
		measure->ExecuteMoveActions(GetLogicalSkinPos(skin, screenPos), logicalScreenPos);
	});
}

void ExecuteXButtonAction(HWND window, POINT screenPos, DWORD mouseData, MOUSEACTION x1Action, MOUSEACTION x2Action)
{
	const WORD button = HIWORD(mouseData);
	if (button == XBUTTON1)
	{
		ExecuteRegularAction(window, screenPos, x1Action);
	}
	else if (button == XBUTTON2)
	{
		ExecuteRegularAction(window, screenPos, x2Action);
	}
}

void ExecuteXButtonDoubleClickAction(HWND window, POINT screenPos, DWORD mouseData)
{
	const WORD button = HIWORD(mouseData);
	if (button == XBUTTON1)
	{
		ExecuteDoubleClickAction(window, screenPos, MOUSE_X1MB_DBLCLK, MOUSE_X1MB_DOWN);
	}
	else if (button == XBUTTON2)
	{
		ExecuteDoubleClickAction(window, screenPos, MOUSE_X2MB_DBLCLK, MOUSE_X2MB_DOWN);
	}
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
	{
		return CallNextHookEx(g_MouseHook, nCode, wParam, lParam);
	}

	const auto* data = (const MOUSEHOOKSTRUCTEX*)lParam;
	const UINT msg = (UINT)wParam;
	const HWND window = data->hwnd;
	const POINT screenPos = data->pt;

	switch (msg)
	{
	case WM_NCMOUSEMOVE:
	case WM_MOUSEMOVE:
		ExecuteMoveActions(window, screenPos);
		break;

	case WM_MOUSEWHEEL:
		ExecuteRegularAction(window, screenPos, GET_WHEEL_DELTA_WPARAM(data->mouseData) < 0 ? MOUSE_MW_DOWN : MOUSE_MW_UP);
		break;

	case WM_MOUSEHWHEEL:
		ExecuteRegularAction(window, screenPos, GET_WHEEL_DELTA_WPARAM(data->mouseData) < 0 ? MOUSE_MW_LEFT : MOUSE_MW_RIGHT);
		break;

	case WM_NCLBUTTONDOWN:
	case WM_LBUTTONDOWN:
		ExecuteRegularAction(window, screenPos, MOUSE_LMB_DOWN);
		break;

	case WM_NCLBUTTONUP:
	case WM_LBUTTONUP:
		ExecuteRegularAction(window, screenPos, MOUSE_LMB_UP);
		break;

	case WM_NCLBUTTONDBLCLK:
	case WM_LBUTTONDBLCLK:
		ExecuteDoubleClickAction(window, screenPos, MOUSE_LMB_DBLCLK, MOUSE_LMB_DOWN);
		break;

	case WM_NCMBUTTONDOWN:
	case WM_MBUTTONDOWN:
		ExecuteRegularAction(window, screenPos, MOUSE_MMB_DOWN);
		break;

	case WM_NCMBUTTONUP:
	case WM_MBUTTONUP:
		ExecuteRegularAction(window, screenPos, MOUSE_MMB_UP);
		break;

	case WM_NCMBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
		ExecuteDoubleClickAction(window, screenPos, MOUSE_MMB_DBLCLK, MOUSE_MMB_DOWN);
		break;

	case WM_NCRBUTTONDOWN:
	case WM_RBUTTONDOWN:
		ExecuteRegularAction(window, screenPos, MOUSE_RMB_DOWN);
		break;

	case WM_NCRBUTTONUP:
	case WM_RBUTTONUP:
		ExecuteRegularAction(window, screenPos, MOUSE_RMB_UP);
		break;

	case WM_NCRBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
		ExecuteDoubleClickAction(window, screenPos, MOUSE_RMB_DBLCLK, MOUSE_RMB_DOWN);
		break;

	case WM_NCXBUTTONDOWN:
	case WM_XBUTTONDOWN:
		ExecuteXButtonAction(window, screenPos, data->mouseData, MOUSE_X1MB_DOWN, MOUSE_X2MB_DOWN);
		break;

	case WM_NCXBUTTONUP:
	case WM_XBUTTONUP:
		ExecuteXButtonAction(window, screenPos, data->mouseData, MOUSE_X1MB_UP, MOUSE_X2MB_UP);
		break;

	case WM_NCXBUTTONDBLCLK:
	case WM_XBUTTONDBLCLK:
		ExecuteXButtonDoubleClickAction(window, screenPos, data->mouseData);
		break;
	}

	return CallNextHookEx(g_MouseHook, nCode, wParam, lParam);
}

}  // namespace

MeasureMouse::MeasureMouse(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_RelativeToSkin(true),
	m_RequireDragging(false),
	m_Capturing(false),
	m_Delay(16),
	m_LastMoveActionTime()
{
	g_Measures.push_back(this);

	if (!g_MouseHook)
	{
		g_MouseHook = SetWindowsHookEx(WH_MOUSE, MouseProc, nullptr, GetCurrentThreadId());
	}
}

MeasureMouse::~MeasureMouse()
{
	if (m_Capturing)
	{
		m_Capturing = false;
		GetSkin()->UpdateMouseMeasureCapture();
	}

	auto iter = std::find(g_Measures.begin(), g_Measures.end(), this);
	if (iter != g_Measures.end())
	{
		g_Measures.erase(iter);
	}

	if (g_Measures.empty() && g_MouseHook)
	{
		UnhookWindowsHookEx(g_MouseHook);
		g_MouseHook = nullptr;
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
	return m_RequireDragging ? m_Capturing : (!IsDisabled() && !IsPaused());
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

bool MeasureMouse::ExecuteAction(MOUSEACTION action, POINT logicalSkinPos, POINT logicalScreenPos, MOUSEACTION fallback)
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
		ReplaceMouseVariables(command, logicalSkinPos, logicalScreenPos);
		GetRainmeter().ExecuteActionCommand(command.c_str(), this);
		return true;
	}

	return false;
}

void MeasureMouse::ExecuteMoveActions(POINT logicalSkinPos, POINT logicalScreenPos)
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
			ReplaceMouseVariables(command, logicalSkinPos, logicalScreenPos);
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

void MeasureMouse::ReplaceMouseVariables(std::wstring& result, POINT logicalSkinPos, POINT logicalScreenPos) const
{
	WCHAR mouseX[32] = { 0 };
	WCHAR mouseY[32] = { 0 };

	const auto& pos = m_RelativeToSkin ? logicalSkinPos : logicalScreenPos;
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
