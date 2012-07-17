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

#ifndef __MOUSE_H__
#define __MOUSE_H__

enum MOUSEACTION
{
	MOUSE_LMB_DOWN,
	MOUSE_LMB_UP,
	MOUSE_LMB_DBLCLK,
	MOUSE_RMB_DOWN,
	MOUSE_RMB_UP,
	MOUSE_RMB_DBLCLK,
	MOUSE_MMB_DOWN,
	MOUSE_MMB_UP,
	MOUSE_MMB_DBLCLK,
	MOUSE_OVER,
	MOUSE_LEAVE
};

enum MOUSECURSOR
{
	MOUSECURSOR_ARROW,
	MOUSECURSOR_HAND,
	MOUSECURSOR_TEXT,
	MOUSECURSOR_HELP,
	MOUSECURSOR_BUSY,
	MOUSECURSOR_CROSS,
	MOUSECURSOR_PEN,
	MOUSECURSOR_CUSTOM
};

class CMouse
{
public:
	CMouse();
	~CMouse();

	void ReadOptions(CConfigParser& parser, const WCHAR* section, CMeterWindow* meterWindow);

	MOUSECURSOR GetCursorType() const { return m_CursorType; }
	HCURSOR GetCursor() const;
	bool GetCursorState() const {return m_CursorState; }

	void DestroyCustomCursor();

	const WCHAR* GetActionCommand(MOUSEACTION action) const;

	const std::wstring& GetRightDownAction() const { return m_RightDownAction; }
	const std::wstring& GetRightUpAction() const { return m_RightUpAction; }
	const std::wstring& GetRightDoubleClickAction() const { return m_RightDoubleClickAction; }
	const std::wstring& GetLeftDownAction() const { return m_LeftDownAction; }
	const std::wstring& GetLeftUpAction() const { return m_LeftUpAction; }
	const std::wstring& GetLeftDoubleClickAction() const { return m_LeftDoubleClickAction; }
	const std::wstring& GetMiddleDownAction() const { return m_MiddleDownAction; }
	const std::wstring& GetMiddleUpAction() const { return m_MiddleUpAction; }
	const std::wstring& GetMiddleDoubleClickAction() const { return m_MiddleDoubleClickAction; }
	const std::wstring& GetOverAction() const { return m_OverAction; }
	const std::wstring& GetLeaveAction() const { return m_LeaveAction; }

private:
	std::wstring m_LeftDownAction;
	std::wstring m_RightDownAction;
	std::wstring m_MiddleDownAction;
	std::wstring m_LeftUpAction;
	std::wstring m_RightUpAction;
	std::wstring m_MiddleUpAction;
	std::wstring m_LeftDoubleClickAction;
	std::wstring m_RightDoubleClickAction;
	std::wstring m_MiddleDoubleClickAction;
	std::wstring m_OverAction;
	std::wstring m_LeaveAction;

	MOUSECURSOR m_CursorType;
	HCURSOR m_CustomCursor;
	bool m_CursorState;
};

#endif
