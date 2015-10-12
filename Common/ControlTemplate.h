/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_COMMON_CONTROLTEMPLATE_H_
#define RM_COMMON_CONTROLTEMPLATE_H_

#include <Windows.h>

namespace ControlTemplate
{

struct Control
{
	const WCHAR* name;
	WORD id;
	WORD textId;
	short x;
	short y;
	short w;
	short h;
	DWORD style;
	DWORD exStyle;
};

typedef WCHAR* (*GetStringFunc)(UINT id);

void CreateControls(const Control* cts, UINT ctCount, HWND parent, HFONT font, GetStringFunc getString);

// Helpers to declare control structs.
#define CT_ITEM(name, id, textId, x, y, w, h, style, exStyle) \
	{ name, id, textId, x, y, w, h, WS_CHILD | style, exStyle }

#define CT_BUTTON(id, textId, x, y, w, h, style, exStyle) \
	CT_ITEM(L"Button", id, textId, x, y, w, h, BS_PUSHBUTTON | style, exStyle)

#define CT_CHECKBOX(id, textId, x, y, w, h, style, exStyle) \
	CT_ITEM(L"Button", id, textId, x, y, w, h, BS_AUTOCHECKBOX | style, exStyle)

#define CT_COMBOBOX(id, textId, x, y, w, h, style, exStyle) \
	CT_ITEM(L"ComboBox", id, textId, x, y, w, h, style, exStyle)

#define CT_EDIT(id, textId, x, y, w, h, style, exStyle) \
	CT_ITEM(L"Edit", id, textId, x, y, w, h, ES_LEFT | style, exStyle)

#define CT_GROUPBOX(id, textId, x, y, w, h, style, exStyle) \
	CT_ITEM(L"Button", id, textId, x, y, w, h, BS_GROUPBOX | style, exStyle)

#define CT_ICON(id, textId, x, y, w, h, style, exStyle) \
	CT_ITEM(L"Static", id, textId, x, y, w, h, SS_ICON | style, exStyle)

#define CT_LABEL(id, textId, x, y, w, h, style, exStyle) \
	CT_ITEM(L"Static", id, textId, x, y, w, h, SS_LEFT | style, exStyle)

#define CT_LINEH(id, textId, x, y, w, h, style, exStyle) \
	CT_ITEM(L"Static", id, textId, x, y, w, h, SS_ETCHEDHORZ | style, exStyle)

#define CT_LINKLABEL(id, textId, x, y, w, h, style, exStyle) \
	CT_ITEM(L"SysLink", id, textId, x, y, w, h, style, exStyle)

#define CT_LISTBOX(id, textId, x, y, w, h, style, exStyle) \
	CT_ITEM(L"ListBox", id, textId, x, y, w, h, style, exStyle)

#define CT_LISTVIEW(id, textId, x, y, w, h, style, exStyle) \
	CT_ITEM(L"SysListView32", id, textId, x, y, w, h, style, exStyle)

#define CT_TAB(id, textId, x, y, w, h, style, exStyle) \
	CT_ITEM(L"SysTabControl32", id, textId, x, y, w, h, TCS_TABS | style, exStyle)

#define CT_TREEVIEW(id, textId, x, y, w, h, style, exStyle) \
	CT_ITEM(L"SysTreeView32", id, textId, x, y, w, h, style, exStyle)

}  // namespace ControlTemplate

#endif
