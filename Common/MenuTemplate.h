// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>

struct MenuTemplate
{
	BYTE type;
	WORD id;
	WORD idText;

	typedef const WCHAR* (*GetStringFunc)(UINT id);

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
