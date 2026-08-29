// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MenuModifier.h"

MenuModifier* MenuModifier::c_Active = nullptr;

MenuModifier::~MenuModifier()
{
	Stop();
}

void MenuModifier::AddItem(HMENU menu, int virtualKey, UINT command, UINT alternateCommand, const WCHAR* alternateText)
{
	WCHAR text[256];
	const int length = GetMenuString(menu, command, text, _countof(text), MF_BYCOMMAND);
	if (length <= 0) return;

	m_Items.push_back(Item{ menu, virtualKey, command, alternateCommand, text, alternateText, false });
}

void MenuModifier::Start()
{
	assert(!m_Hook && !c_Active);
	if (m_Items.empty()) return;

	Update();

	c_Active = this;
	m_Hook = SetWindowsHookEx(WH_MSGFILTER, MsgFilterProc, nullptr, GetCurrentThreadId());
}

void MenuModifier::Stop()
{
	if (m_Hook)
	{
		UnhookWindowsHookEx(m_Hook);
		m_Hook = nullptr;
	}

	if (c_Active == this) c_Active = nullptr;
}

LRESULT CALLBACK MenuModifier::MsgFilterProc(int code, WPARAM wParam, LPARAM lParam)
{
	// Any message pumped by the menu loop is a chance to notice that a key was pressed or released.
	if (code == MSGF_MENU && c_Active) c_Active->Update();

	return CallNextHookEx(c_Active ? c_Active->m_Hook : nullptr, code, wParam, lParam);
}

void MenuModifier::Update()
{
	for (auto& item : m_Items)
	{
		const bool isAlternate = GetKeyState(item.virtualKey) < 0;
		if (isAlternate == item.isAlternate) continue;

		// SetMenuItemInfo is used instead of ModifyMenu so that the state of the item (checked,
		// grayed, default, ...) is preserved.
		MENUITEMINFO mii = { sizeof(MENUITEMINFO) };
		mii.fMask = MIIM_ID | MIIM_STRING;
		mii.wID = isAlternate ? item.alternateCommand : item.command;
		mii.dwTypeData = const_cast<WCHAR*>((isAlternate ? item.alternateText : item.text).c_str());

		const UINT command = item.isAlternate ? item.alternateCommand : item.command;
		if (SetMenuItemInfo(item.menu, command, FALSE, &mii))
		{
			item.isAlternate = isAlternate;
		}
	}
}
