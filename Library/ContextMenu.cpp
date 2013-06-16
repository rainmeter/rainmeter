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

#include "StdAfx.h"
#include "../Common/MenuTemplate.h"
#include "ContextMenu.h"
#include "Rainmeter.h"
#include "Litestep.h"
#include "MeterWindow.h"
#include "System.h"
#include "TrayWindow.h"
#include "resource.h"

ContextMenu::ContextMenu() :
	m_MenuActive(false)
{
}

/*
** Opens the context menu in given coordinates.
*/
void ContextMenu::ShowMenu(POINT pos, MeterWindow* meterWindow)
{
	static const MenuTemplate s_Menu[] =
	{
		MENU_ITEM(IDM_MANAGE, ID_STR_MANAGE),
		MENU_ITEM(IDM_ABOUT, ID_STR_ABOUT),
		MENU_ITEM(IDM_SHOW_HELP, ID_STR_HELP),
		MENU_SEPARATOR(),
		MENU_SUBMENU(ID_STR_SKINS,
			MENU_ITEM_GRAYED(0, ID_STR_NOSKINS),
			MENU_SEPARATOR(),
			MENU_ITEM(IDM_OPENSKINSFOLDER, ID_STR_OPENFOLDER),
			MENU_ITEM(IDM_DISABLEDRAG, ID_STR_DISABLEDRAGGING)),
		MENU_SUBMENU(ID_STR_THEMES,
			MENU_ITEM_GRAYED(0, ID_STR_NOTHEMES)),
		MENU_SEPARATOR(),
		MENU_ITEM(IDM_EDITCONFIG, ID_STR_EDITSETTINGS),
		MENU_ITEM(IDM_REFRESH, ID_STR_REFRESHALL),
		MENU_SEPARATOR(),
		MENU_SUBMENU(ID_STR_LOGGING,
			MENU_ITEM(IDM_SHOWLOGFILE, ID_STR_SHOWLOGFILE),
			MENU_SEPARATOR(),
			MENU_ITEM(IDM_STARTLOG, ID_STR_STARTLOGGING),
			MENU_ITEM(IDM_STOPLOG, ID_STR_STOPLOGGING),
			MENU_SEPARATOR(),
			MENU_ITEM(IDM_DELETELOGFILE, ID_STR_DELETELOGFILE),
			MENU_ITEM(IDM_DEBUGLOG, ID_STR_DEBUGMODE)),
		MENU_SEPARATOR(),
		MENU_ITEM(IDM_QUIT, ID_STR_EXIT)
	};

	if (m_MenuActive || (meterWindow && meterWindow->IsClosing())) return;

	// Show context menu, if no actions were executed
	HMENU menu = MenuTemplate::CreateMenu(s_Menu, _countof(s_Menu), GetString);
	if (!menu) return;

	m_MenuActive = true;
	Rainmeter& rainmeter = GetRainmeter();

	SetMenuDefaultItem(menu, IDM_MANAGE, MF_BYCOMMAND);

	if (_waccess(GetLogger().GetLogFilePath().c_str(), 0) == -1)
	{
		EnableMenuItem(menu, IDM_SHOWLOGFILE, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(menu, IDM_DELETELOGFILE, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(menu, IDM_STOPLOG, MF_BYCOMMAND | MF_GRAYED);
	}
	else
	{
		EnableMenuItem(
			menu,
			(GetLogger().IsLogToFile()) ? IDM_STARTLOG : IDM_STOPLOG,
			MF_BYCOMMAND | MF_GRAYED);
	}

	if (rainmeter.m_Debug)
	{
		CheckMenuItem(menu, IDM_DEBUGLOG, MF_BYCOMMAND | MF_CHECKED);
	}

	HMENU allSkinsMenu = GetSubMenu(menu, 4);
	if (allSkinsMenu)
	{
		if (!rainmeter.m_SkinRegistry.IsEmpty())
		{
			DeleteMenu(allSkinsMenu, 0, MF_BYPOSITION);  // "No skins available" menuitem
			CreateAllSkinsMenu(allSkinsMenu);
		}

		if (rainmeter.m_DisableDragging)
		{
			CheckMenuItem(allSkinsMenu, IDM_DISABLEDRAG, MF_BYCOMMAND | MF_CHECKED);
		}
	}

	HMENU layoutMenu = GetSubMenu(menu, 5);
	if (layoutMenu)
	{
		if (!rainmeter.m_Layouts.empty())
		{
			DeleteMenu(layoutMenu, 0, MF_BYPOSITION);  // "No layouts available" menuitem
			CreateLayoutMenu(layoutMenu);
		}
	}

	if (meterWindow)
	{
		HMENU rainmeterMenu = menu;
		menu = CreateSkinMenu(meterWindow, 0, allSkinsMenu);

		InsertMenu(menu, IDM_CLOSESKIN, MF_BYCOMMAND | MF_POPUP, (UINT_PTR)rainmeterMenu, L"Rainmeter");
		InsertMenu(menu, IDM_CLOSESKIN, MF_BYCOMMAND | MF_SEPARATOR, 0, nullptr);
	}
	else
	{
		InsertMenu(menu, 12, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);

		// Create a menu for all active skins
		int index = 0;
		std::map<std::wstring, MeterWindow*>::const_iterator iter = rainmeter.m_MeterWindows.begin();
		for (; iter != rainmeter.m_MeterWindows.end(); ++iter)
		{
			MeterWindow* mw = ((*iter).second);
			HMENU skinMenu = CreateSkinMenu(mw, index, allSkinsMenu);
			InsertMenu(menu, 12, MF_BYPOSITION | MF_POPUP, (UINT_PTR)skinMenu, mw->GetFolderPath().c_str());
			++index;
		}

		// Add update notification item
		if (rainmeter.m_NewVersion)
		{
			InsertMenu(menu, 0, MF_BYPOSITION, IDM_NEW_VERSION, GetString(ID_STR_UPDATEAVAILABLE));
			HiliteMenuItem(rainmeter.GetTrayWindow()->GetWindow(), menu, 0, MF_BYPOSITION | MF_HILITE);
			InsertMenu(menu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
		}
	}

	HWND hWnd = WindowFromPoint(pos);
	if (hWnd != nullptr)
	{
		MeterWindow* mw = rainmeter.GetMeterWindow(hWnd);
		if (mw)
		{
			// Cancel the mouse event beforehand
			mw->SetMouseLeaveEvent(true);
		}
	}

	// Set the window to foreground
	hWnd = meterWindow ? meterWindow->GetWindow() : rainmeter.m_TrayWindow->GetWindow();
	HWND hWndForeground = GetForegroundWindow();
	if (hWndForeground != hWnd)
	{
		const DWORD foregroundThreadID = GetWindowThreadProcessId(hWndForeground, nullptr);
		const DWORD currentThreadID = GetCurrentThreadId();
		AttachThreadInput(currentThreadID, foregroundThreadID, TRUE);
		SetForegroundWindow(hWnd);
		AttachThreadInput(currentThreadID, foregroundThreadID, FALSE);
	}

	// Show context menu
	TrackPopupMenu(
		menu,
		TPM_RIGHTBUTTON | TPM_LEFTALIGN | (*GetString(ID_STR_ISRTL) == L'1' ? TPM_LAYOUTRTL : 0),
		pos.x,
		pos.y,
		0,
		hWnd,
		nullptr);

	DestroyMenu(menu);

	m_MenuActive = false;
}

HMENU ContextMenu::CreateSkinMenu(MeterWindow* meterWindow, int index, HMENU menu)
{
	static const MenuTemplate s_Menu[] =
	{
		MENU_ITEM(IDM_SKIN_OPENSKINSFOLDER, 0),
		MENU_SEPARATOR(),
		MENU_SUBMENU(ID_STR_VARIANTS,
			MENU_SEPARATOR()),
		MENU_SEPARATOR(),
		MENU_SUBMENU(ID_STR_SETTINGS,
			MENU_SUBMENU(ID_STR_POSITION,
				MENU_SUBMENU(ID_STR_DISPLAYMONITOR,
					MENU_ITEM(IDM_SKIN_MONITOR_PRIMARY, ID_STR_USEDEFAULTMONITOR),
					MENU_ITEM(ID_MONITOR_FIRST, ID_STR_VIRTUALSCREEN),
					MENU_SEPARATOR(),
					MENU_SEPARATOR(),
					MENU_ITEM(IDM_SKIN_MONITOR_AUTOSELECT, ID_STR_AUTOSELECTMONITOR)),
				MENU_SEPARATOR(),
				MENU_ITEM(IDM_SKIN_VERYTOPMOST, ID_STR_STAYTOPMOST),
				MENU_ITEM(IDM_SKIN_TOPMOST, ID_STR_TOPMOST),
				MENU_ITEM(IDM_SKIN_NORMAL, ID_STR_NORMAL),
				MENU_ITEM(IDM_SKIN_BOTTOM, ID_STR_BOTTOM),
				MENU_ITEM(IDM_SKIN_ONDESKTOP, ID_STR_ONDESKTOP),
				MENU_SEPARATOR(),
				MENU_ITEM(IDM_SKIN_FROMRIGHT, ID_STR_FROMRIGHT),
				MENU_ITEM(IDM_SKIN_FROMBOTTOM, ID_STR_FROMBOTTOM),
				MENU_ITEM(IDM_SKIN_XPERCENTAGE, ID_STR_XASPERCENTAGE),
				MENU_ITEM(IDM_SKIN_YPERCENTAGE, ID_STR_YASPERCENTAGE)),
			MENU_SUBMENU(ID_STR_TRANSPARENCY,
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_0, ID_STR_0PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_10, ID_STR_10PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_20, ID_STR_20PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_30, ID_STR_30PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_40, ID_STR_40PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_50, ID_STR_50PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_60, ID_STR_60PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_70, ID_STR_70PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_80, ID_STR_80PERCENT),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_90, ID_STR_90PERCENT),
				MENU_SEPARATOR(),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_FADEIN, ID_STR_FADEIN),
				MENU_ITEM(IDM_SKIN_TRANSPARENCY_FADEOUT, ID_STR_FADEOUT)),
			MENU_SEPARATOR(),
			MENU_ITEM(IDM_SKIN_HIDEONMOUSE, ID_STR_HIDEONMOUSEOVER),
			MENU_ITEM(IDM_SKIN_DRAGGABLE, ID_STR_DRAGGABLE),
			MENU_ITEM(IDM_SKIN_REMEMBERPOSITION, ID_STR_SAVEPOSITION),
			MENU_ITEM(IDM_SKIN_SNAPTOEDGES, ID_STR_SNAPTOEDGES),
			MENU_ITEM(IDM_SKIN_CLICKTHROUGH, ID_STR_CLICKTHROUGH),
			MENU_ITEM(IDM_SKIN_KEEPONSCREEN, ID_STR_KEEPONSCREEN)),
		MENU_SEPARATOR(),
		MENU_ITEM(IDM_SKIN_MANAGESKIN, ID_STR_MANAGESKIN),
		MENU_ITEM(IDM_SKIN_EDITSKIN, ID_STR_EDITSKIN),
		MENU_ITEM(IDM_SKIN_REFRESH, ID_STR_REFRESHSKIN),
		MENU_SEPARATOR(),
		MENU_ITEM(IDM_CLOSESKIN, ID_STR_UNLOADSKIN)
	};

	HMENU skinMenu = MenuTemplate::CreateMenu(s_Menu, _countof(s_Menu), GetString);
	if (!skinMenu) return nullptr;

	// Tick the position
	HMENU settingsMenu = GetSubMenu(skinMenu, 4);
	if (settingsMenu)
	{
		HMENU posMenu = GetSubMenu(settingsMenu, 0);
		if (posMenu)
		{
			const UINT checkPos = IDM_SKIN_NORMAL - (UINT)meterWindow->GetWindowZPosition();
			CheckMenuRadioItem(posMenu, checkPos, checkPos, checkPos, MF_BYCOMMAND);

			if (meterWindow->GetXFromRight()) CheckMenuItem(posMenu, IDM_SKIN_FROMRIGHT, MF_BYCOMMAND | MF_CHECKED);
			if (meterWindow->GetYFromBottom()) CheckMenuItem(posMenu, IDM_SKIN_FROMBOTTOM, MF_BYCOMMAND | MF_CHECKED);
			if (meterWindow->GetXPercentage()) CheckMenuItem(posMenu, IDM_SKIN_XPERCENTAGE, MF_BYCOMMAND | MF_CHECKED);
			if (meterWindow->GetYPercentage()) CheckMenuItem(posMenu, IDM_SKIN_YPERCENTAGE, MF_BYCOMMAND | MF_CHECKED);

			HMENU monitorMenu = GetSubMenu(posMenu, 0);
			if (monitorMenu)
			{
				CreateMonitorMenu(monitorMenu, meterWindow);
			}
		}

		// Tick the transparency
		HMENU alphaMenu = GetSubMenu(settingsMenu, 1);
		if (alphaMenu)
		{
			UINT checkPos = (UINT)(10 - meterWindow->GetAlphaValue() / 25.5);
			checkPos = min(9, checkPos);
			checkPos = max(0, checkPos);
			CheckMenuRadioItem(alphaMenu, checkPos, checkPos, checkPos, MF_BYPOSITION);

			switch (meterWindow->GetWindowHide())
			{
			case HIDEMODE_FADEIN:
				CheckMenuItem(alphaMenu, IDM_SKIN_TRANSPARENCY_FADEIN, MF_BYCOMMAND | MF_CHECKED);
				EnableMenuItem(alphaMenu, IDM_SKIN_TRANSPARENCY_FADEOUT, MF_BYCOMMAND | MF_GRAYED);
				break;

			case HIDEMODE_FADEOUT:
				CheckMenuItem(alphaMenu, IDM_SKIN_TRANSPARENCY_FADEOUT, MF_BYCOMMAND | MF_CHECKED);
				EnableMenuItem(alphaMenu, IDM_SKIN_TRANSPARENCY_FADEIN, MF_BYCOMMAND | MF_GRAYED);
				break;

			case HIDEMODE_HIDE:
				EnableMenuItem(alphaMenu, IDM_SKIN_TRANSPARENCY_FADEIN, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(alphaMenu, IDM_SKIN_TRANSPARENCY_FADEOUT, MF_BYCOMMAND | MF_GRAYED);
				break;
			}
		}

		// Tick the settings
		switch (meterWindow->GetWindowHide())
		{
		case HIDEMODE_HIDE:
			CheckMenuItem(settingsMenu, IDM_SKIN_HIDEONMOUSE, MF_BYCOMMAND | MF_CHECKED);
			break;

		case HIDEMODE_FADEIN:
		case HIDEMODE_FADEOUT:
			EnableMenuItem(settingsMenu, IDM_SKIN_HIDEONMOUSE, MF_BYCOMMAND | MF_GRAYED);
			break;
		}

		if (meterWindow->GetSnapEdges())
		{
			CheckMenuItem(settingsMenu, IDM_SKIN_SNAPTOEDGES, MF_BYCOMMAND | MF_CHECKED);
		}

		if (meterWindow->GetSavePosition())
		{
			CheckMenuItem(settingsMenu, IDM_SKIN_REMEMBERPOSITION, MF_BYCOMMAND | MF_CHECKED);
		}

		if (GetRainmeter().m_DisableDragging)
		{
			EnableMenuItem(settingsMenu, IDM_SKIN_DRAGGABLE, MF_BYCOMMAND | MF_GRAYED);
		}
		else if (meterWindow->GetWindowDraggable())
		{
			CheckMenuItem(settingsMenu, IDM_SKIN_DRAGGABLE, MF_BYCOMMAND | MF_CHECKED);
		}

		if (meterWindow->GetClickThrough())
		{
			CheckMenuItem(settingsMenu, IDM_SKIN_CLICKTHROUGH, MF_BYCOMMAND | MF_CHECKED);
		}

		if (meterWindow->GetKeepOnScreen())
		{
			CheckMenuItem(settingsMenu, IDM_SKIN_KEEPONSCREEN, MF_BYCOMMAND | MF_CHECKED);
		}
	}

	// Add the name of the Skin to the menu
	const std::wstring& skinName = meterWindow->GetFolderPath();
	ModifyMenu(skinMenu, IDM_SKIN_OPENSKINSFOLDER, MF_BYCOMMAND, IDM_SKIN_OPENSKINSFOLDER, skinName.c_str());
	SetMenuDefaultItem(skinMenu, IDM_SKIN_OPENSKINSFOLDER, FALSE);

	// Remove dummy menuitem from the variants menu
	HMENU variantsMenu = GetSubMenu(skinMenu, 2);
	if (variantsMenu)
	{
		DeleteMenu(variantsMenu, 0, MF_BYPOSITION);
	}

	// Give the menuitem the unique id that depends on the skin
	ChangeSkinIndex(skinMenu, index);

	// Add the variants menu
	if (variantsMenu)
	{
		const SkinRegistry::Folder& skinFolder = *GetRainmeter().m_SkinRegistry.FindFolder(skinName);
		for (int i = 0, isize = (int)skinFolder.files.size(); i < isize; ++i)
		{
			InsertMenu(variantsMenu, i, MF_BYPOSITION, skinFolder.baseID + i, skinFolder.files[i].c_str());
		}

		if (skinFolder.active)
		{
			UINT checkPos = skinFolder.active - 1;
			CheckMenuRadioItem(variantsMenu, checkPos, checkPos, checkPos, MF_BYPOSITION);
		}
	}

	// Add skin root menu
	int itemCount = GetMenuItemCount(menu);
	if (itemCount > 0)
	{
		std::wstring root = meterWindow->GetFolderPath();
		std::wstring::size_type pos = root.find_first_of(L'\\');
		if (pos != std::wstring::npos)
		{
			root.erase(pos);
		}

		for (int i = 0; i < itemCount; ++i)
		{
			const UINT state = GetMenuState(menu, i, MF_BYPOSITION);
			if (state == 0xFFFFFFFF || (state & MF_POPUP) == 0) break;

			WCHAR buffer[MAX_PATH];
			if (GetMenuString(menu, i, buffer, MAX_PATH, MF_BYPOSITION))
			{
				if (_wcsicmp(root.c_str(), buffer) == 0)
				{
					HMENU skinRootMenu = GetSubMenu(menu, i);
					if (skinRootMenu)
					{
						InsertMenu(skinMenu, 3, MF_BYPOSITION | MF_POPUP, (UINT_PTR)skinRootMenu, root.c_str());
					}
					break;
				}
			}
		}
	}

	// Add custom actions to the context menu
	std::wstring contextTitle = meterWindow->GetParser().ReadString(L"Rainmeter", L"ContextTitle", L"");
	if (!contextTitle.empty())
	{
		auto isTitleSeparator = [](const std::wstring& title)
		{
			return title.find_first_not_of(L'-') == std::wstring::npos;
		};

		std::wstring contextAction = meterWindow->GetParser().ReadString(L"Rainmeter", L"ContextAction", L"");
		if (!contextAction.empty() || isTitleSeparator(contextTitle))
		{
			std::vector<std::wstring> cTitles;
			WCHAR buffer[128];
			int i = 1;

			while (!contextTitle.empty() &&
					(!contextAction.empty() || isTitleSeparator(contextTitle)) &&
					(IDM_SKIN_CUSTOMCONTEXTMENU_FIRST + i - 1) <= IDM_SKIN_CUSTOMCONTEXTMENU_LAST) // Set maximum context items in resource.h
			{
				// Trim long titles
				if (contextTitle.size() > 30)
				{
					contextTitle.replace(27, contextTitle.size() - 27, L"...");
				}

				cTitles.push_back(contextTitle);

				_snwprintf_s(buffer, _TRUNCATE, L"ContextTitle%i", ++i);
				contextTitle = meterWindow->GetParser().ReadString(L"Rainmeter", buffer, L"");
				_snwprintf_s(buffer, _TRUNCATE, L"ContextAction%i", i);
				contextAction = meterWindow->GetParser().ReadString(L"Rainmeter", buffer, L"");
			}

			// Build a sub-menu if more than three items
			const size_t titleSize = cTitles.size();
			if (titleSize <= 3)
			{
				size_t position = 0;
				for (size_t i = 0; i < titleSize; ++i)
				{
					if (isTitleSeparator(cTitles[i]))
					{
						// Separators not allowed in main top-level menu
						--position;
					}
					else
					{
						const UINT_PTR id = (index << 16) | (IDM_SKIN_CUSTOMCONTEXTMENU_FIRST + i);
						InsertMenu(skinMenu, position + 1, MF_BYPOSITION | MF_STRING, id, cTitles[i].c_str());
					}

					++position;
				}

				if (position != 0)
				{
					InsertMenu(skinMenu, 1, MF_BYPOSITION | MF_STRING | MF_GRAYED, 0, L"Custom skin actions");
					InsertMenu(skinMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
				}
			}
			else
			{
				HMENU customMenu = CreatePopupMenu();
				InsertMenu(skinMenu, 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)customMenu, L"Custom skin actions");
				
				for (size_t i = 0; i < titleSize; ++i)
				{
					if (isTitleSeparator(cTitles[i]))
					{
						AppendMenu(customMenu, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
					}
					else
					{
						const UINT_PTR id = (index << 16) | (IDM_SKIN_CUSTOMCONTEXTMENU_FIRST + i);
						AppendMenu(customMenu, MF_BYPOSITION | MF_STRING, id, cTitles[i].c_str());
					}
				}

				InsertMenu(skinMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
			}
		}
	}

	return skinMenu;
}

int ContextMenu::CreateAllSkinsMenuRecursive(HMENU skinMenu, int index)
{
	SkinRegistry& skinRegistry = GetRainmeter().m_SkinRegistry;
	const int initialLevel = skinRegistry.GetFolder(index).level;
	int menuIndex = 0;

	const size_t max = skinRegistry.GetFolderCount();
	while (index < max)
	{
		const SkinRegistry::Folder& skinFolder = skinRegistry.GetFolder(index);
		if (skinFolder.level != initialLevel)
		{
			return index - 1;
		}

		HMENU subMenu = CreatePopupMenu();

		// Add current folder
		InsertMenu(skinMenu, menuIndex, MF_POPUP | MF_BYPOSITION, (UINT_PTR)subMenu, skinFolder.name.c_str());

		// Add subfolders
		const bool hasSubfolder = (index + 1) < max && skinRegistry.GetFolder(index + 1).level == initialLevel + 1;
		if (hasSubfolder)
		{
			index = CreateAllSkinsMenuRecursive(subMenu, index + 1);
		}

		// Add files
		{
			int fileIndex = 0;
			const int fileCount = (int)skinFolder.files.size();
			for ( ; fileIndex < fileCount; ++fileIndex)
			{
				InsertMenu(subMenu, fileIndex, MF_STRING | MF_BYPOSITION, skinFolder.baseID + fileIndex, skinFolder.files[fileIndex].c_str());
			}

			if (skinFolder.active)
			{
				UINT checkPos = skinFolder.active - 1;
				CheckMenuRadioItem(subMenu, checkPos, checkPos, checkPos, MF_BYPOSITION);
			}

			if (hasSubfolder && fileIndex != 0)
			{
				InsertMenu(subMenu, fileIndex, MF_SEPARATOR | MF_BYPOSITION, 0, nullptr);
			}
		}

		++menuIndex;
		++index;
	}

	return index;
}

void ContextMenu::CreateLayoutMenu(HMENU layoutMenu)
{
	const auto& layouts = GetRainmeter().m_Layouts;
	for (size_t i = 0, isize = layouts.size(); i < isize; ++i)
	{
		InsertMenu(layoutMenu, i, MF_BYPOSITION, ID_THEME_FIRST + i, layouts[i].c_str());
	}
}

void ContextMenu::CreateMonitorMenu(HMENU monitorMenu, MeterWindow* meterWindow)
{
	const bool screenDefined = meterWindow->GetXScreenDefined();
	const int screenIndex = meterWindow->GetXScreen();

	// for the "Specified monitor" (@n)
	const size_t numOfMonitors = System::GetMonitorCount();  // intentional
	const std::vector<MonitorInfo>& monitors = System::GetMultiMonitorInfo().monitors;

	int i = 1;
	for (auto iter = monitors.cbegin(); iter != monitors.cend(); ++iter, ++i)
	{
		WCHAR buffer[64];
		size_t len = _snwprintf_s(buffer, _TRUNCATE, L"@%i: ", i);

		std::wstring item(buffer, len);

		if ((*iter).monitorName.size() > 32)
		{
			item.append((*iter).monitorName, 0, 32);
			item += L"...";
		}
		else
		{
			item += (*iter).monitorName;
		}

		const UINT flags =
			MF_BYPOSITION |
			((screenDefined && screenIndex == i) ? MF_CHECKED : MF_UNCHECKED) |
			((*iter).active ? MF_ENABLED : MF_GRAYED);
		InsertMenu(monitorMenu, i + 2, flags, ID_MONITOR_FIRST + i, item.c_str());
	}

	if (!screenDefined)
	{
		CheckMenuItem(monitorMenu, IDM_SKIN_MONITOR_PRIMARY, MF_BYCOMMAND | MF_CHECKED);
	}

	if (screenDefined && screenIndex == 0)
	{
		CheckMenuItem(monitorMenu, ID_MONITOR_FIRST, MF_BYCOMMAND | MF_CHECKED);
	}

	if (meterWindow->GetAutoSelectScreen())
	{
		CheckMenuItem(monitorMenu, IDM_SKIN_MONITOR_AUTOSELECT, MF_BYCOMMAND | MF_CHECKED);
	}
}

void ContextMenu::ChangeSkinIndex(HMENU menu, int index)
{
	if (index > 0)
	{
		const int count = GetMenuItemCount(menu);
		for (int i = 0; i < count; ++i)
		{
			HMENU subMenu = GetSubMenu(menu, i);
			if (subMenu)
			{
				ChangeSkinIndex(subMenu, index);
			}
			else
			{
				MENUITEMINFO mii = {sizeof(MENUITEMINFO)};
				mii.fMask = MIIM_FTYPE | MIIM_ID;
				GetMenuItemInfo(menu, i, TRUE, &mii);
				if ((mii.fType & MFT_SEPARATOR) == 0)
				{
					mii.wID |= (index << 16);
					mii.fMask = MIIM_ID;
					SetMenuItemInfo(menu, i, TRUE, &mii);
				}
			}
		}
	}
}
