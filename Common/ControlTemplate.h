/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_COMMON_CONTROLTEMPLATE_H_
#define RM_COMMON_CONTROLTEMPLATE_H_

#include <Windows.h>
#include <vector>

#define CONTROL_FACTORY(method, className, defaultStyle) \
	static Control method(WORD id, WORD textId, short x, short y, short w, short h, \
		DWORD style, DWORD exStyle, BYTE options = ANCHOR_TOP_LEFT) \
	{ \
		return Item(className, id, textId, x, y, w, h, defaultStyle | style, exStyle, options); \
	}

struct Control
{
	enum Option : BYTE
	{
		ANCHOR_LEFT = 0x01,
		ANCHOR_TOP = 0x02,
		ANCHOR_TOP_LEFT = ANCHOR_LEFT | ANCHOR_TOP,
		ANCHOR_RIGHT = 0x04,
		ANCHOR_BOTTOM = 0x08,
		ANCHOR_BOTTOM_RIGHT = ANCHOR_BOTTOM | ANCHOR_RIGHT,
		ANCHOR_ALL = ANCHOR_TOP_LEFT | ANCHOR_BOTTOM_RIGHT,
		BOLD_FONT = 0x10
	};

	static Control Item(const WCHAR* name, WORD id, WORD textId, short x, short y, short w, short h,
		DWORD style, DWORD exStyle, BYTE options = ANCHOR_TOP_LEFT)
	{
		return { name, id, textId, x, y, w, h, WS_CHILD | style, exStyle, options };
	}

	CONTROL_FACTORY(Button, L"Button", BS_PUSHBUTTON)
	CONTROL_FACTORY(CheckBox, L"Button", BS_AUTOCHECKBOX)
	CONTROL_FACTORY(ComboBox, L"ComboBox", 0)
	CONTROL_FACTORY(Edit, L"Edit", ES_LEFT)
	CONTROL_FACTORY(GroupBox, L"Button", BS_GROUPBOX)
	CONTROL_FACTORY(Icon, L"Static", SS_ICON)
	CONTROL_FACTORY(Label, L"Static", SS_LEFT)
	CONTROL_FACTORY(LineH, L"Static", SS_ETCHEDHORZ)
	CONTROL_FACTORY(LineV, L"Static", SS_ETCHEDVERT)
	CONTROL_FACTORY(LinkLabel, L"SysLink", 0)
	CONTROL_FACTORY(ListBox, L"ListBox", 0)
	CONTROL_FACTORY(ListView, L"SysListView32", 0)
	CONTROL_FACTORY(Tab, L"SysTabControl32", TCS_TABS)
	CONTROL_FACTORY(TreeView, L"SysTreeView32", 0)

	const WCHAR* name;
	WORD id;
	WORD textId;
	short x;
	short y;
	short w;
	short h;
	DWORD style;
	DWORD exStyle;
	BYTE options;
};

#undef CONTROL_FACTORY

class ControlTemplate
{
public:
	ControlTemplate();
	~ControlTemplate();

	typedef WCHAR* (*GetStringFunc)(UINT id);
	void Initialize(const Control* cts, UINT ctCount, HWND parent, UINT dpi, GetStringFunc getString);
	void Relayout(UINT dpi);

	UINT ScaleDialogUnits(int value) const;

private:
	struct CreatedControl
	{
		Control control;
		HWND window;
		RECT initialRect;
	};

	ControlTemplate(const ControlTemplate& controlTemplate) = delete;
	ControlTemplate& operator=(const ControlTemplate& controlTemplate) = delete;

	void UpdateFonts(UINT dpi);

	HWND m_Parent;
	SIZE m_InitialParentSize;
	UINT m_InitialDpi;
	UINT m_CurrentDpi;
	HFONT m_Font;
	HFONT m_FontBold;
	std::vector<CreatedControl> m_Controls;
};

#endif
