// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MenuTemplate.h"

HMENU MenuTemplate::CreateMenu(const MenuTemplate* items, UINT itemCount, GetStringFunc getString)
{
	UINT itemIndex = 0;
	return CreateSubMenu(items, itemIndex, itemCount, getString);
}

HMENU MenuTemplate::CreateSubMenu(const MenuTemplate* items, UINT& itemIndex, UINT itemCount, GetStringFunc getString)
{
	HMENU menu = CreatePopupMenu();

	do
	{
		const MenuTemplate& item = items[itemIndex];
		++itemIndex;

		UINT itemFlags = MF_STRING;
		UINT_PTR itemId = item.id;
		const WCHAR* itemText = item.idText ? getString(item.idText) : nullptr;

		if (item.type == MenuItem_ItemGrayed)
		{
			itemFlags |= MF_GRAYED;
		}
		else if (item.type == MenuItem_Separator)
		{
			itemFlags = MF_SEPARATOR;
		}
		else if (item.type == MenuItem_SubMenuBegin)
		{
			itemFlags = MF_POPUP;
			itemId = (UINT_PTR)CreateSubMenu(items, itemIndex, itemCount, getString);
		}
		else if (item.type == MenuItem_SubMenuEnd)
		{
			return menu;
		}

		AppendMenu(menu, itemFlags, itemId, itemText);
	}
	while (itemIndex < itemCount);

	return menu;
}
