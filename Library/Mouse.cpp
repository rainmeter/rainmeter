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

	m_MouseActions[MOUSE_LMB_UP]      = parser.ReadString(section, L"LeftMouseUpAction", L"", false);
	m_MouseActions[MOUSE_LMB_DOWN]    = parser.ReadString(section, L"LeftMouseDownAction", L"", false);
	m_MouseActions[MOUSE_LMB_DBLCLK]  = parser.ReadString(section, L"LeftMouseDoubleClickAction", L"", false);
	m_MouseActions[MOUSE_MMB_UP]      = parser.ReadString(section, L"MiddleMouseUpAction", L"", false);
	m_MouseActions[MOUSE_MMB_DOWN]    = parser.ReadString(section, L"MiddleMouseDownAction", L"", false);
	m_MouseActions[MOUSE_MMB_DBLCLK]  = parser.ReadString(section, L"MiddleMouseDoubleClickAction", L"", false);
	m_MouseActions[MOUSE_RMB_UP]      = parser.ReadString(section, L"RightMouseUpAction", L"", false);
	m_MouseActions[MOUSE_RMB_DOWN]    = parser.ReadString(section, L"RightMouseDownAction", L"", false);
	m_MouseActions[MOUSE_RMB_DBLCLK]  = parser.ReadString(section, L"RightMouseDoubleClickAction", L"", false);
	m_MouseActions[MOUSE_X1MB_UP]     = parser.ReadString(section, L"X1MouseUpAction", L"", false);
	m_MouseActions[MOUSE_X1MB_DOWN]   = parser.ReadString(section, L"X1MouseDownAction", L"", false);
	m_MouseActions[MOUSE_X1MB_DBLCLK] = parser.ReadString(section, L"X1MouseDoubleClickAction", L"", false);
	m_MouseActions[MOUSE_X2MB_UP]     = parser.ReadString(section, L"X2MouseUpAction", L"", false);
	m_MouseActions[MOUSE_X2MB_DOWN]   = parser.ReadString(section, L"X2MouseDownAction", L"", false);
	m_MouseActions[MOUSE_X2MB_DBLCLK] = parser.ReadString(section, L"X2MouseDoubleClickAction", L"", false);

	m_MouseActions[MOUSE_MW_UP]       = parser.ReadString(section, L"MouseScrollUpAction", L"", false);
	m_MouseActions[MOUSE_MW_DOWN]     = parser.ReadString(section, L"MouseScrollDownAction", L"", false);
	m_MouseActions[MOUSE_MW_LEFT]     = parser.ReadString(section, L"MouseScrollLeftAction", L"", false);
	m_MouseActions[MOUSE_MW_RIGHT]    = parser.ReadString(section, L"MouseScrollRightAction", L"", false);

	m_MouseActions[MOUSE_OVER]        = parser.ReadString(section, L"MouseOverAction", L"", false);
	m_MouseActions[MOUSE_LEAVE]       = parser.ReadString(section, L"MouseLeaveAction", L"", false);

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

HCURSOR Mouse::GetCursor() const
{
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
	std::wstring command = m_MouseActions[action];
	ReplaceMouseVariables(command);
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
