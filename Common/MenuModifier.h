// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <string>
#include <vector>
#include <Windows.h>

// Changes menu items while a modifier key is held down. Menus run their own modal message loop,
// so the registered items are kept in sync for as long as the menu is displayed.
class MenuModifier
{
public:
	MenuModifier() {}
	~MenuModifier();

	MenuModifier(const MenuModifier& other) = delete;
	MenuModifier& operator=(MenuModifier other) = delete;

	void AddItem(HMENU menu, int virtualKey, UINT command, UINT alternateCommand, const WCHAR* alternateText);
	void Start();
	void Stop();

private:
	struct Item
	{
		HMENU menu;
		int virtualKey;
		UINT command;
		UINT alternateCommand;
		std::wstring text;
		std::wstring alternateText;
		bool isAlternate;
	};

	static LRESULT CALLBACK MsgFilterProc(int code, WPARAM wParam, LPARAM lParam);

	void Update();

	std::vector<Item> m_Items;
	HHOOK m_Hook = nullptr;

	static MenuModifier* c_Active;
};
