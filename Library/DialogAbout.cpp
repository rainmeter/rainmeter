/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Rainmeter.h"
#include "System.h"
#include "resource.h"
#include "DialogAbout.h"
#include "CommandHandler.h"
#include "Util.h"
#include "../Version.h"
#include "../Common/Platform.h"

WINDOWPLACEMENT DialogAbout::c_WindowPlacement = { 0 };
DialogAbout* DialogAbout::c_Dialog = nullptr;

DialogAbout::DialogAbout() : Dialog(&c_WindowPlacement)
{
}

DialogAbout::~DialogAbout()
{
}

void DialogAbout::Open()
{
	if (!c_Dialog)
	{
		c_Dialog = new DialogAbout();
	}

	c_Dialog->ShowDialogWindow(
		GetString(IDS_AboutRainmeter),
		0, 0, 416, 172,
		DS_CENTER | WS_POPUP | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU,
		WS_EX_APPWINDOW | WS_EX_CONTROLPARENT | (GetRainmeter().IsLanguageRTL() ? WS_EX_LAYOUTRTL : 0),
		nullptr);
}

INT_PTR DialogAbout::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const INT_PTR baseResult = Dialog::HandleMessage(uMsg, wParam, lParam);

	switch (uMsg)
	{
	case WM_INITDIALOG:
		return OnInitDialog(wParam, lParam);

	case WM_COMMAND:
		return OnCommand(wParam, lParam);

	case WM_NOTIFY:
		return OnNotify(wParam, lParam);

	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			Relayout(LOWORD(lParam), HIWORD(lParam));
		}
		return TRUE;

	case WM_CLOSE:
		delete c_Dialog;
		c_Dialog = nullptr;
		return TRUE;
	}

	return baseResult;
}

INT_PTR DialogAbout::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	// FIXME: Temporary hack.
	short buttonWidth = (short)GetRainmeter().GetLanguageButtonWidth();

	const Control s_Controls[] =
	{
		Control::Icon(Id_AppIcon, 0,
			356, 12, 64, 64,
			WS_VISIBLE, 0),

		Control::Label(Id_VersionLabel, 0,
			10, 8, 300, 11,
			WS_VISIBLE, 0,
			Control::ANCHOR_TOP_LEFT | Control::BOLD_FONT),
		Control::LinkLabel(Id_BuildLink, 0,
			10, 23, 300, 13,
			WS_VISIBLE, 0),
		Control::LinkLabel(Id_HomeLink, IDS_GetLatestVersion,
			10, 36, 300, 13,
			WS_VISIBLE, 0),
		Control::LinkLabel(Id_LicenseLink, IDS_CopyrightNotice,
			10, 49, 300, 13,
			WS_VISIBLE | LWS_NOPREFIX, 0),

		Control::Label(Id_WinVerLabel, 0,
			10, 72, 360, 13,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),
		Control::LinkLabel(Id_PathLink, 0,
			10, 85, 360, 13,
			WS_VISIBLE | LWS_NOPREFIX, 0),
		Control::LinkLabel(Id_SkinPathLink, 0,
			10, 98, 360, 13,
			WS_VISIBLE | LWS_NOPREFIX, 0),
		Control::LinkLabel(Id_SettingsPathLink, 0,
			10, 111, 360, 13,
			WS_VISIBLE | LWS_NOPREFIX, 0),
		Control::LinkLabel(Id_IniFileLink, 0,
			10, 124, 360, 13,
			WS_VISIBLE | LWS_NOPREFIX, 0),
		Control::Button(Id_CopyButton, IDS_CopyToClipboard,
			10, 148, buttonWidth + 15, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		Control::Button(Id_CloseButton, IDS_Close,
			356, 148, 50, 14,
			WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0,
			Control::ANCHOR_BOTTOM_RIGHT)
	};

	CreateControls(s_Controls, _countof(s_Controls), GetString);
	Initialize();

	HICON hIcon = GetIcon(IDI_RAINMETER, true);
	SendMessage(m_Window, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	SendMessage(m_Window, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

	HWND item = GetControl(Id_CloseButton);
	SendMessage(m_Window, WM_NEXTDLGCTL, (WPARAM)item, TRUE);

	return TRUE;
}

INT_PTR DialogAbout::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case Id_CopyButton:
		{
			WCHAR lang[LOCALE_NAME_MAX_LENGTH];
			LCID lcid = GetRainmeter().GetResourceLCID();
			GetLocaleInfo(lcid, LOCALE_SENGLISHLANGUAGENAME, lang, _countof(lang));

			WCHAR tmpSz[MAX_PATH];
			int len = _snwprintf_s(
				tmpSz,
				_TRUNCATE,
				L"Rainmeter %s.%i (%s)\nLanguage: %s (%lu)\nBuild time: %s\nBuild commit: %s\n",
				APPVERSION,
				revision_number,
				APPBITS,
				lang,
				lcid,
				GetRainmeter().GetBuildTime().c_str(),
				GetRainmeter().GetBuildHash().c_str());

			std::wstring text(tmpSz, len);

			_snwprintf_s(
				tmpSz,
				_TRUNCATE,
				L"OS: %s - %s (%hu)\n",
				GetPlatform().GetFriendlyName().c_str(),
				GetPlatform().GetUserLanguage().c_str(),
				GetUserDefaultUILanguage());

			text += tmpSz;
			text += L"Path: ";
			text += GetRainmeter().GetPath();
			text += L"\nSkinPath: ";
			text += GetRainmeter().GetSkinPath();
			text += L"\nSettingsPath: ";
			text += GetRainmeter().GetSettingsPath();
			text += L"\nIniFile: ";
			text += GetRainmeter().GetIniFile();
			System::SetClipboardText(text);
		}
		return TRUE;

	case Id_CloseButton:
		PostMessage(m_Window, WM_CLOSE, 0, 0);
		return TRUE;
	}

	return FALSE;
}

INT_PTR DialogAbout::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR nm = (LPNMHDR)lParam;
	switch (nm->code)
	{
	case NM_CLICK:
		if (nm->idFrom == Id_BuildLink)
		{
			std::wstring hashLink = L"https://github.com/rainmeter/rainmeter/commit/" + GetRainmeter().GetBuildHash();
			CommandHandler::RunFile(hashLink.c_str());
		}
		else if (nm->idFrom == Id_HomeLink)
		{
			CommandHandler::RunFile(L"https://www.rainmeter.net");
		}
		else if (nm->idFrom == Id_LicenseLink)
		{
			CommandHandler::RunFile(L"https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html");
		}
		else if (nm->idFrom == Id_PathLink)
		{
			CommandHandler::RunFile(GetRainmeter().GetPath().c_str());
		}
		else if (nm->idFrom == Id_SkinPathLink)
		{
			CommandHandler::RunFile(GetRainmeter().GetSkinPath().c_str());
		}
		else if (nm->idFrom == Id_SettingsPathLink)
		{
			CommandHandler::RunFile(GetRainmeter().GetSettingsPath().c_str());
		}
		else if (nm->idFrom == Id_IniFileLink)
		{
			CommandHandler::RunFile(GetRainmeter().GetSkinEditor().c_str(), GetRainmeter().GetIniFile().c_str());
		}
		return TRUE;
	}

	return FALSE;
}

void DialogAbout::Initialize()
{
	// Control::LinkLabel() controls have no "ellipsis" styles, so trim any links if necessary.
	auto trimLink = [](WCHAR* text) -> WCHAR*
	{
		if (wcsnlen_s(text, MAX_PATH) > 100)  // May need to be adjusted if the dialog's size changes.
		{
			text[100] = L'\0';
			wcscat(text, L"...</a>");
		}
		return text;
	};

	HWND item = GetControl(Id_AppIcon);
	HICON icon = GetIconBySize(IDI_RAINMETER, m_ControlTemplate.ScaleDialogUnits(64));
	Static_SetIcon(item, icon);

	WCHAR tmpSz[MAX_PATH];
	_snwprintf_s(tmpSz, _TRUNCATE, L"Rainmeter %s.%i (%s)",
		APPVERSION, revision_number, APPBITS);
	item = GetControl(Id_VersionLabel);
	SetWindowText(item, tmpSz);

	_snwprintf_s(tmpSz, _TRUNCATE, L"Build time: %s (<a>%s</a>)", GetRainmeter().GetBuildTime().c_str(), GetRainmeter().GetBuildHash().c_str());
	item = GetControl(Id_BuildLink);
	SetWindowText(item, tmpSz);

	_snwprintf_s(tmpSz, _TRUNCATE, L"OS: %s - %s (%hu)",
		GetPlatform().GetFriendlyName().c_str(),
		GetPlatform().GetUserLanguage().c_str(),
		GetUserDefaultUILanguage());
	item = GetControl(Id_WinVerLabel);
	SetWindowText(item, tmpSz);

	_snwprintf_s(tmpSz, _TRUNCATE, L"Path: <a>%s</a>", GetRainmeter().GetPath().c_str());
	item = GetControl(Id_PathLink);
	SetWindowText(item, trimLink(tmpSz));

	_snwprintf_s(tmpSz, _TRUNCATE, L"SkinPath: <a>%s</a>", GetRainmeter().GetSkinPath().c_str());
	item = GetControl(Id_SkinPathLink);
	SetWindowText(item, trimLink(tmpSz));

	_snwprintf_s(tmpSz, _TRUNCATE, L"SettingsPath: <a>%s</a>", GetRainmeter().GetSettingsPath().c_str());
	item = GetControl(Id_SettingsPathLink);
	SetWindowText(item, trimLink(tmpSz));

	_snwprintf_s(tmpSz, _TRUNCATE, L"IniFile: <a>%s</a>", GetRainmeter().GetIniFile().c_str());
	item = GetControl(Id_IniFileLink);
	SetWindowText(item, trimLink(tmpSz));
}

void DialogAbout::Relayout(int w, int h)
{
	Dialog::Relayout();

	HWND item = GetControl(Id_AppIcon);
	HICON icon = GetIconBySize(IDI_RAINMETER, m_ControlTemplate.ScaleDialogUnits(96));
	Static_SetIcon(item, icon);
}
