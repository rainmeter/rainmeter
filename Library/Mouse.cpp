/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "ConfigParser.h"
#include "Skin.h"
#include "Meter.h"
#include "Logger.h"
#include "Mouse.h"

const struct { const WCHAR* name; MOUSEACTION type; } g_MouseActionTable[] =
{
	{ L"LeftMouseUpAction", MOUSE_LMB_UP },
	{ L"LeftMouseDownAction", MOUSE_LMB_DOWN },
	{ L"LeftMouseDoubleClickAction", MOUSE_LMB_DBLCLK },
	{ L"MiddleMouseUpAction", MOUSE_MMB_UP },
	{ L"MiddleMouseDownAction", MOUSE_MMB_DOWN },
	{ L"MiddleMouseDoubleClickAction", MOUSE_MMB_DBLCLK },
	{ L"RightMouseUpAction", MOUSE_RMB_UP },
	{ L"RightMouseDownAction", MOUSE_RMB_DOWN },
	{ L"RightMouseDoubleClickAction", MOUSE_RMB_DBLCLK },
	{ L"X1MouseUpAction", MOUSE_X1MB_UP },
	{ L"X1MouseDownAction", MOUSE_X1MB_DOWN },
	{ L"X1MouseDoubleClickAction", MOUSE_X1MB_DBLCLK },
	{ L"X2MouseUpAction", MOUSE_X2MB_UP },
	{ L"X2MouseDownAction", MOUSE_X2MB_DOWN },
	{ L"X2MouseDoubleClickAction", MOUSE_X2MB_DBLCLK },
	{ L"MouseScrollUpAction", MOUSE_MW_UP },
	{ L"MouseScrollDownAction", MOUSE_MW_DOWN },
	{ L"MouseScrollLeftAction", MOUSE_MW_LEFT },
	{ L"MouseScrollRightAction", MOUSE_MW_RIGHT },
	{ L"MouseOverAction", MOUSE_OVER },
	{ L"MouseLeaveAction", MOUSE_LEAVE },
};

const WCHAR* OptionNameForMouseActionType(MOUSEACTION type)
{
	for (const auto& entry : g_MouseActionTable)
	{
		if (entry.type == type) return entry.name;
	}
	return nullptr;
}

Mouse::Mouse(Skin* skin, Meter* meter) : m_Skin(skin), m_Meter(meter),
	m_CursorType(MOUSECURSOR_HAND),
	m_CustomCursor(),
	m_CursorState(true),
	m_MouseActionTypes(0)
{
}

Mouse::~Mouse()
{
	DestroyCustomCursor();
}

void Mouse::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	DestroyCustomCursor();

	m_MouseActionTypes = 0;
	for (auto& mouseAction : m_MouseActions)
	{
		m_MouseActionTypes |= mouseAction.type;
		mouseAction.action = parser.ReadString(section, OptionNameForMouseActionType(mouseAction.type), L"", false);
	}

	for (auto& entry : g_MouseActionTable)
	{
		if (m_MouseActionTypes & entry.type) continue;

		const std::wstring& action = parser.ReadString(section, OptionNameForMouseActionType(entry.type), L"", false);
		if (parser.GetLastDefaultUsed()) continue;

		m_MouseActionTypes |= entry.type;
		auto& mouseAction = m_MouseActions.emplace_back();
		mouseAction.type = entry.type;
		mouseAction.action = action;
	}

	if (HasScrollAction())
	{
		m_Skin->SetHasMouseScrollAction();
	}

	const bool defaultState = (section == L"Rainmeter") ? true : m_Skin->GetMouse().GetCursorState();
	m_CursorState = parser.ReadBool(section, L"MouseActionCursor", defaultState);

	const WCHAR* defaultMouseCursor = (section == L"Rainmeter") ? L"HAND" : L"";
	const WCHAR* mouseCursor = parser.ReadString(section, L"MouseActionCursorName", defaultMouseCursor).c_str();

	auto inheritSkinDefault = [&]()
	{
		// Inherit from [Rainmeter].
		m_CursorType = m_Skin->GetMouse().GetCursorType();
		if (m_CursorType == MOUSECURSOR_CUSTOM)
		{
			mouseCursor = m_Skin->GetParser().ReadString(L"Rainmeter", L"MouseActionCursorName", L"").c_str();
		}
	};

	if (*mouseCursor == L'\0')  // meters' default
	{
		inheritSkinDefault();
	}
	else if (_wcsicmp(mouseCursor, L"HAND") == 0)  // skin's default
	{
		m_CursorType = MOUSECURSOR_HAND;
	}
	else if (_wcsicmp(mouseCursor, L"TEXT") == 0)
	{
		m_CursorType = MOUSECURSOR_TEXT;
	}
	else if (_wcsicmp(mouseCursor, L"HELP") == 0)
	{
		m_CursorType = MOUSECURSOR_HELP;
	}
	else if (_wcsicmp(mouseCursor, L"BUSY") == 0)
	{
		m_CursorType = MOUSECURSOR_BUSY;
	}
	else if (_wcsicmp(mouseCursor, L"CROSS") == 0)
	{
		m_CursorType = MOUSECURSOR_CROSS;
	}
	else if (_wcsicmp(mouseCursor, L"PEN") == 0)
	{
		m_CursorType = MOUSECURSOR_PEN;
	}
	else if (_wcsicmp(mouseCursor, L"NO") == 0)
	{
		m_CursorType = MOUSECURSOR_NO;
	}
	else if (_wcsicmp(mouseCursor, L"SIZE_ALL") == 0)
	{
		m_CursorType = MOUSECURSOR_SIZE_ALL;
	}
	else if (_wcsicmp(mouseCursor, L"SIZE_NESW") == 0)
	{
		m_CursorType = MOUSECURSOR_SIZE_NESW;
	}
	else if (_wcsicmp(mouseCursor, L"SIZE_NS") == 0)
	{
		m_CursorType = MOUSECURSOR_SIZE_NS;
	}
	else if (_wcsicmp(mouseCursor, L"SIZE_NWSE") == 0)
	{
		m_CursorType = MOUSECURSOR_SIZE_NWSE;
	}
	else if (_wcsicmp(mouseCursor, L"SIZE_WE") == 0)
	{
		m_CursorType = MOUSECURSOR_SIZE_WE;
	}
	else if (_wcsicmp(mouseCursor, L"UPARROW") == 0)
	{
		m_CursorType = MOUSECURSOR_UPARROW;
	}
	else if (_wcsicmp(mouseCursor, L"WAIT") == 0)
	{
		m_CursorType = MOUSECURSOR_WAIT;
	}
	else if (wcschr(mouseCursor, L'.'))
	{
		m_CursorType = MOUSECURSOR_CUSTOM;
	}
	else
	{
		inheritSkinDefault();
	}

	if (m_CursorType == MOUSECURSOR_CUSTOM)
	{
		std::wstring cursorPath = m_Skin->GetResourcesPath();
		cursorPath += L"Cursors\\";
		cursorPath += mouseCursor;
		m_CustomCursor = LoadCursorFromFile(cursorPath.c_str());
		if (!m_CustomCursor)
		{
			m_CursorType = MOUSECURSOR_ARROW;
			LogErrorF(m_Skin, L"Invalid cursor: %s", cursorPath.c_str());
		}
	}
}

HCURSOR Mouse::GetCursor(bool isButton) const
{
	if (!isButton)
	{
		// Disabled non-button mouse actions should use the default "arrow" pointer
		bool getCursor = false;

		for (const auto& mouseAction : m_MouseActions)
		{
			if ((mouseAction.type & MOUSEACTION_BUTTON) == 0) continue;

			if (mouseAction.state == MOUSEACTION_ENABLED && !mouseAction.action.empty())
			{
				getCursor = true;
				break;
			}
		}

		if (!getCursor)
		{
			return LoadCursor(nullptr, IDC_ARROW);
		}
	}

	LPCWSTR name = IDC_ARROW;
	switch (m_CursorType)
	{
	case MOUSECURSOR_HAND:
		name = IDC_HAND;
		break;

	case MOUSECURSOR_TEXT:
		name = IDC_IBEAM;
		break;

	case MOUSECURSOR_HELP:
		name = IDC_HELP;
		break;

	case MOUSECURSOR_BUSY:
		name = IDC_APPSTARTING;
		break;

	case MOUSECURSOR_CROSS:
		name = IDC_CROSS;
		break;

	case MOUSECURSOR_PEN:
		name = MAKEINTRESOURCE(32631);
		break;

	case MOUSECURSOR_NO:
		name = IDC_NO;
		break;

	case MOUSECURSOR_SIZE_ALL:
		name = IDC_SIZEALL;
		break;

	case MOUSECURSOR_SIZE_NESW:
		name = IDC_SIZENESW;
		break;

	case MOUSECURSOR_SIZE_NS:
		name = IDC_SIZENS;
		break;

	case MOUSECURSOR_SIZE_NWSE:
		name = IDC_SIZENWSE;
		break;

	case MOUSECURSOR_SIZE_WE:
		name = IDC_SIZEWE;
		break;

	case MOUSECURSOR_UPARROW:
		name = IDC_UPARROW;
		break;

	case MOUSECURSOR_WAIT:
		name = IDC_WAIT;
		break;

	case MOUSECURSOR_CUSTOM:
		{
			if (m_CustomCursor)
			{
				return m_CustomCursor;
			}
		}
		break;
	}

	return LoadCursor(nullptr, name);
}

void Mouse::DestroyCustomCursor()
{
	if (m_CustomCursor)
	{
		DestroyCursor(m_CustomCursor);
		m_CustomCursor = nullptr;
	}
}

void Mouse::ReplaceMouseVariables(std::wstring& result) const
{
	// Check for new-style variables: [$MOUSEX]
	m_Skin->GetParser().ParseVariables(result, ConfigParser::VariableType::Mouse, m_Meter);

	// Check for old-style variables: $MOUSEX$
	size_t start = 0, end;
	bool loop = true;

	do
	{
		start = result.find(L'$', start);
		if (start != std::wstring::npos)
		{
			size_t si = start + 1;
			end = result.find(L'$', si);
			if (end != std::wstring::npos)
			{
				size_t ei = end - 1;
				if (si != ei && result[si] == L'*' && result[ei] == L'*')
				{
					result.erase(ei, 1);
					result.erase(si, 1);
					start = ei;
				}
				else
				{
					std::wstring strVariable = result.substr(si, end - si);
					std::wstring value = m_Skin->GetParser().GetMouseVariable(strVariable, m_Meter);
					if (!value.empty())
					{
						// Variable found, replace it with the value
						result.replace(start, end - start + 1, value);
						start += value.length();
					}
					else
					{
						start = end;
					}
				}
			}
			else
			{
				loop = false;
			}
		}
		else
		{
			loop = false;
		}
	}
	while (loop);
}

void Mouse::DisableMouseAction(const std::wstring& options)
{
	const auto types = OptionStringToMouseActions(options);
	for (auto& mouseAction : m_MouseActions)
	{
		if ((types & mouseAction.type) == 0) continue;
		mouseAction.previousState = MOUSEACTION_DISABLED;
		mouseAction.state = MOUSEACTION_DISABLED;
	}
}

void Mouse::ClearMouseAction(const std::wstring& options)
{
	const auto types = OptionStringToMouseActions(options);
	for (auto& mouseAction : m_MouseActions)
	{
		if ((types & mouseAction.type) == 0) continue;
		mouseAction.previousState = MOUSEACTION_CLEARED;
		mouseAction.state = MOUSEACTION_CLEARED;
	}
}

void Mouse::EnableMouseAction(const std::wstring& options)
{
	const auto types = OptionStringToMouseActions(options);
	for (auto& mouseAction : m_MouseActions)
	{
		if ((types & mouseAction.type) == 0) continue;
		mouseAction.state = MOUSEACTION_ENABLED;
	}
}

void Mouse::ToggleMouseAction(const std::wstring& options)
{
	const auto types = OptionStringToMouseActions(options);
	for (auto& mouseAction : m_MouseActions)
	{
		if ((types & mouseAction.type) == 0) continue;
		const bool alreadyEnabled = mouseAction.state == MOUSEACTION_ENABLED;
		mouseAction.state = alreadyEnabled ? mouseAction.previousState : MOUSEACTION_ENABLED;
	}
}

bool Mouse::GetActionCommand(MOUSEACTION type, std::wstring& command) const
{
	const auto* mouseAction = GetMouseActionForType(type);
	if (mouseAction && mouseAction->state == MOUSEACTION_ENABLED)
	{
		command = mouseAction->action;
		ReplaceMouseVariables(command);
		return true;
	}

	return false;
}

const std::wstring& Mouse::GetAction(MOUSEACTION action) const
{
	static const std::wstring disabled = L"[]";
	static const std::wstring empty = L"";

	const auto* mouseAction = GetMouseActionForType(action);
	if (!mouseAction) return empty;

	return
		mouseAction->state == MOUSEACTION_ENABLED ? mouseAction->action :
		mouseAction->state == MOUSEACTION_DISABLED ? disabled :
		empty;
}

const MouseAction* Mouse::GetMouseActionForType(MOUSEACTION type) const
{
	if ((m_MouseActionTypes & type) == 0) return nullptr;

	for (auto& mouseAction : m_MouseActions)
	{
		if (mouseAction.type == type) return &mouseAction;
	}

	return nullptr;
}

MOUSEACTION Mouse::OptionStringToMouseActions(const std::wstring& options) const
{
	if (options == L"*") return MOUSEACTION_ALL;

	auto tokens = ConfigParser::Tokenize(options, L"|");
	if (tokens.empty()) return MOUSEACTION_NONE;

	uint32_t result = 0;
	for (const auto& token : tokens)
	{
		bool found = false;
		const WCHAR* tok = token.c_str();
		for (auto& entry : g_MouseActionTable)
		{
			if (_wcsicmp(tok, entry.name) == 0)
			{
				found = true;
				result |= entry.type;
				break;
			}
		}

		if (!found)
		{
			LogErrorF(m_Meter, L"Invalid action: %s", tok);
			return MOUSEACTION_NONE;
		}
	}

	return (MOUSEACTION)result;
}
