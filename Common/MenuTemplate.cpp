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
