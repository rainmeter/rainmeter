/*
  Copyright (C) 2013 Birunthan Mohanathas

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
#include "DialogInstall.h"
#include "Resource.h"
#include "../Common/ControlTemplate.h"

CDialogInstall* CDialogInstall::c_Dialog = nullptr;

HICON GetIcon(UINT id, bool large)
{
	typedef HRESULT (WINAPI * FPLOADICONMETRIC)(HINSTANCE hinst, PCWSTR pszName, int lims, HICON* phico);

	HINSTANCE hExe = GetModuleHandle(nullptr);
	HINSTANCE hComctl = GetModuleHandle(L"Comctl32");
	if (hComctl)
	{
		// Try LoadIconMetric for better quality with high DPI
		FPLOADICONMETRIC loadIconMetric = (FPLOADICONMETRIC)GetProcAddress(hComctl, "LoadIconMetric");
		if (loadIconMetric)
		{
			HICON icon;
			HRESULT hr = loadIconMetric(hExe, MAKEINTRESOURCE(id), large ? LIM_LARGE : LIM_SMALL, &icon);
			if (SUCCEEDED(hr))
			{
				return icon;
			}
		}
	}

	return (HICON)LoadImage(
		hExe,
		MAKEINTRESOURCE(id),
		IMAGE_ICON,
		GetSystemMetrics(large ? SM_CXICON : SM_CXSMICON),
		GetSystemMetrics(large ? SM_CYICON : SM_CYSMICON),
		LR_SHARED);
}

WCHAR* GetString(UINT id)
{
	switch (id)
	{
	case 1: return L"Install";
	case 2: return L"Rainmeter 3.0.2 beta setup";
	case 3: return L"Click 'Install' to proceed. Rainmeter will launch after setup.";
	case 5: return L"Close";
	case 6: return L"Language:";
	case 7: return L"Destination folder:";
	case 9: return L"...";
	case 10: return L"Launch Rainmeter automatically on login";
	case 13: return L"Cancel";
	case 14: return L"C:\\Program Files\\Rainmeter";
	case 15: return L"Standard install (recommended)";
	case 16: return L"Portable install";
	case 17: return L"Installation type:";
	}
	return L"";
}

CDialogInstall::CDialogInstall() : Dialog()
{
}

CDialogInstall::~CDialogInstall()
{
}

CDialogInstall* CDialogInstall::Create()
{
	c_Dialog = new CDialogInstall();

	c_Dialog->ShowDialogWindow(
		L"Installer",
		0, 0, 350, 210,
		DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU,
		WS_EX_APPWINDOW | WS_EX_CONTROLPARENT,
		nullptr,
		false);

	return c_Dialog;
}

INT_PTR CDialogInstall::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		return OnInitDialog(wParam, lParam);

	case WM_ACTIVATE:
		return OnActivate(wParam, lParam);

	case WM_COMMAND:
		return OnCommand(wParam, lParam);

	case WM_NOTIFY:
		return OnNotify(wParam, lParam);

	case WM_CLOSE:
		{
			delete this;
		}
		return TRUE;
	}

	return FALSE;
}

INT_PTR CDialogInstall::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	static const ControlTemplate::Control s_Controls[] =
	{
		CT_ICON(Id_HeaderIcon, 0,
			8, 10, 24, 24,
			WS_VISIBLE, 0),

		CT_LABEL(Id_HeaderTitleLabel, 2,
			40, 6, 250, 14,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),

		CT_LABEL(-1, 3,
			40, 20, 250, 9,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),

			
		CT_BUTTON(Id_InstallButton, 1,
			199, 191, 70, 14,
			WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0),

		CT_BUTTON(Id_CancelButton, 13,
			274, 191, 70, 14,
			WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0),

		CT_TAB(Id_Tab, 0,
			-2, 38, 400, 148,
			WS_VISIBLE | WS_TABSTOP | TCS_FIXEDWIDTH, 0)  // Last for correct tab order.
	};

	CreateControls(s_Controls, _countof(s_Controls), m_Font, GetString);
	
	m_TabContents.Create(m_Window);
	m_TabContents.Activate();

	SendMessage(m_Window, WM_SETICON, ICON_SMALL, (LPARAM)GetIcon(IDI_APPICON, false));

	HWND item = GetControl(Id_HeaderIcon);
	Static_SetIcon(item, GetIcon(IDI_APPICON, true));
	
	item = GetControl(Id_HeaderTitleLabel);
	SendMessage(item, WM_SETFONT, (WPARAM)m_FontBold, 0);

	item = GetControl(Id_InstallButton);
	SendMessage(m_Window, WM_NEXTDLGCTL, (WPARAM)item, TRUE);
	
	return TRUE;
}

INT_PTR CDialogInstall::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case Id_CancelButton:
		PostMessage(m_Window, WM_CLOSE, 0, 0);
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

INT_PTR CDialogInstall::OnNotify(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

/*
** Constructor.
**
*/
CDialogInstall::TabContents::TabContents() : Tab()
{
}

void CDialogInstall::TabContents::Create(HWND owner)
{
	Tab::CreateTabWindow(10, 50, 380, 135, owner);

	static const ControlTemplate::Control s_Controls[] =
	{
		CT_LABEL(-1, 6,
			0, 3, 107, 9,
			WS_VISIBLE, 0),

		CT_COMBOBOX(Id_LanguageComboBox, 0,
			107, 0, 222, 14,
			WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL, 0),

		CT_LABEL(-1, 17,
			0, 21, 107, 9,
			WS_VISIBLE, 0),

		CT_COMBOBOX(Id_InstallationTypeComboBox, 0,
			107, 18, 222, 14,
			WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 0),

		CT_LABEL(-1, 7,
			0, 43, 107, 9,
			WS_VISIBLE, 0),

		CT_EDIT(Id_DestinationEdit, 14,
			107, 40, 192, 14,
			WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_READONLY, WS_EX_CLIENTEDGE),

		CT_BUTTON(Id_DestinationBrowseButton, 9,
			303, 40, 25, 14,
			WS_VISIBLE | WS_TABSTOP, 0),

		CT_CHECKBOX(Id_LaunchOnLoginCheckBox, 10,
			0, 69, 250, 9,
			WS_VISIBLE | WS_TABSTOP, 0),
	};

	CreateControls(s_Controls, _countof(s_Controls), c_Dialog->m_Font, GetString);
	
	HWND item = GetControl(Id_LanguageComboBox);
	ComboBox_AddString(item, L"English - English (United States)");
	ComboBox_SetCurSel(item, 0);

	item = GetControl(Id_InstallationTypeComboBox);
	ComboBox_AddString(item, L"Standard 64-bit installation (reccomended)");
	ComboBox_AddString(item, L"Standard 32-bit installation");
	ComboBox_AddString(item, L"Portable 64-bit installation");
	ComboBox_AddString(item, L"Portable 32-bit installation");
	ComboBox_SetCurSel(item, 0);
}

void CDialogInstall::TabContents::Initialize()
{
}

INT_PTR CDialogInstall::TabContents::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return OnCommand(wParam, lParam);
	}

	return FALSE;
}

INT_PTR CDialogInstall::TabContents::OnCommand(WPARAM wParam, LPARAM lParam)
{
	return 0;
}
