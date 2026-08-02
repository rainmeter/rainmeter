// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <vector>
#include <Windows.h>

class Skin;

// Handles the creation and display of Rainmeter and skin context menus.
class ContextMenu
{
public:
	ContextMenu();

	ContextMenu(const ContextMenu& other) = delete;
	ContextMenu& operator=(ContextMenu other) = delete;

	bool IsMenuActive() { return m_ActiveMenu != nullptr; }

	void ShowMenu(POINT pos, Skin* skin, HWND parentWindow = nullptr);
	void ShowSkinCustomMenu(POINT pos, Skin* skin);
	void ShowSkinSelectionMenu(POINT pos, Skin* skin, HWND parentWindow);

	static void CreateMonitorMenu(HMENU monitorMenu, Skin* skin);

	static HMENU CreateGameModeOnStartMenu();
	static HMENU CreateGameModeOnStopMenu();

private:
	void DisplayMenu(POINT pos, HMENU menu, HWND parentWindow, HWND commandWindow = nullptr);

	static HMENU CreateSkinMenu(Skin* skin, int index, HMENU menu);
	static HMENU CreateSkinSettingsMenu(const std::vector<Skin*>& skins);
	static HMENU CreateSkinSelectionMenu();
	static void AppendSkinCustomMenu(
		Skin* skin, int index, HMENU menu, bool standaloneMenu);
	static void ChangeSkinIndex(HMENU subMenu, int index);

	static void CreateAllSkinsMenu(HMENU skinMenu) { CreateSkinsMenuRecursive(skinMenu, 0, false); }
	static int CreateSkinsMenuRecursive(HMENU skinMenu, int index, bool isFavoriteMenu);

	static void CreateLayoutMenu(HMENU layoutMenu);

	static void CreateFavoritesMenu(HMENU favoriteMenu) { CreateSkinsMenuRecursive(favoriteMenu, 0, true); }

	HMENU m_ActiveMenu;
};
