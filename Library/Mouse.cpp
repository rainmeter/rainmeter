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
#include "Meter.h"
#include "Litestep.h"
#include "Mouse.h"

CMouse::CMouse() :
	m_CursorType(MOUSECURSOR_HAND),
	m_CustomCursor(),
	m_CursorState(true),
	m_MeterWindow()
{
}

CMouse::~CMouse()
{
	DestroyCustomCursor();
}

void CMouse::ReadOptions(CConfigParser& parser, const WCHAR* section, CMeterWindow* meterWindow)
{
	m_MeterWindow = meterWindow;
	m_MeterName = section;

	DestroyCustomCursor();

	m_LeftDownAction = parser.ReadString(section, L"LeftMouseDownAction", L"", false);
	m_RightDownAction = parser.ReadString(section, L"RightMouseDownAction", L"", false);
	m_MiddleDownAction = parser.ReadString(section, L"MiddleMouseDownAction", L"", false);
	m_X1DownAction = parser.ReadString(section, L"X1MouseDownAction", L"", false);
	m_X2DownAction = parser.ReadString(section, L"X2MouseDownAction", L"", false);
	m_LeftUpAction = parser.ReadString(section, L"LeftMouseUpAction", L"", false);
	m_RightUpAction = parser.ReadString(section, L"RightMouseUpAction", L"", false);
	m_MiddleUpAction = parser.ReadString(section, L"MiddleMouseUpAction", L"", false);
	m_X1UpAction = parser.ReadString(section, L"X1MouseUpAction", L"", false);
	m_X2UpAction = parser.ReadString(section, L"X2MouseUpAction", L"", false);
	m_LeftDoubleClickAction = parser.ReadString(section, L"LeftMouseDoubleClickAction", L"", false);
	m_RightDoubleClickAction = parser.ReadString(section, L"RightMouseDoubleClickAction", L"", false);
	m_MiddleDoubleClickAction = parser.ReadString(section, L"MiddleMouseDoubleClickAction", L"", false);
	m_X1DoubleClickAction = parser.ReadString(section, L"X1MouseDoubleClickAction", L"", false);	
	m_X2DoubleClickAction = parser.ReadString(section, L"X2MouseDoubleClickAction", L"", false);

	m_OverAction = parser.ReadString(section, L"MouseOverAction", L"", false);
	m_LeaveAction = parser.ReadString(section, L"MouseLeaveAction", L"", false);

	m_MouseScrollDownAction = parser.ReadString(section, L"MouseScrollDownAction", L"", false);
	m_MouseScrollUpAction = parser.ReadString(section, L"MouseScrollUpAction", L"", false);
	m_MouseScrollLeftAction = parser.ReadString(section, L"MouseScrollLeftAction", L"", false);
	m_MouseScrollRightAction = parser.ReadString(section, L"MouseScrollRightAction", L"", false);
	if (!m_MouseScrollDownAction.empty() || !m_MouseScrollUpAction.empty() ||
		!m_MouseScrollLeftAction.empty() || !m_MouseScrollRightAction.empty())
	{
		meterWindow->SetHasMouseScrollAction();
	}

	const bool defaultState = (section == L"Rainmeter") ? true : meterWindow->GetMouse().GetCursorState();
	m_CursorState = 0!=parser.ReadInt(section, L"MouseActionCursor", defaultState);

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

std::wstring CMouse::GetActionCommand(MOUSEACTION action) const
{
	std::wstring command = L"";

	switch (action)
	{
	case MOUSE_LMB_DOWN:
		command = m_LeftDownAction;
		break;

	case MOUSE_LMB_UP:
		command = m_LeftUpAction;
		break;

	case MOUSE_LMB_DBLCLK:
		command = m_LeftDoubleClickAction;
		break;

	case MOUSE_RMB_DOWN:
		command = m_RightDownAction;
		break;

	case MOUSE_RMB_UP:
		command = m_RightUpAction;
		break;

	case MOUSE_RMB_DBLCLK:
		command = m_RightDoubleClickAction;
		break;

	case MOUSE_MMB_DOWN:
		command = m_MiddleDownAction;
		break;

	case MOUSE_MMB_UP:
		command = m_MiddleUpAction;
		break;

	case MOUSE_MMB_DBLCLK:
		command = m_MiddleDoubleClickAction;
		break;

	case MOUSE_MW_DOWN:
		command = m_MouseScrollDownAction;
		break;

	case MOUSE_MW_UP:
		command = m_MouseScrollUpAction;
		break;

	case MOUSE_MW_LEFT:
		command = m_MouseScrollLeftAction;
		break;

	case MOUSE_MW_RIGHT:
		command = m_MouseScrollRightAction;
		break;

	case MOUSE_X1MB_DOWN:
		command = m_X1DownAction;
		break;

	case MOUSE_X1MB_UP:
		command = m_X1UpAction;
		break;

	case MOUSE_X1MB_DBLCLK:
		command = m_X1DoubleClickAction;
		break;

	case MOUSE_X2MB_DOWN:
		command = m_X2DownAction;
		break;

	case MOUSE_X2MB_UP:
		command = m_X2UpAction;
		break;

	case MOUSE_X2MB_DBLCLK:
		command = m_X2DoubleClickAction;
		break;
	}

	ReplaceMouseVariables(command);

	return command;
}

void CMouse::DestroyCustomCursor()
{
	if (m_CustomCursor)
	{
		DestroyCursor(m_CustomCursor);
		m_CustomCursor = NULL;
	}
}

void CMouse::ReplaceMouseVariables(std::wstring& result) const
{
	// Check for variables ($VAR$)
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
					std::wstring value = GetMouseVariable(strVariable);
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

std::wstring CMouse::GetMouseVariable(std::wstring variable) const
{
	std::wstring result = L"";

	POINT pt;
	GetCursorPos(&pt);

	CMeter* meter = m_MeterWindow->GetMeter(m_MeterName);

	if (_wcsnicmp(variable.c_str(), L"MOUSEX", 6) == 0)
	{
		double xOffset = (double)(m_MeterWindow->GetX() + (meter ? meter->GetX() : 0) - 1);
		if (_wcsicmp(variable.c_str(), L"MOUSEX:%") == 0)
		{
			xOffset = ((pt.x - xOffset) / (meter ? meter->GetW() : m_MeterWindow->GetW())) * 100;
			result = std::to_wstring((int)xOffset);
		}
		else
		{
			result = std::to_wstring((long)(pt.x - xOffset));
		}
	}
	else if (_wcsnicmp(variable.c_str(), L"MOUSEY", 6) == 0)
	{
		double yOffset = (double)(m_MeterWindow->GetY() + (meter ? meter->GetY() : 0) - 1);
		if (_wcsicmp(variable.c_str(), L"MOUSEY:%") == 0)
		{
			yOffset = ((pt.y - yOffset) / (meter ? meter->GetH() : m_MeterWindow->GetH())) * 100;
			result = std::to_wstring((int)yOffset);
		}
		else
		{
			result = std::to_wstring((long)(pt.y - yOffset));
		}
	}

	return result;
}