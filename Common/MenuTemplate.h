/*
  Copyright (C) 2012 Birunthan Mohanathas

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
