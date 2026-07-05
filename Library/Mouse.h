/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __MOUSE_H__
#define __MOUSE_H__

enum MOUSEACTION : uint32_t
{
	MOUSE_LMB_UP = 1 << 0,
	MOUSE_LMB_DOWN = 1 << 1,
	MOUSE_LMB_DBLCLK = 1 << 2,
	MOUSE_MMB_UP = 1 << 3,
	MOUSE_MMB_DOWN = 1 << 4,
	MOUSE_MMB_DBLCLK = 1 << 5,
	MOUSE_RMB_UP = 1 << 6,
	MOUSE_RMB_DOWN = 1 << 7,
	MOUSE_RMB_DBLCLK = 1 << 8,
	MOUSE_X1MB_UP = 1 << 9,
	MOUSE_X1MB_DOWN = 1 << 10,
	MOUSE_X1MB_DBLCLK = 1 << 11,
	MOUSE_X2MB_UP = 1 << 12,
	MOUSE_X2MB_DOWN = 1 << 13,
	MOUSE_X2MB_DBLCLK = 1 << 14,

	MOUSE_MW_UP = 1 << 15,
	MOUSE_MW_DOWN = 1 << 16,
	MOUSE_MW_LEFT = 1 << 17,
	MOUSE_MW_RIGHT = 1 << 18,

	MOUSE_OVER = 1 << 19,
	MOUSE_LEAVE = 1 << 20,

	MOUSEACTION_ALL = ~(-1 << 21),
	MOUSEACTION_BUTTON = ~(-1 << 15),
	MOUSEACTION_NONE = 0
};

enum MOUSEACTIONSTATE
{
	MOUSEACTION_ENABLED,
	MOUSEACTION_DISABLED,
	MOUSEACTION_CLEARED
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
	MOUSECURSOR_NO,
	MOUSECURSOR_SIZE_ALL,
	MOUSECURSOR_SIZE_NESW,
	MOUSECURSOR_SIZE_NS,
	MOUSECURSOR_SIZE_NWSE,
	MOUSECURSOR_SIZE_WE,
	MOUSECURSOR_UPARROW,
	MOUSECURSOR_WAIT,
	MOUSECURSOR_CUSTOM
};

struct MouseAction
{
	MOUSEACTION type;
	std::wstring action;
	MOUSEACTIONSTATE state;
	MOUSEACTIONSTATE previousState;

	MouseAction() : state(MOUSEACTION_ENABLED), previousState(MOUSEACTION_DISABLED) { }
};

class Mouse
{
public:
	Mouse(Skin* skin, Meter* meter = nullptr);
	~Mouse();

	Mouse(const Mouse& other) = delete;
	Mouse& operator=(Mouse other) = delete;

	void ReadOptions(ConfigParser& parser, const WCHAR* section);

	MOUSECURSOR GetCursorType() const { return m_CursorType; }
	HCURSOR GetCursor(bool isButton = false) const;
	bool GetCursorState() const { return m_CursorState; }

	void DestroyCustomCursor();

	void DisableMouseAction(const std::wstring& options);
	void ClearMouseAction(const std::wstring& options);
	void EnableMouseAction(const std::wstring& options);
	void ToggleMouseAction(const std::wstring& options);

	bool GetActionCommand(MOUSEACTION type, std::wstring& command) const;
	const std::wstring& GetAction(MOUSEACTION action) const;

	bool HasButtonAction() const
	{
		return !(
			GetLeftUpAction().empty() &&
			GetLeftDownAction().empty() &&
			GetLeftDoubleClickAction().empty() &&
			GetMiddleUpAction().empty() &&
			GetMiddleDownAction().empty() &&
			GetMiddleDoubleClickAction().empty() &&
			GetRightUpAction().empty() &&
			GetRightDownAction().empty() &&
			GetRightDoubleClickAction().empty() &&
			GetX1UpAction().empty() &&
			GetX1DownAction().empty() &&
			GetX1DoubleClickAction().empty() &&
			GetX2UpAction().empty() &&
			GetX2DownAction().empty() &&
			GetX2DoubleClickAction().empty()
			);
	}

	bool HasScrollAction() const
	{
		return !(
			GetMouseScrollUpAction().empty() &&
			GetMouseScrollDownAction().empty() &&
			GetMouseScrollLeftAction().empty() &&
			GetMouseScrollRightAction().empty()
			);
	}

	const std::wstring& GetLeftUpAction() const            { return GetAction(MOUSE_LMB_UP); }
	const std::wstring& GetLeftDownAction() const          { return GetAction(MOUSE_LMB_DOWN); }
	const std::wstring& GetLeftDoubleClickAction() const   { return GetAction(MOUSE_LMB_DBLCLK); }
	const std::wstring& GetMiddleUpAction() const          { return GetAction(MOUSE_MMB_UP); }
	const std::wstring& GetMiddleDownAction() const        { return GetAction(MOUSE_MMB_DOWN); }
	const std::wstring& GetMiddleDoubleClickAction() const { return GetAction(MOUSE_MMB_DBLCLK); }
	const std::wstring& GetRightUpAction() const           { return GetAction(MOUSE_RMB_UP); }
	const std::wstring& GetRightDownAction() const         { return GetAction(MOUSE_RMB_DOWN); }
	const std::wstring& GetRightDoubleClickAction() const  { return GetAction(MOUSE_RMB_DBLCLK); }
	const std::wstring& GetX1UpAction() const              { return GetAction(MOUSE_X1MB_UP); }
	const std::wstring& GetX1DownAction() const            { return GetAction(MOUSE_X1MB_DOWN); }
	const std::wstring& GetX1DoubleClickAction() const     { return GetAction(MOUSE_X1MB_DBLCLK); }
	const std::wstring& GetX2UpAction() const              { return GetAction(MOUSE_X2MB_UP); }
	const std::wstring& GetX2DownAction() const            { return GetAction(MOUSE_X2MB_DOWN); }
	const std::wstring& GetX2DoubleClickAction() const     { return GetAction(MOUSE_X2MB_DBLCLK); }

	const std::wstring& GetMouseScrollUpAction() const     { return GetAction(MOUSE_MW_UP); }
	const std::wstring& GetMouseScrollDownAction() const   { return GetAction(MOUSE_MW_DOWN); }
	const std::wstring& GetMouseScrollLeftAction() const   { return GetAction(MOUSE_MW_LEFT); }
	const std::wstring& GetMouseScrollRightAction() const  { return GetAction(MOUSE_MW_RIGHT); }

	const std::wstring& GetOverAction() const              { return GetAction(MOUSE_OVER); }
	const std::wstring& GetLeaveAction() const             { return GetAction(MOUSE_LEAVE); }

private:
	void ReplaceMouseVariables(std::wstring& result) const;
	const MouseAction* GetMouseActionForType(MOUSEACTION type) const;
	MOUSEACTION OptionStringToMouseActions(const std::wstring& options) const;

	std::vector<MouseAction> m_MouseActions;
	uint32_t m_MouseActionTypes;

	MOUSECURSOR m_CursorType;
	HCURSOR m_CustomCursor;
	bool m_CursorState;

	Skin* m_Skin;
	Meter* m_Meter;
};

#endif
