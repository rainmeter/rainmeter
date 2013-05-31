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
#include "../Common/ControlTemplate.h"

CDialogInstall::CDialogInstall() : CDialog()
{
}

CDialogInstall::~CDialogInstall()
{
}

CDialogInstall* CDialogInstall::Create()
{
	auto dialog = new CDialogInstall();

	dialog->ShowDialogWindow(
		L"Installer",
		0, 0, 350, 210,
		DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
		WS_EX_APPWINDOW | WS_EX_CONTROLPARENT,
		nullptr,
		false);

	return dialog;
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

WCHAR* GetString(UINT id) { return L"test"; }

INT_PTR CDialogInstall::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	static const ControlTemplate::Control s_Controls[] =
	{
		CT_BUTTON(Id_CloseButton, 0,
			294, 191, 50, 14,
			WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0)
	};

	CreateControls(s_Controls, _countof(s_Controls), m_Font, GetString);

//	HICON hIcon = GetIcon(IDI_APPLICATION);
//	SendMessage(m_Window, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	HWND item = GetControl(Id_CloseButton);
	SendMessage(m_Window, WM_NEXTDLGCTL, (WPARAM)item, TRUE);

	return TRUE;
}

INT_PTR CDialogInstall::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case Id_CloseButton:
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
