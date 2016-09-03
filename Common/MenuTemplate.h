/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_COMMON_MENUTEMPLATE_H_
#define RM_COMMON_MENUTEMPLATE_H_

#include <Windows.h>

struct MenuTemplate
{
	BYTE type;
	WORD id;
	WORD idText;

	typedef WCHAR* (*GetStringFunc)(UINT id);

	static HMENU CreateMenu(const MenuTemplate* items, UINT itemCount, GetStringFunc getString);

private:
	static HMENU CreateSubMenu(const MenuTemplate* items, UINT& itemIndex, UINT itemCount, GetStringFunc getString);
};

enum MenuTemplateItem
{
	MenuItem_Item,
	MenuItem_ItemGrayed,
	MenuItem_Separator,
	MenuItem_SubMenuBegin,
	MenuItem_SubMenuEnd
};

#define MENU_ITEM(...) { MenuItem_Item, __VA_ARGS__ }
#define MENU_ITEM_GRAYED(...) { MenuItem_ItemGrayed, __VA_ARGS__ }
#define MENU_SEPARATOR() { MenuItem_Separator }
#define MENU_SUBMENU(idText, ...) { MenuItem_SubMenuBegin, 0, idText }, __VA_ARGS__, { MenuItem_SubMenuEnd }

#endif
