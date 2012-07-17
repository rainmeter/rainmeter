/*
  Copyright (C) 2012 Rainmeter Team

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "ConfigParser.h"
#include "MeterWindow.h"
#include "Litestep.h"
#include "Mouse.h"

CMouse::CMouse() :
	m_CursorType(MOUSECURSOR_HAND),
	m_CustomCursor(),
	m_CursorState(true)
{
}

CMouse::~CMouse()
{
	DestroyCustomCursor();
}

void CMouse::ReadOptions(CConfigParser& parser, const WCHAR* section, CMeterWindow* meterWindow)
{
	DestroyCustomCursor();

	m_LeftDownAction = parser.ReadString(section, L"LeftMouseDownAction", L"", false);
	m_RightDownAction = parser.ReadString(section, L"RightMouseDownAction", L"", false);
	m_MiddleDownAction = parser.ReadString(section, L"MiddleMouseDownAction", L"", false);
	m_LeftUpAction = parser.ReadString(section, L"LeftMouseUpAction", L"", false);
	m_RightUpAction = parser.ReadString(section, L"RightMouseUpAction", L"", false);
	m_MiddleUpAction = parser.ReadString(section, L"MiddleMouseUpAction", L"", false);
	m_LeftDoubleClickAction = parser.ReadString(section, L"LeftMouseDoubleClickAction", L"", false);
	m_RightDoubleClickAction = parser.ReadString(section, L"RightMouseDoubleClickAction", L"", false);
	m_MiddleDoubleClickAction = parser.ReadString(section, L"MiddleMouseDoubleClickAction", L"", false);
	m_OverAction = parser.ReadString(section, L"MouseOverAction", L"", false);
	m_LeaveAction = parser.ReadString(section, L"MouseLeaveAction", L"", false);

	m_CursorState = 0!=parser.ReadInt(section, L"MouseActionCursor", meterWindow->GetMouse().GetCursorState());

	const WCHAR* defaultMouseCursor = (section == L"Rainmeter") ? L"HAND" : L"";
	const WCHAR* mouseCursor = parser.ReadString(section, L"MouseActionCursorName", defaultMouseCursor).c_str();
	if (_wcsicmp(mouseCursor, L"HAND") == 0)
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
		// Inherit from [Rainmeter].
		m_CursorType = meterWindow->GetMouse().GetCursorType();
		if (m_CursorType == MOUSECURSOR_CUSTOM)
		{
			mouseCursor = meterWindow->GetParser().ReadString(L"Rainmeter", L"MouseActionCursorName", L"").c_str();
		}
	}

	if (m_CursorType == MOUSECURSOR_CUSTOM)
	{
		std::wstring cursorPath = meterWindow->GetResourcesPath();
		cursorPath += L"Cursors\\";
		cursorPath += mouseCursor;
		m_CustomCursor = LoadCursorFromFile(cursorPath.c_str());
		if (!m_CustomCursor)
		{
			m_CursorType = MOUSECURSOR_ARROW;
			LogWithArgs(LOG_ERROR, L"Invalid cursor: %s", cursorPath.c_str());
		}
	}
}

HCURSOR CMouse::GetCursor() const
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

	return LoadCursor(NULL, name);
}

const WCHAR* CMouse::GetActionCommand(MOUSEACTION action) const
{
	const WCHAR* command = NULL;

	switch (action)
	{
	case MOUSE_LMB_DOWN:
		command = m_LeftDownAction.c_str();
		break;

	case MOUSE_LMB_UP:
		command = m_LeftUpAction.c_str();
		break;

	case MOUSE_LMB_DBLCLK:
		command = m_LeftDoubleClickAction.c_str();
		break;

	case MOUSE_RMB_DOWN:
		command = m_RightDownAction.c_str();
		break;

	case MOUSE_RMB_UP:
		command = m_RightUpAction.c_str();
		break;

	case MOUSE_RMB_DBLCLK:
		command = m_RightDoubleClickAction.c_str();
		break;

	case MOUSE_MMB_DOWN:
		command = m_MiddleDownAction.c_str();
		break;

	case MOUSE_MMB_UP:
		command = m_MiddleUpAction.c_str();
		break;

	case MOUSE_MMB_DBLCLK:
		command = m_MiddleDoubleClickAction.c_str();
		break;
	}

	return *command ? command : NULL;
}

void CMouse::DestroyCustomCursor()
{
	if (m_CustomCursor)
	{
		DestroyCursor(m_CustomCursor);
		m_CustomCursor = NULL;
	}
}