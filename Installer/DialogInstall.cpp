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
#include "Install.h"
#include "Resource.h"
#include "Util.h"
#include "../Common/ControlTemplate.h"
#include "../Common/Platform.h"

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

CDialogInstall::CDialogInstall() : Dialog(),
	m_InstallProcess(),
	m_InstallProcessWaitThread()
{
}

CDialogInstall::~CDialogInstall()
{
}

CDialogInstall* CDialogInstall::Create()
{
	c_Dialog = new CDialogInstall();

	c_Dialog->ShowDialogWindow(
		L"Rainmeter Setup",
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
			10, 10, 24, 24,
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
			-2, 36, 400, 150,
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
	if (Platform::IsAtLeastWinVista() && !Util::IsProcessUserAdmin())
	{
		Button_SetElevationRequiredState(item, TRUE);
	}

	return TRUE;
}

INT_PTR CDialogInstall::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case Id_CancelButton:
		PostMessage(m_Window, WM_CLOSE, 0, 0);
		break;

	case Id_InstallButton:
		LaunchInstallProcess();
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

void CDialogInstall::LaunchInstallProcess()
{
	const bool isProcsesUserAdmin = Util::IsProcessUserAdmin();
	if (!isProcsesUserAdmin && (Platform::IsAtLeastWinVista() && !Util::CanProcessUserElevate()))
	{
		MessageBox(
			m_Window,
			L"Adminstrative privileges are required to install Rainmeter.\n\nClick OK to close setup.",
			L"Rainmeter Setup", MB_OK | MB_ICONERROR);
		PostMessage(m_Window, WM_CLOSE, 0, 0);
		return;
	}

	m_InstallProcessWaitThread = CreateThread(
		nullptr, 0, ElevatedProcessWaitThreadProc, nullptr, CREATE_SUSPENDED, nullptr);
	if (!m_InstallProcessWaitThread)
	{
		// TODO.
	}

	WCHAR exePath[MAX_PATH];
	GetModuleFileName(nullptr, exePath, _countof(exePath));

	HWND item = m_TabContents.GetControl(TabContents::Id_LanguageComboBox); 
	const LCID lcid = (LCID)ComboBox_GetItemData(item, ComboBox_GetCurSel(item));

	item = m_TabContents.GetControl(TabContents::Id_InstallationTypeComboBox); 
	const LPARAM typeData = ComboBox_GetItemData(item, ComboBox_GetCurSel(item));

	WCHAR targetPath[MAX_PATH];
	item = m_TabContents.GetControl(TabContents::Id_DestinationEdit);
	Edit_GetText(item, targetPath, _countof(targetPath));

	item = m_TabContents.GetControl(TabContents::Id_LaunchOnLoginCheckBox);
	const int launchOnLogin = Button_GetCheck(item) == BST_CHECKED ? 1 : 0;

	WCHAR params[512];
	wsprintf(
		params, L"OPT:%s|%ld|%hd|%hd|%d",
		targetPath, lcid, LOWORD(typeData), HIWORD(typeData), launchOnLogin);

	// Launch the installer process and, if needed, request elevation.
	SHELLEXECUTEINFO sei = {sizeof(sei)};
	sei.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
	sei.lpVerb = isProcsesUserAdmin ? L"open" : L"runas";
	sei.lpFile = exePath;
	sei.lpParameters = params;
	sei.hwnd = m_Window;
	sei.nShow = SW_NORMAL;

	if (!ShellExecuteEx(&sei) || !sei.hProcess)
	{
		MessageBox(m_Window,
			L"Adminstrative privileges are required to install Rainmeter.\n\nClick OK to close setup.",
			L"Rainmeter Setup", MB_OK | MB_ICONERROR);
		PostMessage(m_Window, WM_CLOSE, 0, 0);
		return;
	}

	m_InstallProcess = sei.hProcess;
	ResumeThread(m_InstallProcessWaitThread);
}

DWORD WINAPI CDialogInstall::ElevatedProcessWaitThreadProc(void* param)
{
	WaitForSingleObject(c_Dialog->m_InstallProcess, INFINITE);

	CloseHandle(c_Dialog->m_InstallProcess);
	c_Dialog->m_InstallProcess = nullptr;

	CloseHandle(c_Dialog->m_InstallProcessWaitThread);
	c_Dialog->m_InstallProcessWaitThread = nullptr;

	PostMessage(c_Dialog->m_Window, WM_CLOSE, 0, 0);

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
	ComboBox_SetItemData(item, 0, MAKELPARAM(InstallType::Standard, InstallArch::X64));
	ComboBox_AddString(item, L"Standard 32-bit installation");
	ComboBox_SetItemData(item, 1, MAKELPARAM(InstallType::Standard, InstallArch::X32));
	ComboBox_AddString(item, L"Portable 64-bit installation");
	ComboBox_SetItemData(item, 2, MAKELPARAM(InstallType::Portable, InstallArch::X64));
	ComboBox_AddString(item, L"Portable 32-bit installation");
	ComboBox_SetItemData(item, 3, MAKELPARAM(InstallType::Portable, InstallArch::X32));
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
	switch (LOWORD(wParam))
	{
	case Id_DestinationBrowseButton:
		{
			WCHAR buffer[MAX_PATH];
			BROWSEINFO bi = {0};
			bi.hwndOwner = c_Dialog->GetWindow();
			bi.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS;

			PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&bi);
			if (pidl && SHGetPathFromIDList(pidl, buffer))
			{
				HWND item = GetControl(Id_DestinationEdit);
				Static_SetText(item, buffer);
				CoTaskMemFree(pidl);
			}
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}
