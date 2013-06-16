/*
  Copyright (C) 2013 Rainmeter Team

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

#ifndef RM_LIBRARY_CONTEXTMENU_H
#define RM_LIBRARY_CONTEXTMENU_H

#include <Windows.h>

class MeterWindow;

// Handles the creation and display of Rainmeter and skin context menus.
class ContextMenu
{
public:
	ContextMenu();

	bool IsMenuActive() { return m_MenuActive; }

	void ShowMenu(POINT pos, MeterWindow* meterWindow);

	static void CreateMonitorMenu(HMENU monitorMenu, MeterWindow* meterWindow);

private:
	static HMENU CreateSkinMenu(MeterWindow* meterWindow, int index, HMENU menu);
	static void ChangeSkinIndex(HMENU subMenu, int index);
	
	static void CreateAllSkinsMenu(HMENU skinMenu) { CreateAllSkinsMenuRecursive(skinMenu, 0); }
	static int CreateAllSkinsMenuRecursive(HMENU skinMenu, int index);

	static void CreateLayoutMenu(HMENU layoutMenu);

	bool m_MenuActive;
};

#endif
