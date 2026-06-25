/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../Common/MenuTemplate.h"
#include "../Common/Gfx/Canvas.h"
#include "ContextMenu.h"
#include "GameMode.h"
#include "Meter.h"
#include "Rainmeter.h"
#include "Util.h"
#include "Skin.h"
#include "System.h"
#include "MonitorUtil.h"
#include "TrayIcon.h"
#include "resource.h"

ContextMenu::ContextMenu() :
	m_ActiveMenu(nullptr)
{
}

template<typename Getter>
auto GetMatchingSkinValue(const std::vector<Skin*>& skins, Getter getter) -> std::optional<decltype(getter(skins[0]))>
{
	if (skins.empty()) return std::nullopt;

	auto value = getter(skins[0]);
	for (size_t i = 1; i < skins.size(); ++i)
	{
		if (getter(skins[i]) != value)
		{
			return std::nullopt;
		}
	}

	return value;
}

template<typename Getter>
void CheckMenuItemIfMatch(HMENU menu, UINT id, const std::vector<Skin*>& skins, Getter getter)
{
	const auto value = GetMatchingSkinValue(skins, getter);
	if (value && *value)
	{
		CheckMenuItem(menu, id, MF_BYCOMMAND | MF_CHECKED);
	}
}

bool IsMenuCommandChecked(HMENU menu, UINT command)
{
	if (!menu) return false;

	const int count = GetMenuItemCount(menu);
	for (int i = 0; i < count; ++i)
	{
		MENUITEMINFO mii = { sizeof(MENUITEMINFO) };
		mii.fMask = MIIM_ID | MIIM_STATE | MIIM_SUBMENU;

		if (!GetMenuItemInfo(menu, i, TRUE, &mii))
		{
			continue;
		}

		if (mii.wID == command)
		{
			return (mii.fState & MFS_CHECKED) != 0;
		}

		if (mii.hSubMenu && IsMenuCommandChecked(mii.hSubMenu, command))
		{
			return true;
		}
	}

	return false;
}

/*
** Opens the context menu in given coordinates.
*/
void ContextMenu::ShowMenu(POINT pos, Skin* skin)
{
	static const MenuTemplate s_Menu[] =
	{
		MENU_ITEM(IDM_MANAGE, IDS_Manage),
		MENU_ITEM(IDM_ABOUT, IDS_About),
		MENU_ITEM(IDM_SHOW_HELP, IDS_Help),
		MENU_SEPARATOR(),
		MENU_SUBMENU(IDS_Skins,
			MENU_ITEM(IDM_OPENSKINSFOLDER, IDS_OpenFolder),
			MENU_ITEM(IDM_DISABLEDRAG, IDS_DisableDragging),
			MENU_SEPARATOR(),
			MENU_ITEM_GRAYED(0, IDS_NoSkins)),
		MENU_SUBMENU(IDS_Favorites,
			MENU_ITEM_GRAYED(0, IDS_NoFavorites)),
		MENU_SUBMENU(IDS_Themes,
			MENU_ITEM_GRAYED(0, IDS_NoThemes)),
		MENU_SEPARATOR(),
		MENU_ITEM(IDM_EDITCONFIG, IDS_EditSettings),
		MENU_ITEM(IDM_REFRESH, IDS_RefreshAll),
		MENU_SEPARATOR(),
		MENU_SUBMENU(IDS_Logging,
			MENU_ITEM(IDM_SHOWLOGFILE, IDS_ShowLogFile),
			MENU_SEPARATOR(),
			MENU_ITEM(IDM_STARTLOG, IDS_StartLogging),
			MENU_ITEM(IDM_STOPLOG, IDS_StopLogging),
			MENU_SEPARATOR(),
			MENU_ITEM(IDM_DELETELOGFILE, IDS_DeleteLogFile),
			MENU_ITEM(IDM_DEBUGLOG, IDS_DebugMode)),
		MENU_SEPARATOR(),
		MENU_ITEM_GRAYED(0, IDS_GameMode),
		MENU_SEPARATOR(),
		MENU_ITEM(IDM_QUIT, IDS_Exit)
	};

	static const MenuTemplate s_GameModeMenu[] =
	{
		MENU_ITEM_GRAYED(0, IDS_GameMode),
		MENU_SEPARATOR(),
		MENU_ITEM(IDM_QUIT, IDS_Exit)
	};

	if (m_ActiveMenu || (skin && skin->IsClosing())) return;

	Rainmeter& rainmeter = GetRainmeter();

	// Show context menu, if no actions were executed
	HMENU menu = !GetGameMode().IsEnabled() ?
		MenuTemplate::CreateMenu(s_Menu, _countof(s_Menu), GetString) :
		MenuTemplate::CreateMenu(s_GameModeMenu, _countof(s_GameModeMenu), GetString);

	if (!menu) return;

	auto displayMenu = [&]() -> void
	{
		HWND hWnd = WindowFromPoint(pos);
		if (hWnd != nullptr)
		{
			Skin* s = rainmeter.GetSkin(hWnd);
			if (s)
			{
				// Cancel the mouse event beforehand
				s->SetMouseLeaveEvent(true);
			}
		}

		HWND parentWindow = skin ? skin->GetWindow() : rainmeter.m_TrayIcon->GetWindow();
		DisplayMenu(pos, menu, parentWindow);
	};

	int gamePos = GetMenuItemCount(menu) - 3;
	HMENU gameMenu = CreateGameModeMenu();
	if (gameMenu)
	{
		DeleteMenu(menu, gamePos, MF_BYPOSITION);
		InsertMenu(menu, gamePos, MF_BYPOSITION | MF_POPUP, (UINT_PTR)gameMenu, GetString(IDS_GameMode));
	}

	if (GetGameMode().IsEnabled())
	{
		displayMenu();
		return;
	}

	SetMenuDefaultItem(menu, IDM_MANAGE, MF_BYCOMMAND);

	if (_waccess_s(GetLogger().GetLogFilePath().c_str(), 0) != 0)
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
			// "Open folder" = 0, "Disable dragging" = 1, separator = 2
			DeleteMenu(allSkinsMenu, 3, MF_BYPOSITION);  // "No skins available" menuitem
			CreateAllSkinsMenu(allSkinsMenu);
		}

		if (rainmeter.m_DisableDragging)
		{
			CheckMenuItem(allSkinsMenu, IDM_DISABLEDRAG, MF_BYCOMMAND | MF_CHECKED);
		}
	}

	HMENU favoritesMenu = GetSubMenu(menu, 5);
	if (favoritesMenu)
	{
		if (!rainmeter.m_Favorites.empty())
		{
			DeleteMenu(favoritesMenu, 0, MF_BYPOSITION);  // "No skins available" menuitem
			CreateFavoritesMenu(favoritesMenu);
		}
	}

	HMENU layoutMenu = GetSubMenu(menu, 6);
	if (layoutMenu)
	{
		if (!rainmeter.m_Layouts.empty())
		{
			DeleteMenu(layoutMenu, 0, MF_BYPOSITION);  // "No layouts available" menuitem
			CreateLayoutMenu(layoutMenu);
		}
	}

	if (skin)
	{
		HMENU rainmeterMenu = menu;
		menu = CreateSkinMenu(skin, 0, allSkinsMenu);

		InsertMenu(menu, IDM_CLOSESKIN, MF_BYCOMMAND | MF_POPUP, (UINT_PTR)rainmeterMenu, L"Rainmeter");
		InsertMenu(menu, IDM_CLOSESKIN, MF_BYCOMMAND | MF_SEPARATOR, 0, nullptr);
	}
	else
	{
		// Create a menu for all active skins
		int index = 0;
		auto iter = rainmeter.m_Skins.cbegin();
		for (; iter != rainmeter.m_Skins.end(); ++iter)
		{
			if (index == 0)
			{
				InsertMenu(menu, 13, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
			}

			Skin* skin = ((*iter).second);
			HMENU skinMenu = CreateSkinMenu(skin, index, allSkinsMenu);
			InsertMenu(menu, 13, MF_BYPOSITION | MF_POPUP, (UINT_PTR)skinMenu, skin->GetFolderPath().c_str());
			++index;
		}

		bool newVersion = rainmeter.GetNewVersion();
		bool downloadedNewVersion = rainmeter.GetDownloadedNewVersion();
		bool obsoleteLanguage = rainmeter.GetLanguageStatus();
		int sepPos = 0;

		// Add update notification item
		if (newVersion || downloadedNewVersion)
		{
			UINT_PTR idm = downloadedNewVersion ? IDM_INSTALL_NEW_VERSION : IDM_NEW_VERSION;
			const WCHAR* str = GetString(downloadedNewVersion ? IDS_InstallNewVersion : IDS_UpdateAvailable);
			InsertMenu(menu, 0, MF_BYPOSITION, idm, str);
			++sepPos;
		}

		// Add language status if obsolete
		if (obsoleteLanguage)
		{
			InsertMenu(menu, !newVersion ? 0 : 1, MF_BYPOSITION, IDM_LANGUAGEOBSOLETE, GetString(IDS_LanguageObsolete));
			++sepPos;
		}

		// Add separator if necessary
		if (sepPos > 0)
		{
			InsertMenu(menu, sepPos, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
		}
	}

	displayMenu();
}

void ContextMenu::ShowSkinCustomMenu(POINT pos, Skin* skin)
{
	if (m_ActiveMenu || skin->IsClosing()) return;

	HMENU menu = CreatePopupMenu();
	AppendSkinCustomMenu(skin, 0, menu, true);

	DisplayMenu(pos, menu, skin->GetWindow());
}

void ContextMenu::ShowSkinSelectionMenu(POINT pos, Skin* skin, HWND parentWindow)
{
	if (m_ActiveMenu || skin->IsClosing()) return;

	HMENU menu = CreateSkinSelectionMenu();
	if (!menu)
	{
		return;
	}

	DisplayMenu(pos, menu, parentWindow);
}

void ContextMenu::DisplayMenu(POINT pos, HMENU menu, HWND parentWindow)
{
	m_ActiveMenu = menu;

	// Set the window to foreground
	HWND foregroundWindow = GetForegroundWindow();
	if (foregroundWindow != parentWindow)
	{
		const DWORD foregroundThreadID = GetWindowThreadProcessId(foregroundWindow, nullptr);
		const DWORD currentThreadID = GetCurrentThreadId();
		AttachThreadInput(currentThreadID, foregroundThreadID, TRUE);
		SetForegroundWindow(parentWindow);
		AttachThreadInput(currentThreadID, foregroundThreadID, FALSE);
	}

	// Disable each meter's tooltip
	auto skin = GetRainmeter().GetSkin(parentWindow);
	if (skin) for (const auto& meter : skin->GetMeters()) meter->DisableToolTip();

	// Show context menu
	UINT command = TrackPopupMenu(
		menu,
		TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_LEFTALIGN | (GetRainmeter().IsLanguageRTL() ? TPM_LAYOUTRTL : 0),
		pos.x,
		pos.y,
		0,
		parentWindow,
		nullptr);

	if (skin)
	{
		for (const auto& meter : skin->GetMeters()) meter->ResetToolTip();
	}

	if (command)
	{
		// WM_COMMAND doesn't use lParam for anything so repurpose it for the checked state.
		const LPARAM checked = IsMenuCommandChecked(menu, command) ? 1 : 0;
		SendMessage(parentWindow, WM_COMMAND, command, checked);
	}

	m_ActiveMenu = nullptr;
	DestroyMenu(menu);
}

// TODO: Get rid of this after adding these labels to the language files.
const WCHAR* GetStringTemp(UINT id)
{
	switch (id)
	{
		case IDS_Zoom: return L"Zoom";
		case IDS_0Percent: return L"0%";
		case IDS_10Percent: return L"10%";
		case IDS_20Percent: return L"20%";
		case IDS_30Percent: return L"30%";
		case IDS_40Percent: return L"40%";
		case IDS_50Percent: return L"50%";
		case IDS_60Percent: return L"60%";
		case IDS_70Percent: return L"70%";
		case IDS_80Percent: return L"80%";
		case IDS_90Percent: return L"90%";
		case IDS_Approx100Percent: return L"~100%";
		case IDS_100Percent: return L"100%";
		case IDS_110Percent: return L"110%";
		case IDS_120Percent: return L"120%";
		case IDS_130Percent: return L"130%";
		case IDS_140Percent: return L"140%";
		case IDS_150Percent: return L"150%";
	}

	return GetString(id);
}

HMENU ContextMenu::CreateSkinSettingsMenu(const std::vector<Skin*>& skins)
{
	static const MenuTemplate s_Menu[] =
	{
		MENU_SUBMENU(IDS_Position,
			MENU_SUBMENU(IDS_DisplayMonitor,
				MENU_ITEM(IDM_SKIN_MONITOR_PRIMARY, IDS_UseDefaultMonitor),
				MENU_ITEM(ID_MONITOR_FIRST, IDS_VirtualScreen),
				MENU_SEPARATOR(),
				MENU_SEPARATOR(),
				MENU_ITEM(IDM_SKIN_MONITOR_AUTOSELECT, IDS_AutoSelectMonitor)),
			MENU_SEPARATOR(),
			MENU_ITEM(IDM_SKIN_VERYTOPMOST, IDS_StayTopmost),
			MENU_ITEM(IDM_SKIN_TOPMOST, IDS_Topmost),
			MENU_ITEM(IDM_SKIN_NORMAL, IDS_Normal),
			MENU_ITEM(IDM_SKIN_BOTTOM, IDS_Bottom),
			MENU_ITEM(IDM_SKIN_ONDESKTOP, IDS_OnDesktop),
			MENU_SEPARATOR(),
			MENU_ITEM(IDM_SKIN_FROMRIGHT, IDS_FromRight),
			MENU_ITEM(IDM_SKIN_FROMBOTTOM, IDS_FromBottom),
			MENU_ITEM(IDM_SKIN_XPERCENTAGE, IDS_XAsPercentage),
			MENU_ITEM(IDM_SKIN_YPERCENTAGE, IDS_YAsPercentage)),
		MENU_SUBMENU(IDS_Zoom,
			MENU_ITEM(IDM_SKIN_ZOOM_80, IDS_80Percent),
			MENU_ITEM(IDM_SKIN_ZOOM_90, IDS_90Percent),
			MENU_ITEM(IDM_SKIN_ZOOM_100, IDS_100Percent),
			MENU_ITEM(IDM_SKIN_ZOOM_110, IDS_110Percent),
			MENU_ITEM(IDM_SKIN_ZOOM_120, IDS_120Percent),
			MENU_ITEM(IDM_SKIN_ZOOM_130, IDS_130Percent),
			MENU_ITEM(IDM_SKIN_ZOOM_140, IDS_140Percent),
			MENU_ITEM(IDM_SKIN_ZOOM_150, IDS_150Percent)),
		MENU_SEPARATOR(),
		MENU_SUBMENU(IDS_Transparency,
			MENU_ITEM(IDM_SKIN_TRANSPARENCY_0, IDS_0Percent),
			MENU_ITEM(IDM_SKIN_TRANSPARENCY_10, IDS_10Percent),
			MENU_ITEM(IDM_SKIN_TRANSPARENCY_20, IDS_20Percent),
			MENU_ITEM(IDM_SKIN_TRANSPARENCY_30, IDS_30Percent),
			MENU_ITEM(IDM_SKIN_TRANSPARENCY_40, IDS_40Percent),
			MENU_ITEM(IDM_SKIN_TRANSPARENCY_50, IDS_50Percent),
			MENU_ITEM(IDM_SKIN_TRANSPARENCY_60, IDS_60Percent),
			MENU_ITEM(IDM_SKIN_TRANSPARENCY_70, IDS_70Percent),
			MENU_ITEM(IDM_SKIN_TRANSPARENCY_80, IDS_80Percent),
			MENU_ITEM(IDM_SKIN_TRANSPARENCY_90, IDS_90Percent),
			MENU_ITEM(IDM_SKIN_TRANSPARENCY_100, IDS_Approx100Percent)),
		MENU_SUBMENU(IDS_OnHover,
			MENU_ITEM(IDM_SKIN_HIDEONMOUSE_NONE, IDS_DoNothing),
			MENU_ITEM(IDM_SKIN_HIDEONMOUSE, IDS_Hide),
			MENU_ITEM(IDM_SKIN_TRANSPARENCY_FADEIN, IDS_FadeIn),
			MENU_ITEM(IDM_SKIN_TRANSPARENCY_FADEOUT, IDS_FadeOut)),
		MENU_SEPARATOR(),
		MENU_ITEM(IDM_SKIN_CLICKTHROUGH, IDS_ClickThrough),
		MENU_ITEM(IDM_SKIN_DRAGGABLE, IDS_Draggable),
		MENU_ITEM(IDM_SKIN_KEEPONSCREEN, IDS_KeepOnScreen),
		MENU_ITEM(IDM_SKIN_REMEMBERPOSITION, IDS_SavePosition),
		MENU_ITEM(IDM_SKIN_SNAPTOEDGES, IDS_SnapToEdges),
		MENU_ITEM(IDM_SKIN_FAVORITE, IDS_Favorite)
	};

	HMENU settingsMenu = MenuTemplate::CreateMenu(s_Menu, _countof(s_Menu), GetStringTemp);
	if (skins.empty()) return settingsMenu;

	HMENU posMenu = GetSubMenu(settingsMenu, offset);
	if (posMenu)
	{
		if (const auto zPos = GetMatchingSkinValue(skins, [](Skin* skin) { return skin->GetWindowZPosition(); }))
		{
			const UINT checkPos = IDM_SKIN_NORMAL - (UINT)*zPos;
			CheckMenuRadioItem(posMenu, checkPos, checkPos, checkPos, MF_BYCOMMAND);
		}

		CheckMenuItemIfMatch(posMenu, IDM_SKIN_FROMRIGHT, skins, [](Skin* skin) { return skin->GetX().fromOpposite; });
		CheckMenuItemIfMatch(posMenu, IDM_SKIN_FROMBOTTOM, skins, [](Skin* skin) { return skin->GetY().fromOpposite; });
		CheckMenuItemIfMatch(posMenu, IDM_SKIN_XPERCENTAGE, skins, [](Skin* skin) { return skin->GetX().percentage; });
		CheckMenuItemIfMatch(posMenu, IDM_SKIN_YPERCENTAGE, skins, [](Skin* skin) { return skin->GetY().percentage; });

		HMENU monitorMenu = GetSubMenu(posMenu, 0);
		if (monitorMenu)
		{
			CreateMonitorMenu(monitorMenu, skins[0]);
			if (!GetMatchingSkinValue(skins, [](Skin* skin) { return skin->GetX().monitor; }))
			{
				for (int i = 0, count = GetMenuItemCount(monitorMenu); i < count; ++i)
				{
					CheckMenuItem(monitorMenu, i, MF_BYPOSITION | MF_UNCHECKED);
				}
			}

			const auto autoSelectScreen = GetMatchingSkinValue(skins, [](Skin* skin) { return skin->GetAutoSelectScreen(); });
			if (!autoSelectScreen || !*autoSelectScreen)
			{
				CheckMenuItem(monitorMenu, IDM_SKIN_MONITOR_AUTOSELECT, MF_BYCOMMAND | MF_UNCHECKED);
			}
		}
	}

	HMENU zoomMenu = GetSubMenu(settingsMenu, offset + 1);
	if (zoomMenu)
	{
		const auto sharedZoom = GetMatchingSkinValue(skins, [](Skin* skin) { return skin->GetZoom(); });
		if (sharedZoom)
		{
			const float zoom = *sharedZoom;
			const int zoomPercent = (int)(zoom * 100.0f + 0.5f);
			UINT checkId = 0;
			static const float c_Zooms[] = { 0.8f, 0.9f, 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f };
			for (UINT i = 0; i < _countof(c_Zooms); ++i)
			{
				if (fabsf(zoom - c_Zooms[i]) <= 0.0001f)
				{
					checkId = IDM_SKIN_ZOOM_80 + i;
					break;
				}
			}

			if (checkId == 0)
			{
				WCHAR buffer[32];
				_snwprintf_s(buffer, _TRUNCATE, L"%i%%", zoomPercent);

				UINT position = _countof(c_Zooms);
				for (UINT i = 0; i < _countof(c_Zooms); ++i)
				{
					const int itemPercent = (int)(c_Zooms[i] * 100.0f + 0.5f);
					if (zoomPercent < itemPercent)
					{
						position = i;
						break;
					}
				}

				InsertMenu(zoomMenu, position, MF_BYPOSITION | MF_STRING, IDM_SKIN_ZOOM_CUSTOM, buffer);
				CheckMenuRadioItem(zoomMenu, IDM_SKIN_ZOOM_80, IDM_SKIN_ZOOM_CUSTOM, IDM_SKIN_ZOOM_CUSTOM, MF_BYCOMMAND);
			}
			else
			{
				CheckMenuRadioItem(zoomMenu, IDM_SKIN_ZOOM_80, IDM_SKIN_ZOOM_150, checkId, MF_BYCOMMAND);
			}
		}
	}

	HMENU alphaMenu = GetSubMenu(settingsMenu, offset + 3);
	if (alphaMenu)
	{
		if (const auto alpha = GetMatchingSkinValue(skins, [](Skin* skin) { return skin->GetAlphaValue(); }))
		{
			UINT checkPos = *alpha <= 1 ? 10 : (UINT)max(0, min(9, (int)(10 - *alpha / 25.5)));
			CheckMenuRadioItem(alphaMenu, checkPos, checkPos, checkPos, MF_BYPOSITION);
		}
	}

	HMENU hoverMenu = GetSubMenu(settingsMenu, offset + 4);
	if (hoverMenu)
	{
		if (auto mode = GetMatchingSkinValue(skins, [](Skin* skin) { return skin->GetWindowHide(); }))
		{
			*mode = min(3, max(0, *mode));
			CheckMenuRadioItem(hoverMenu, *mode, *mode, *mode, MF_BYPOSITION);
		}
	}

	CheckMenuItemIfMatch(settingsMenu, IDM_SKIN_SNAPTOEDGES, skins, [](Skin* skin) {
		return skin->GetSnapEdges();
	});

	CheckMenuItemIfMatch(settingsMenu, IDM_SKIN_REMEMBERPOSITION, skins, [](Skin* skin) {
		return skin->GetSavePosition();
	});

	CheckMenuItemIfMatch(settingsMenu, IDM_SKIN_DRAGGABLE, skins, [&](Skin* skin) {
		return skin->IsSelected() ? skin->m_OldWindowDraggable : skin->GetWindowDraggable();
	});

	CheckMenuItemIfMatch(settingsMenu, IDM_SKIN_CLICKTHROUGH, skins, [&](Skin* skin) {
		return skin->IsSelected() ? skin->m_OldClickThrough : skin->GetClickThrough();
	});

	CheckMenuItemIfMatch(settingsMenu, IDM_SKIN_KEEPONSCREEN, skins, [&](Skin* skin) {
		return skin->IsSelected() ? skin->m_OldKeepOnScreen : skin->GetKeepOnScreen();
	});

	CheckMenuItemIfMatch(settingsMenu, IDM_SKIN_FAVORITE, skins, [](Skin* skin) {
		return skin->GetFavorite();
	});

	if (GetRainmeter().m_DisableDragging)
	{
		EnableMenuItem(settingsMenu, IDM_SKIN_DRAGGABLE, MF_BYCOMMAND | MF_GRAYED);
	}
}

HMENU ContextMenu::CreateSkinSelectionMenu()
{
	std::vector<Skin*> selectedSkins;
	for (const auto& skins : GetRainmeter().GetAllSkins())
	{
		Skin* skin = skins.second;
		if (skin->IsSelected())
		{
			selectedSkins.push_back(skin);
		}
	}

	HMENU menu = CreateSkinSettingsMenu(selectedSkins, true);

	InsertMenu(menu, 0, MF_BYPOSITION | MF_STRING, IDM_SKIN_SELECT, L"Select all skins");
	InsertMenu(menu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);

	AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
	AppendMenu(menu, MF_STRING, IDM_SKIN_REFRESH, L"Refresh selection");
	AppendMenu(menu, MF_STRING, IDM_CLOSESKIN, L"Unload selection");

	return menu;
}

HMENU ContextMenu::CreateSkinMenu(Skin* skin, int index, HMENU menu)
{
	static const MenuTemplate s_Menu[] =
	{
		MENU_ITEM(IDM_SKIN_OPENSKINSFOLDER, 0),
		MENU_SEPARATOR(),
		MENU_SUBMENU(IDS_Variants,
			MENU_SEPARATOR()),
		MENU_SEPARATOR(),
		MENU_ITEM_GRAYED(0, IDS_Settings),
		MENU_SEPARATOR(),
		MENU_ITEM(IDM_SKIN_MANAGESKIN, IDS_ManageSkin),
		MENU_ITEM(IDM_SKIN_EDITSKIN, IDS_EditSkin),
		MENU_ITEM(IDM_SKIN_REFRESH, IDS_RefreshSkin),
		MENU_SEPARATOR(),
		MENU_ITEM(IDM_CLOSESKIN, IDS_UnloadSkin)
	};

	HMENU skinMenu = MenuTemplate::CreateMenu(s_Menu, _countof(s_Menu), GetStringTemp);

	HMENU settingsMenu = CreateSkinSettingsMenu({ skin });
	DeleteMenu(skinMenu, 4, MF_BYPOSITION);
	InsertMenu(skinMenu, 4, MF_BYPOSITION | MF_POPUP, (UINT_PTR)settingsMenu, GetString(IDS_Settings));

	// Add the name of the Skin to the menu
	const std::wstring& skinName = skin->GetFolderPath();
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
			InsertMenu(variantsMenu, i, MF_BYPOSITION, skinFolder.baseID + i, skinFolder.files[i].filename.c_str());
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
		std::wstring root = skin->GetFolderPath();
		std::wstring::size_type pos = root.find_first_of(L'\\');
		if (pos != std::wstring::npos)
		{
			root.erase(pos);
		}

		// Skip "Open folder", "Disable dragging" and a separator
		for (int i = 3; i < itemCount; ++i)
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

	AppendSkinCustomMenu(skin, index, skinMenu, false);

	return skinMenu;
}

void ContextMenu::AppendSkinCustomMenu(
	Skin* skin, int index, HMENU menu, bool standaloneMenu)
{
	// Add custom actions to the context menu
	std::wstring contextTitle = skin->GetParser().ReadString(L"Rainmeter", L"ContextTitle", L"");
	if (contextTitle.empty())
	{
		return;
	}

	auto isTitleSeparator = [](const std::wstring& title)
	{
		return title.find_first_not_of(L'-') == std::wstring::npos;
	};

	std::wstring contextAction = skin->GetParser().ReadString(L"Rainmeter", L"ContextAction", L"");
	if (contextAction.empty() && !isTitleSeparator(contextTitle))
	{
		return;
	}

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
		contextTitle = skin->GetParser().ReadString(L"Rainmeter", buffer, L"");
		_snwprintf_s(buffer, _TRUNCATE, L"ContextAction%i", i);
		contextAction = skin->GetParser().ReadString(L"Rainmeter", buffer, L"");
	}

	// Build a sub-menu if more than three items
	const size_t titleSize = cTitles.size();
	if (titleSize <= 3 || standaloneMenu)
	{
		size_t position = 0;
		for (size_t i = 0; i < titleSize; ++i)
		{
			if (isTitleSeparator(cTitles[i]))
			{
				if (standaloneMenu)
				{
					AppendMenu(menu, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
				}
				else
				{
					// Separators not allowed in main top-level menu
					--position;
				}
			}
			else
			{
				const UINT_PTR id = ((UINT_PTR)index << 16) | (IDM_SKIN_CUSTOMCONTEXTMENU_FIRST + i);
				InsertMenu(menu, (UINT)(position + 1), MF_BYPOSITION | MF_STRING, id, cTitles[i].c_str());
			}

			++position;
		}

		if (position != 0 && !standaloneMenu)
		{
			InsertMenu(menu, 1, MF_BYPOSITION | MF_STRING | MF_GRAYED, 0, GetString(IDS_CustomSkinActions));
			InsertMenu(menu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
		}
	}
	else
	{
		HMENU customMenu = CreatePopupMenu();
		InsertMenu(menu, 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)customMenu, GetString(IDS_CustomSkinActions));

		for (size_t i = 0; i < titleSize; ++i)
		{
			if (isTitleSeparator(cTitles[i]))
			{
				AppendMenu(customMenu, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
			}
			else
			{
				const UINT_PTR id = ((UINT_PTR)index << 16) | (IDM_SKIN_CUSTOMCONTEXTMENU_FIRST + i);
				AppendMenu(customMenu, MF_BYPOSITION | MF_STRING, id, cTitles[i].c_str());
			}
		}

		InsertMenu(menu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
	}
}

int ContextMenu::CreateSkinsMenuRecursive(HMENU skinMenu, int index, bool isFavoriteMenu)
{
	SkinRegistry& skinRegistry = GetRainmeter().m_SkinRegistry;
	const int initialLevel = skinRegistry.GetFolder(index).level;

	// For "Skins" menu, index starts at 3 ("Open folder" = 0, "Disable dragging" = 1, separator = 2)
	// For "Favorites" menu, index starts at 0
	int menuIndex = isFavoriteMenu ? 0 : 3;

	const int max = (int)skinRegistry.GetFolderCount();
	while (index < max)
	{
		const SkinRegistry::Folder& skinFolder = skinRegistry.GetFolder(index);

		if (!isFavoriteMenu || (isFavoriteMenu && skinFolder.hasFavorite))
		{
			if (skinFolder.level != initialLevel)
			{
				return index - 1;
			}

			HMENU subMenu = isFavoriteMenu ? skinMenu : CreatePopupMenu();

			// Add current folder
			if (!isFavoriteMenu)
			{
				InsertMenu(skinMenu, menuIndex, MF_POPUP | MF_BYPOSITION, (UINT_PTR)subMenu, skinFolder.name.c_str());
			}

			// Add subfolders
			const bool hasSubfolder = (index + 1) < max && skinRegistry.GetFolder(index + 1).level == initialLevel + 1;
			if (hasSubfolder)
			{
				index = CreateSkinsMenuRecursive(subMenu, index + 1, isFavoriteMenu);
			}

			// Add files
			{
				std::wstring filename;
				SkinRegistry::Indexes indexes;
				bool hasFavorite = false;
				int fileIndex = 0;
				const int fileCount = (int)skinFolder.files.size();

				for (; fileIndex < fileCount; ++fileIndex)
				{
					if (!isFavoriteMenu || (isFavoriteMenu && skinFolder.files[fileIndex].isFavorite))
					{
						filename = skinFolder.files[fileIndex].filename;

						if (isFavoriteMenu)
						{
							indexes = skinRegistry.FindIndexesForID(skinFolder.baseID);
							filename.insert(0, L"\\");
							filename.insert(0, skinRegistry.GetFolderPath(indexes.folder));
						}

						InsertMenu(subMenu, isFavoriteMenu ? -1 : fileIndex, MF_STRING | MF_BYPOSITION, skinFolder.baseID + fileIndex, filename.c_str());
						hasFavorite = true;
					}
				}

				if (skinFolder.active)
				{
					UINT checkId = skinFolder.baseID + skinFolder.active - 1;
					CheckMenuRadioItem(subMenu, checkId, checkId, checkId, MF_BYCOMMAND);
				}

				if (!isFavoriteMenu && hasSubfolder && fileIndex != 0)
				{
					InsertMenu(subMenu, fileIndex, MF_SEPARATOR | MF_BYPOSITION, 0, nullptr);
				}
			}

			++menuIndex;
		}

		++index;
	}

	return index;
}

void ContextMenu::CreateLayoutMenu(HMENU layoutMenu)
{
	const auto& layouts = GetRainmeter().m_Layouts;
	for (size_t i = 0, isize = layouts.size(); i < isize; ++i)
	{
		InsertMenu(layoutMenu, (UINT)i, MF_BYPOSITION, ID_THEME_FIRST + i, layouts[i].c_str());
	}
}

void ContextMenu::CreateMonitorMenu(HMENU monitorMenu, Skin* skin)
{
	const bool monitorDefined = skin->GetX().monitor.has_value();
	const int monitor = skin->GetX().monitor.value_or(0);

	// for the "Specified monitor" (@n)
	const auto& monitors = MonitorUtil::GetMultiMonitorInfo().monitors;

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
			((monitorDefined && monitor == i) ? MF_CHECKED : MF_UNCHECKED) |
			((*iter).active ? MF_ENABLED : MF_GRAYED);
		InsertMenu(monitorMenu, i + 2, flags, ID_MONITOR_FIRST + i, item.c_str());
	}

	if (!monitorDefined)
	{
		CheckMenuItem(monitorMenu, IDM_SKIN_MONITOR_PRIMARY, MF_BYCOMMAND | MF_CHECKED);
	}

	if (monitorDefined && monitor == 0)
	{
		CheckMenuItem(monitorMenu, ID_MONITOR_FIRST, MF_BYCOMMAND | MF_CHECKED);
	}

	if (skin->GetAutoSelectScreen())
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

HMENU ContextMenu::CreateGameModeOnStartMenu()
{
	static const MenuTemplate s_Menu[] =
	{
		MENU_ITEM(ID_GAMEMODE_ONSTART_FIRST, IDS_GameModeActionsUnloadAll)
	};

	HMENU menu = MenuTemplate::CreateMenu(s_Menu, _countof(s_Menu), GetString);
	if (!menu) return nullptr;

	std::wstring& action = GetGameMode().GetOnStartAction();
	bool checked = false;

	const auto& layouts = GetRainmeter().m_Layouts;
	if (layouts.size() > 0)
	{
		InsertMenu(menu, 1, MF_BYPOSITION, MF_SEPARATOR, nullptr);

		for (size_t i = 0, isize = layouts.size(); i < isize; ++i)
		{
			LPCWSTR item = layouts[i].c_str();
			UINT pos = (UINT)(i + 2);
			InsertMenu(menu, pos, MF_BYPOSITION, ID_GAMEMODE_ONSTART_FIRST + i + 1, item);
			if (_wcsicmp(item, action.c_str()) == 0)
			{
				CheckMenuRadioItem(menu, pos, pos, pos, MF_BYPOSITION);
				checked = true;
			}
		}
	}

	if (!checked)
	{
		CheckMenuRadioItem(menu, 0, 0, 0, MF_BYPOSITION);
	}

	return menu;
}

HMENU ContextMenu::CreateGameModeOnStopMenu()
{
	static const MenuTemplate s_Menu[] =
	{
		MENU_ITEM(ID_GAMEMODE_ONSTOP_FIRST, IDS_GameModeActionsCurrent)
	};

	HMENU menu = MenuTemplate::CreateMenu(s_Menu, _countof(s_Menu), GetString);
	if (!menu) return nullptr;

	std::wstring& action = GetGameMode().GetOnStopAction();
	bool checked = false;

	const auto& layouts = GetRainmeter().m_Layouts;
	if (layouts.size() > 0)
	{
		InsertMenu(menu, 1, MF_BYPOSITION, MF_SEPARATOR, nullptr);

		for (size_t i = 0, isize = layouts.size(); i < isize; ++i)
		{
			LPCWSTR item = layouts[i].c_str();
			UINT pos = (UINT)(i + 2);
			InsertMenu(menu, pos, MF_BYPOSITION, ID_GAMEMODE_ONSTOP_FIRST + i + 1, item);
			if (_wcsicmp(item, action.c_str()) == 0)
			{
				CheckMenuRadioItem(menu, pos, pos, pos, MF_BYPOSITION);
				checked = true;
			}
		}
	}

	if (!checked)
	{
		CheckMenuRadioItem(menu, 0, 0, 0, MF_BYPOSITION);
	}

	return menu;
}

HMENU ContextMenu::CreateGameModeMenu()
{
	static const MenuTemplate s_Menu[] =
	{
		MENU_ITEM(IDM_GAMEMODE_START, IDS_GameModeStart),
		MENU_SEPARATOR(),
		MENU_ITEM_GRAYED(IDM_GAMEMODE_FULLSCREEN, IDS_GameModeFullScreen),
		MENU_ITEM_GRAYED(IDM_GAMEMODE_PROCESSLIST, IDS_GameModeProcessList),
		MENU_SEPARATOR(),
		MENU_ITEM_GRAYED(0, IDS_GameModeActionsOnStart),
		MENU_ITEM_GRAYED(0, IDS_GameModeActionsOnStop)
	};

	HMENU menu = MenuTemplate::CreateMenu(s_Menu, _countof(s_Menu), GetString);
	if (!menu) return nullptr;

	GameMode& game = GetGameMode();
	bool enabled = game.IsEnabled();

	// If game is enabled (or in layout mode), change item 0 to "Disable"
	if (!game.IsDisabled())
	{
		DeleteMenu(menu, 0, MF_BYPOSITION);
		InsertMenu(menu, 0, MF_BYPOSITION, IDM_GAMEMODE_STOP, GetString(IDS_GameModeStop));
	}

	// Tick the settings
	if (game.GetFullScreenMode())
	{
		CheckMenuItem(menu, IDM_GAMEMODE_FULLSCREEN, MF_BYCOMMAND | MF_CHECKED);
	}

	if (game.GetProcessListMode())
	{
		CheckMenuItem(menu, IDM_GAMEMODE_PROCESSLIST, MF_BYCOMMAND | MF_CHECKED);
	}

	// Only allow changing of settings if not enabled (layout enabled or disabled is okay)
	if (!game.IsEnabled())
	{
		EnableMenuItem(menu, IDM_GAMEMODE_FULLSCREEN, MF_ENABLED);
		EnableMenuItem(menu, IDM_GAMEMODE_PROCESSLIST, MF_ENABLED);

		HMENU onStartMenu = CreateGameModeOnStartMenu();
		if (onStartMenu)
		{
			DeleteMenu(menu, 5, MF_BYPOSITION);
			InsertMenu(menu, 5, MF_BYPOSITION | MF_POPUP, (UINT_PTR)onStartMenu, GetString(IDS_GameModeActionsOnStart));
		}

		HMENU onStopMenu = CreateGameModeOnStopMenu();
		if (onStopMenu)
		{
			DeleteMenu(menu, 6, MF_BYPOSITION);
			InsertMenu(menu, 6, MF_BYPOSITION | MF_POPUP, (UINT_PTR)onStopMenu, GetString(IDS_GameModeActionsOnStop));
		}
	}

	return menu;
}
