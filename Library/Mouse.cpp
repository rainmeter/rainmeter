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

Mouse::Mouse(Skin* skin, Meter* meter) : m_Skin(skin), m_Meter(meter),
	m_CursorType(MOUSECURSOR_HAND),
	m_CustomCursor(),
	m_CursorState(true)
{
}

Mouse::~Mouse()
{
	DestroyCustomCursor();
}

void Mouse::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	DestroyCustomCursor();

	m_MouseActions[MOUSE_LMB_UP].action      = parser.ReadString(section, L"LeftMouseUpAction", L"", false);
	m_MouseActions[MOUSE_LMB_DOWN].action    = parser.ReadString(section, L"LeftMouseDownAction", L"", false);
	m_MouseActions[MOUSE_LMB_DBLCLK].action  = parser.ReadString(section, L"LeftMouseDoubleClickAction", L"", false);
	m_MouseActions[MOUSE_MMB_UP].action      = parser.ReadString(section, L"MiddleMouseUpAction", L"", false);
	m_MouseActions[MOUSE_MMB_DOWN].action    = parser.ReadString(section, L"MiddleMouseDownAction", L"", false);
	m_MouseActions[MOUSE_MMB_DBLCLK].action  = parser.ReadString(section, L"MiddleMouseDoubleClickAction", L"", false);
	m_MouseActions[MOUSE_RMB_UP].action      = parser.ReadString(section, L"RightMouseUpAction", L"", false);
	m_MouseActions[MOUSE_RMB_DOWN].action    = parser.ReadString(section, L"RightMouseDownAction", L"", false);
	m_MouseActions[MOUSE_RMB_DBLCLK].action  = parser.ReadString(section, L"RightMouseDoubleClickAction", L"", false);
	m_MouseActions[MOUSE_X1MB_UP].action     = parser.ReadString(section, L"X1MouseUpAction", L"", false);
	m_MouseActions[MOUSE_X1MB_DOWN].action   = parser.ReadString(section, L"X1MouseDownAction", L"", false);
	m_MouseActions[MOUSE_X1MB_DBLCLK].action = parser.ReadString(section, L"X1MouseDoubleClickAction", L"", false);
	m_MouseActions[MOUSE_X2MB_UP].action     = parser.ReadString(section, L"X2MouseUpAction", L"", false);
	m_MouseActions[MOUSE_X2MB_DOWN].action   = parser.ReadString(section, L"X2MouseDownAction", L"", false);
	m_MouseActions[MOUSE_X2MB_DBLCLK].action = parser.ReadString(section, L"X2MouseDoubleClickAction", L"", false);

	m_MouseActions[MOUSE_MW_UP].action       = parser.ReadString(section, L"MouseScrollUpAction", L"", false);
	m_MouseActions[MOUSE_MW_DOWN].action     = parser.ReadString(section, L"MouseScrollDownAction", L"", false);
	m_MouseActions[MOUSE_MW_LEFT].action     = parser.ReadString(section, L"MouseScrollLeftAction", L"", false);
	m_MouseActions[MOUSE_MW_RIGHT].action    = parser.ReadString(section, L"MouseScrollRightAction", L"", false);

	m_MouseActions[MOUSE_OVER].action        = parser.ReadString(section, L"MouseOverAction", L"", false);
	m_MouseActions[MOUSE_LEAVE].action       = parser.ReadString(section, L"MouseLeaveAction", L"", false);

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
		for (size_t i = 0; i < (MOUSEACTION_COUNT - 2); ++i)	// Do not process Over/Leave
		{
			if (m_MouseActions[i].state == MOUSEACTION_ENABLED &&
				!m_MouseActions[i].action.empty())
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

std::wstring Mouse::GetActionCommand(MOUSEACTION action) const
{
	std::wstring command = GetAction(action);
	if (m_MouseActions[action].state == MOUSEACTION_ENABLED)
	{
		ReplaceMouseVariables(command);
	}
	return command;
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
	for (const auto& token : Translate(options))
	{
		m_MouseActions[token].previousState = MOUSEACTION_DISABLED;
		m_MouseActions[token].state = MOUSEACTION_DISABLED;
	}
}

void Mouse::ClearMouseAction(const std::wstring& options)
{
	for (const auto& token : Translate(options))
	{
		m_MouseActions[token].previousState = MOUSEACTION_CLEARED;
		m_MouseActions[token].state = MOUSEACTION_CLEARED;
	}
}

void Mouse::EnableMouseAction(const std::wstring& options)
{
	for (const auto& token : Translate(options))
	{
		m_MouseActions[token].state = MOUSEACTION_ENABLED;
	}
}

void Mouse::ToggleMouseAction(const std::wstring& options)
{
	for (const auto& token : Translate(options))
	{
		if (m_MouseActions[token].state == MOUSEACTION_ENABLED)
		{
			m_MouseActions[token].state = m_MouseActions[token].previousState;
		}
		else
		{
			m_MouseActions[token].state = MOUSEACTION_ENABLED;
		}
	}
}

const std::wstring& Mouse::GetAction(MOUSEACTION action) const
{
	static const std::wstring disabled = L"[]";
	static const std::wstring empty = L"";

	switch (m_MouseActions[action].state)
	{
	case MOUSEACTION_ENABLED: return m_MouseActions[action].action;
	case MOUSEACTION_DISABLED: return disabled;
	}

	return empty;
}

std::vector<MOUSEACTION> Mouse::Translate(const std::wstring& options) const
{
	std::vector<MOUSEACTION> result;

	if (options.compare(L"*") == 0)
	{
		result.emplace_back(MOUSE_LMB_UP);
		result.emplace_back(MOUSE_LMB_DOWN);
		result.emplace_back(MOUSE_LMB_DBLCLK);
		result.emplace_back(MOUSE_MMB_UP);
		result.emplace_back(MOUSE_MMB_DOWN);
		result.emplace_back(MOUSE_MMB_DBLCLK);
		result.emplace_back(MOUSE_RMB_UP);
		result.emplace_back(MOUSE_RMB_DOWN);
		result.emplace_back(MOUSE_RMB_DBLCLK);
		result.emplace_back(MOUSE_X1MB_UP);
		result.emplace_back(MOUSE_X1MB_DOWN);
		result.emplace_back(MOUSE_X1MB_DBLCLK);
		result.emplace_back(MOUSE_X2MB_UP);
		result.emplace_back(MOUSE_X2MB_DOWN);
		result.emplace_back(MOUSE_X2MB_DBLCLK);
		result.emplace_back(MOUSE_MW_UP);
		result.emplace_back(MOUSE_MW_DOWN);
		result.emplace_back(MOUSE_MW_LEFT);
		result.emplace_back(MOUSE_MW_RIGHT);
		result.emplace_back(MOUSE_OVER);
		result.emplace_back(MOUSE_LEAVE);
	}
	else
	{
		auto tokens = ConfigParser::Tokenize(options, L"|");
		if (!tokens.empty())
		{
			for (const auto token : tokens)
			{
				const WCHAR* tok = token.c_str();

				if (_wcsicmp(tok, L"LeftMouseUpAction") == 0)                 result.emplace_back(MOUSE_LMB_UP);
				else if (_wcsicmp(tok, L"LeftMouseDownAction") == 0)          result.emplace_back(MOUSE_LMB_DOWN);
				else if (_wcsicmp(tok, L"LeftMouseDoubleClickAction") == 0)   result.emplace_back(MOUSE_LMB_DBLCLK);
				else if (_wcsicmp(tok, L"MiddleMouseUpAction") == 0)          result.emplace_back(MOUSE_MMB_UP);
				else if (_wcsicmp(tok, L"MiddleMouseDownAction") == 0)        result.emplace_back(MOUSE_MMB_DOWN);
				else if (_wcsicmp(tok, L"MiddleMouseDoubleClickAction") == 0) result.emplace_back(MOUSE_MMB_DBLCLK);
				else if (_wcsicmp(tok, L"RightMouseUpAction") == 0)           result.emplace_back(MOUSE_RMB_UP);
				else if (_wcsicmp(tok, L"RightMouseDownAction") == 0)         result.emplace_back(MOUSE_RMB_DOWN);
				else if (_wcsicmp(tok, L"RightMouseDoubleClickAction") == 0)  result.emplace_back(MOUSE_RMB_DBLCLK);
				else if (_wcsicmp(tok, L"X1MouseUpAction") == 0)              result.emplace_back(MOUSE_X1MB_UP);
				else if (_wcsicmp(tok, L"X1MouseDownAction") == 0)            result.emplace_back(MOUSE_X1MB_DOWN);
				else if (_wcsicmp(tok, L"X1MouseDoubleClickAction") == 0)     result.emplace_back(MOUSE_X1MB_DBLCLK);
				else if (_wcsicmp(tok, L"X2MouseUpAction") == 0)              result.emplace_back(MOUSE_X2MB_UP);
				else if (_wcsicmp(tok, L"X2MouseDownAction") == 0)            result.emplace_back(MOUSE_X2MB_DOWN);
				else if (_wcsicmp(tok, L"X2MouseDoubleClickAction") == 0)     result.emplace_back(MOUSE_X2MB_DBLCLK);

				else if (_wcsicmp(tok, L"MouseScrollUpAction") == 0)          result.emplace_back(MOUSE_MW_UP);
				else if (_wcsicmp(tok, L"MouseScrollDownAction") == 0)        result.emplace_back(MOUSE_MW_DOWN);
				else if (_wcsicmp(tok, L"MouseScrollLeftAction") == 0)        result.emplace_back(MOUSE_MW_LEFT);
				else if (_wcsicmp(tok, L"MouseScrollRightAction") == 0)       result.emplace_back(MOUSE_MW_RIGHT);

				else if (_wcsicmp(tok, L"MouseOverAction") == 0)              result.emplace_back(MOUSE_OVER);
				else if (_wcsicmp(tok, L"MouseLeaveAction") == 0)             result.emplace_back(MOUSE_LEAVE);

				else
				{
					LogErrorF(m_Meter, L"Invalid action: %s", tok);
					result.clear();
					break;
				}
			}
		}
	}

	return result;
}
