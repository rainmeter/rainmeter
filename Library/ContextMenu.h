/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_CONTEXTMENU_H
#define RM_LIBRARY_CONTEXTMENU_H

#include <Windows.h>

class Skin;

// Handles the creation and display of Rainmeter and skin context menus.
class ContextMenu
{
public:
	ContextMenu();

	ContextMenu(const ContextMenu& other) = delete;
	ContextMenu& operator=(ContextMenu other) = delete;

	bool IsMenuActive() { return m_MenuActive; }

	void ShowMenu(POINT pos, Skin* skin);
	void ShowSkinCustomMenu(POINT pos, Skin* skin);

	static void CreateMonitorMenu(HMENU monitorMenu, Skin* skin);

private:
	static void DisplayMenu(POINT pos, HMENU menu, HWND parentWindow);

	static HMENU CreateSkinMenu(Skin* skin, int index, HMENU menu);
	static void AppendSkinCustomMenu(
		Skin* skin, int index, HMENU menu, bool standaloneMenu);
	static void ChangeSkinIndex(HMENU subMenu, int index);
	
	static void CreateAllSkinsMenu(HMENU skinMenu) { CreateSkinsMenuRecursive(skinMenu, 0, false); }
	static int CreateSkinsMenuRecursive(HMENU skinMenu, int index, bool isFavoriteMenu);

	static void CreateLayoutMenu(HMENU layoutMenu);

	static void CreateFavoritesMenu(HMENU favoriteMenu) { CreateSkinsMenuRecursive(favoriteMenu, 0, true); }

	bool m_MenuActive;
};

#endif
