/*
  Copyright (C) 2011 Birunthan Mohanathas

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
#include "Rainmeter.h"
#include "System.h"
#include "MeterWindow.h"
#include "TrayWindow.h"
#include "Measure.h"
#include "resource.h"
#include "DialogManage.h"
#include "DialogAbout.h"
#include "../Version.h"
#include <Commdlg.h>

extern CRainmeter* Rainmeter;

WINDOWPLACEMENT CDialogManage::c_WindowPlacement = {0};
CDialogManage* CDialogManage::c_Dialog = NULL;

/*
** Constructor.
**
*/
CDialogManage::CDialogManage() : CDialog()
{
}

/*
** Destructor.
**
*/
CDialogManage::~CDialogManage()
{
}

/*
** Opens the Manage dialog by tab name.
**
*/
void CDialogManage::Open(const WCHAR* name)
{
	int tab = 0;

	if (name)
	{
		if (_wcsicmp(name, L"Layouts") == 0 ||
			_wcsicmp(name, L"Themes") == 0)  // For backwards compatibility.
		{
			tab = 1;
		}
		else if (_wcsicmp(name, L"Settings") == 0)
		{
			tab = 2;
		}
	}

	Open(tab);
}

/*
** Opens the Manage dialog.
**
*/
void CDialogManage::Open(int tab)
{
	if (!c_Dialog)
	{
		c_Dialog = new CDialogManage();
	}

	c_Dialog->ShowDialogWindow(
		GetString(ID_STR_MANAGERAINMETER),
		0, 0, 500, 322,
		DS_CENTER | WS_POPUP | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
		WS_EX_APPWINDOW | WS_EX_CONTROLPARENT | ((*GetString(ID_STR_ISRTL) == L'1') ? WS_EX_LAYOUTRTL : 0),
		Rainmeter->GetWindow());

	// Fake WM_NOTIFY to change tab
	NMHDR nm;
	nm.code = TCN_SELCHANGE;
	nm.idFrom = Id_Tab;
	nm.hwndFrom = c_Dialog->GetControl(Id_Tab);
	TabCtrl_SetCurSel(nm.hwndFrom, tab);
	c_Dialog->OnNotify(0, (LPARAM)&nm);
}

/*
** Opens the Manage dialog Skins tab with skin selected.
**
*/
void CDialogManage::OpenSkin(CMeterWindow* meterWindow)
{
	Open();

	if (c_Dialog)
	{
		std::wstring name = meterWindow->GetFolderPath() + L'\\';
		name += meterWindow->GetFileName();

		HWND item = c_Dialog->m_TabSkins.GetControl(CTabSkins::Id_SkinsTreeView);
		c_Dialog->m_TabSkins.SelectTreeItem(item, TreeView_GetRoot(item), name.c_str());
	}
}

/*
** Updates Skins tab.
**
*/
void CDialogManage::UpdateSkins(CMeterWindow* meterWindow, bool deleted)
{
	if (c_Dialog && c_Dialog->m_TabSkins.IsInitialized())
	{
		c_Dialog->m_TabSkins.Update(meterWindow, deleted);
	}
}

CDialog::CTab& CDialogManage::GetActiveTab()
{
	int sel = TabCtrl_GetCurSel(GetControl(Id_Tab));
	if (sel == 0)
	{
		return m_TabSkins;
	}
	else if (sel == 1)
	{
		return m_TabLayouts;
	}
	else // if (sel == 2)
	{
		return m_TabSettings;
	}
}

INT_PTR CDialogManage::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			GetWindowPlacement(m_Window, &c_WindowPlacement);
			if (c_WindowPlacement.showCmd == SW_SHOWMINIMIZED)
			{
				c_WindowPlacement.showCmd = SW_SHOWNORMAL;
			}

			delete c_Dialog;
			c_Dialog = NULL;
		}
		return TRUE;
	}

	return FALSE;
}

INT_PTR CDialogManage::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	// FIXME: Temporary hack.
	short buttonWidth = (short)_wtoi(GetString(ID_STR_NUM_BUTTONWIDTH));

	const ControlTemplate::Control s_Controls[] =
	{
		CT_BUTTON(Id_RefreshAllButton, ID_STR_REFRESHALL,
			5, 303, buttonWidth, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_BUTTON(Id_EditSettingsButton, ID_STR_EDITSETTINGS,
			buttonWidth + 9, 303, buttonWidth, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_BUTTON(Id_OpenLogButton, ID_STR_OPENLOG,
			buttonWidth + buttonWidth + 13, 303, buttonWidth, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_BUTTON(Id_HelpButton, ID_STR_HELP,
			389, 303, 50, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_BUTTON(Id_CloseButton, ID_STR_CLOSE,
			444, 303, 50, 14,
			WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0),
		CT_TAB(Id_Tab, 0,
			6, 6, 488, 293,
			WS_VISIBLE | WS_TABSTOP | TCS_FIXEDWIDTH, 0)  // Last for correct tab order.
	};

	CreateControls(s_Controls, _countof(s_Controls), m_Font, GetString);

	HWND item = GetControl(Id_Tab);
	m_TabSkins.Create(m_Window);
	m_TabLayouts.Create(m_Window);
	m_TabSettings.Create(m_Window);

	TCITEM tci = {0};
	tci.mask = TCIF_TEXT;
	tci.pszText = GetString(ID_STR_SKINS);
	TabCtrl_InsertItem(item, 0, &tci);
	tci.pszText = GetString(ID_STR_THEMES);
	TabCtrl_InsertItem(item, 1, &tci);
	tci.pszText = GetString(ID_STR_SETTINGS);
	TabCtrl_InsertItem(item, 2, &tci);

	HICON hIcon = GetIcon(IDI_RAINMETER);
	SendMessage(m_Window, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	item = GetControl(Id_CloseButton);
	SendMessage(m_Window, WM_NEXTDLGCTL, (WPARAM)item, TRUE);

	item = m_TabSkins.GetControl(CTabSkins::Id_FileLabel);
	SendMessage(item, WM_SETFONT, (WPARAM)m_FontBold, 0);

	if (Platform::IsAtLeastWinVista())
	{
		// Use arrows instead of plus/minus in the tree for Vista+
		item = m_TabSkins.GetControl(CTabSkins::Id_SkinsTreeView);
		SetWindowTheme(item, L"explorer", NULL);
	}

	if (c_WindowPlacement.length == 0)
	{
		c_WindowPlacement.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(m_Window, &c_WindowPlacement);
	}

	SetWindowPlacement(m_Window, &c_WindowPlacement);

	return FALSE;
}

INT_PTR CDialogManage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case Id_RefreshAllButton:
		Rainmeter->RefreshAll();
		break;

	case Id_EditSettingsButton:
		Rainmeter->EditSettings();
		break;

	case Id_OpenLogButton:
		CDialogAbout::Open();
		break;

	case Id_CloseButton:
		HandleMessage(WM_CLOSE, 0, 0);
		break;

	case Id_HelpButton:
		{
			std::wstring url = L"http://docs.rainmeter.net/manual/user-interface/manage#";

			CTab& tab = GetActiveTab();
			if (&tab == &m_TabSkins)
			{
				url += L"Skins";
			}
			else if (&tab == &m_TabLayouts)
			{
				url  += L"Layouts";
			}
			else // if (&tab == &m_TabSettings)
			{
				url += L"Settings";
			}

			url += L"Tab";
			ShellExecute(m_Window, L"open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

INT_PTR CDialogManage::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR nm = (LPNMHDR)lParam;
	switch (nm->idFrom)
	{
	case Id_Tab:
		if (nm->code == TCN_SELCHANGE)
		{
			// Disable all tab windows first
			EnableWindow(m_TabSkins.GetWindow(), FALSE);
			EnableWindow(m_TabLayouts.GetWindow(), FALSE);
			EnableWindow(m_TabSettings.GetWindow(), FALSE);

			GetActiveTab().Activate();
		}
		break;

	default:
		return 1;
	}

	return 0;
}

// -----------------------------------------------------------------------------------------------
//
//                                Skins tab
//
// -----------------------------------------------------------------------------------------------

/*
** Constructor.
**
*/
CDialogManage::CTabSkins::CTabSkins() : CTab(),
	m_SkinWindow(),
	m_HandleCommands(false),
	m_IgnoreUpdate(false)
{
}

void CDialogManage::CTabSkins::Create(HWND owner)
{
	CTab::CreateTabWindow(15, 30, 470, 260, owner);

	// FIXME: Temporary hack.
	short labelWidth = (short)_wtoi(GetString(ID_STR_NUM_LABELWIDTH));

	const ControlTemplate::Control s_Controls[] =
	{
		CT_BUTTON(Id_ActiveSkinsButton, ID_STR_ACTIVESKINS,
			0, 0, 146, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_TREEVIEW(Id_SkinsTreeView, 0,
			0, 18, 145, 221,
			WS_VISIBLE | WS_TABSTOP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS | WS_VSCROLL, WS_EX_CLIENTEDGE),
		CT_BUTTON(Id_CreateSkinPackageButton, ID_STR_CREATERMSKINPACKAGE,
			0, 244, 146, 14,
			WS_VISIBLE | WS_TABSTOP, 0),

		CT_LABEL(Id_FileLabel, ID_STR_ELLIPSIS,
			165, 0, 130, 14,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),
		CT_LABEL(Id_ConfigLabel, 0,
			165, 15, 130, 9,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),
		CT_BUTTON(Id_LoadButton, ID_STR_LOAD,
			310, 0, 50, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_BUTTON(Id_RefreshButton, ID_STR_REFRESH,
			364, 0, 50, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_BUTTON(Id_EditButton, ID_STR_EDIT,
			418, 0, 50, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),

		CT_LABEL(-1, ID_STR_AUTHORSC,
			165, 30, 80, 9,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),
		CT_LABEL(Id_AuthorLabel, 0,
			230, 30, 245, 9,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),
		CT_LABEL(-1, ID_STR_VERSIONSC,
			165, 43, 80, 9,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),
		CT_LABEL(Id_VersionLabel, 0,
			230, 43, 245, 9,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),
		CT_LABEL(-1, ID_STR_LICENSESC,
			165, 56, 80, 9,
			WS_VISIBLE | WS_TABSTOP | SS_NOPREFIX, 0),
		CT_LABEL(Id_LicenseLabel, 0,
			230, 56, 245, 9,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),
		CT_LABEL(-1, ID_STR_INFORMATIONSC,
			165, 69, 80, 9,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),
		CT_EDIT(Id_DescriptionLabel, 0,
			228, 69, 238, 64,
			WS_VISIBLE | ES_MULTILINE | ES_READONLY, 0),
		CT_LINKLABEL(Id_AddMetadataLink, ID_STR_ADDMETADATA,
			165, 142, 150, 9,
			0, 0),

		CT_LINEH(-1, ID_STR_COORDINATESSC,
			165, 156, 304, 1,
			WS_VISIBLE, 0),

		CT_LABEL(-1, ID_STR_COORDINATESSC,
			165, 169, labelWidth, 9,
			WS_VISIBLE, 0),
		CT_EDIT(Id_XPositionEdit, 0,
			165 + labelWidth, 166, 38, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, WS_EX_CLIENTEDGE),
		CT_EDIT(Id_YPositionEdit, 0,
			165 + labelWidth + 42, 166, 38, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, WS_EX_CLIENTEDGE),
		CT_LABEL(-1, ID_STR_POSITIONSC,
			165, 190, labelWidth, 9,
			WS_VISIBLE, 0),
		CT_COMBOBOX(Id_ZPositionDropDownList, 0,
			165 + labelWidth, 187, 80, 14,
			WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL | WS_DISABLED, 0),
		CT_LABEL(-1, ID_STR_LOADORDERSC,
			165, 208, labelWidth, 9,
			WS_VISIBLE, 0),
		CT_EDIT(Id_LoadOrderEdit, 0,
			165 + labelWidth, 205, 80, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, WS_EX_CLIENTEDGE),
		CT_LABEL(-1, ID_STR_TRANSPARENCYSC,
			165, 229, labelWidth, 9,
			WS_VISIBLE, 0),
		CT_COMBOBOX(Id_TransparencyDropDownList, 0,
			165 + labelWidth, 226, 80, 14,
			WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL | WS_DISABLED, 0),
		CT_LABEL(-1, ID_STR_ONHOVERSC,
			165, 247, labelWidth, 9,
			WS_VISIBLE, 0),
		CT_COMBOBOX(Id_OnHoverDropDownList, 0,
			165 + labelWidth, 244, 80, 14,
			WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL | WS_DISABLED, 0),

		CT_BUTTON(Id_DisplayMonitorButton, ID_STR_DISPLAYMONITOR,
			350, 166, 118, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_CHECKBOX(Id_DraggableCheckBox, ID_STR_DRAGGABLE,
			350, 190, 118, 9,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_CHECKBOX(Id_ClickThroughCheckBox, ID_STR_CLICKTHROUGH,
			350, 203, 118, 9,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_CHECKBOX(Id_KeepOnScreenCheckBox, ID_STR_KEEPONSCREEN,
			350, 216, 118, 9,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_CHECKBOX(Id_SavePositionCheckBox, ID_STR_SAVEPOSITION,
			350, 229, 118, 9,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_CHECKBOX(Id_SnapToEdgesCheckBox, ID_STR_SNAPTOEDGES,
			350, 242, 118, 9,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0)
	};

	CreateControls(s_Controls, _countof(s_Controls), c_Dialog->m_Font, GetString);
}

void CDialogManage::CTabSkins::Initialize()
{
	BUTTON_SPLITINFO bsi;
	bsi.mask = BCSIF_SIZE;
	bsi.size.cx = 20;
	bsi.size.cy = 14;

	HWND item = GetControl(Id_ActiveSkinsButton);
	CDialog::SetMenuButton(item);

	item = GetControl(Id_DisplayMonitorButton);
	CDialog::SetMenuButton(item);

	// Load folder/.ini icons from shell32
	HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_COLOR32, 2, 10);
	HMODULE hDLL = GetModuleHandle(L"shell32");

	HICON hIcon = (HICON)LoadImage(hDLL, MAKEINTRESOURCE(4), IMAGE_ICON, 16, 16, LR_SHARED);
	ImageList_AddIcon(hImageList, hIcon);
	hIcon = (HICON)LoadImage(hDLL, MAKEINTRESOURCE(151), IMAGE_ICON, 16, 16, LR_SHARED); 
	ImageList_AddIcon(hImageList, hIcon);

	// Apply icons and populate tree
	item = GetControl(Id_SkinsTreeView);
	TreeView_SetImageList(item, hImageList, TVSIL_NORMAL);
	Update(NULL, false);

	// Get rid of the EDITTEXT control border
	item = GetControl(Id_DescriptionLabel);
	SetWindowLongPtr(item, GWL_EXSTYLE, GetWindowLongPtr(item, GWL_EXSTYLE) &~ WS_EX_CLIENTEDGE);
	SetWindowPos(item, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER); 

	item = GetControl(Id_TransparencyDropDownList);
	ComboBox_AddString(item, L"0%");
	ComboBox_AddString(item, L"10%");
	ComboBox_AddString(item, L"20%");
	ComboBox_AddString(item, L"30%");
	ComboBox_AddString(item, L"40%");
	ComboBox_AddString(item, L"50%");
	ComboBox_AddString(item, L"60%");
	ComboBox_AddString(item, L"70%");
	ComboBox_AddString(item, L"80%");
	ComboBox_AddString(item, L"90%");

	item = GetControl(Id_ZPositionDropDownList);
	ComboBox_AddString(item, GetString(ID_STR_ONDESKTOP));
	ComboBox_AddString(item, GetString(ID_STR_BOTTOM));
	ComboBox_AddString(item, GetString(ID_STR_NORMAL));
	ComboBox_AddString(item, GetString(ID_STR_TOPMOST));
	ComboBox_AddString(item, GetString(ID_STR_STAYTOPMOST));

	item = GetControl(Id_OnHoverDropDownList);
	ComboBox_AddString(item, GetString(ID_STR_DONOTHING));
	ComboBox_AddString(item, GetString(ID_STR_HIDE));
	ComboBox_AddString(item, GetString(ID_STR_FADEIN));
	ComboBox_AddString(item, GetString(ID_STR_FADEOUT));

	m_Initialized = true;
	m_HandleCommands = true;
}

/*
** Updates metadata and settings when changed.
**
*/
void CDialogManage::CTabSkins::Update(CMeterWindow* meterWindow, bool deleted)
{
	if (meterWindow)
	{
		if (!deleted && m_IgnoreUpdate)
		{
			// Changed setting from dialog, no need to update
			m_IgnoreUpdate = false;
		}
		else if (m_SkinWindow && m_SkinWindow == meterWindow) 
		{
			// Update from currently open skin
			m_HandleCommands = false;
			if (deleted)
			{
				DisableControls();
				m_SkinWindow = NULL;
			}
			else
			{
				SetControls();
			}
			m_HandleCommands = true;
		}
		else if (wcscmp(meterWindow->GetFolderPath().c_str(), m_SkinFolderPath.c_str()) == 0 &&
				 wcscmp(meterWindow->GetFileName().c_str(), m_SkinFileName.c_str()) == 0)
		{
			ReadSkin();
		}
	}
	else
	{
		// Populate tree
		HWND item = GetControl(Id_SkinsTreeView);
		TreeView_DeleteAllItems(item);

		TVINSERTSTRUCT tvi = {0};
		tvi.hInsertAfter = TVI_LAST;
		tvi.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvi.item.iImage = tvi.item.iSelectedImage = 0;

		if (!Rainmeter->m_SkinFolders.empty())
		{
			PopulateTree(item, tvi);
		}
	}
}

void CDialogManage::CTabSkins::SetControls()
{
	WCHAR buffer[64];

	HWND item = GetControl(Id_EditButton);
	EnableWindow(item, TRUE);

	item = GetControl(Id_LoadButton);
	EnableWindow(item, TRUE);

	if (m_SkinWindow)
	{
		SetWindowText(item, GetString(ID_STR_UNLOAD));

		item = GetControl(Id_RefreshButton);
		EnableWindow(item, TRUE);

		item = GetControl(Id_XPositionEdit);
		EnableWindow(item, TRUE);
		_itow_s(m_SkinWindow->GetX(), buffer, 10);
		SetWindowText(item, buffer);

		item = GetControl(Id_YPositionEdit);
		EnableWindow(item, TRUE);
		_itow_s(m_SkinWindow->GetY(), buffer, 10);
		SetWindowText(item, buffer);

		item = GetControl(Id_DisplayMonitorButton);
		EnableWindow(item, TRUE);

		item = GetControl(Id_DraggableCheckBox);
		if (Rainmeter->GetDisableDragging())
		{
			EnableWindow(item, FALSE);
			Button_SetCheck(item, BST_UNCHECKED);
		}
		else
		{
			EnableWindow(item, TRUE);
			Button_SetCheck(item, m_SkinWindow->GetWindowDraggable());
		}

		item = GetControl(Id_ClickThroughCheckBox);
		EnableWindow(item, TRUE);
		Button_SetCheck(item, m_SkinWindow->GetClickThrough());

		item = GetControl(Id_KeepOnScreenCheckBox);
		EnableWindow(item, TRUE);
		Button_SetCheck(item, m_SkinWindow->GetKeepOnScreen());

		item = GetControl(Id_SavePositionCheckBox);
		EnableWindow(item, TRUE);
		Button_SetCheck(item, m_SkinWindow->GetSavePosition());

		item = GetControl(Id_SnapToEdgesCheckBox);
		EnableWindow(item, TRUE);
		Button_SetCheck(item, m_SkinWindow->GetSnapEdges());

		item = GetControl(Id_TransparencyDropDownList);
		EnableWindow(item, TRUE);
		int value = (int)(10 - m_SkinWindow->GetAlphaValue() / 25.5);
		value = min(9, value);
		value = max(0, value);
		ComboBox_SetCurSel(item, value);

		item = GetControl(Id_ZPositionDropDownList);
		EnableWindow(item, TRUE);
		ComboBox_SetCurSel(item, m_SkinWindow->GetWindowZPosition() + 2);

		item = GetControl(Id_LoadOrderEdit);
		EnableWindow(item, TRUE);
		_itow_s(Rainmeter->GetLoadOrder(m_SkinFolderPath), buffer, 10);
		SetWindowText(item, buffer);

		item = GetControl(Id_OnHoverDropDownList);
		EnableWindow(item, TRUE);
		ComboBox_SetCurSel(item, m_SkinWindow->GetWindowHide());
	}
	else
	{
		SetWindowText(item, GetString(ID_STR_LOAD));
	}
}

void CDialogManage::CTabSkins::DisableControls(bool clear)
{
	HWND item = GetControl(Id_LoadButton);
	SetWindowText(item, GetString(ID_STR_LOAD));

	if (clear)
	{
		EnableWindow(item, FALSE);

		item = GetControl(Id_EditButton);
		EnableWindow(item, FALSE);
		
		item = GetControl(Id_FileLabel);
		SetWindowText(item, GetString(ID_STR_ELLIPSIS));

		item = GetControl(Id_ConfigLabel);
		SetWindowText(item, L"");

		item = GetControl(Id_AuthorLabel);
		SetWindowText(item, L"");

		item = GetControl(Id_VersionLabel);
		SetWindowText(item, L"");

		item = GetControl(Id_LicenseLabel);
		SetWindowText(item, L"");

		item = GetControl(Id_DescriptionLabel);
		SetWindowText(item, L"");
		ShowScrollBar(item, SB_VERT, FALSE);

		item = GetControl(Id_AddMetadataLink);
		ShowWindow(item, SW_HIDE);
	}
	else
	{
		EnableWindow(item, TRUE);
	}

	item = GetControl(Id_RefreshButton);
	EnableWindow(item, FALSE);

	item = GetControl(Id_XPositionEdit);
	SetWindowText(item, L"");
	EnableWindow(item, FALSE);

	item = GetControl(Id_YPositionEdit);
	SetWindowText(item, L"");
	EnableWindow(item, FALSE);

	item = GetControl(Id_DisplayMonitorButton);
	EnableWindow(item, FALSE);

	item = GetControl(Id_DraggableCheckBox);
	EnableWindow(item, FALSE);
	Button_SetCheck(item, BST_UNCHECKED);

	item = GetControl(Id_ClickThroughCheckBox);
	EnableWindow(item, FALSE);
	Button_SetCheck(item, BST_UNCHECKED);

	item = GetControl(Id_KeepOnScreenCheckBox);
	EnableWindow(item, FALSE);
	Button_SetCheck(item, BST_UNCHECKED);

	item = GetControl(Id_SavePositionCheckBox);
	EnableWindow(item, FALSE);
	Button_SetCheck(item, BST_UNCHECKED);

	item = GetControl(Id_SnapToEdgesCheckBox);
	EnableWindow(item, FALSE);
	Button_SetCheck(item, BST_UNCHECKED);

	item = GetControl(Id_TransparencyDropDownList);
	EnableWindow(item, FALSE);
	ComboBox_SetCurSel(item, -1);

	item = GetControl(Id_ZPositionDropDownList);
	EnableWindow(item, FALSE);
	ComboBox_SetCurSel(item, -1);

	item = GetControl(Id_LoadOrderEdit);
	SetWindowText(item, L"");
	EnableWindow(item, FALSE);

	item = GetControl(Id_OnHoverDropDownList);
	EnableWindow(item, FALSE);
	ComboBox_SetCurSel(item, -1);
}

void CDialogManage::CTabSkins::ReadSkin()
{
	HWND item = GetControl(Id_FileLabel);
	SetWindowText(item, m_SkinFileName.c_str());

	PathSetDlgItemPath(m_Window, Id_ConfigLabel, m_SkinFolderPath.c_str());

	item = GetControl(Id_EditButton);
	EnableWindow(item, TRUE);

	std::wstring file = Rainmeter->GetSkinPath() + m_SkinFolderPath;
	file += L'\\';
	file += m_SkinFileName;
	m_SkinWindow = Rainmeter->GetMeterWindowByINI(file);
	if (!m_SkinWindow)
	{
		DisableControls();
	}

	SetControls();

	WCHAR* buffer = new WCHAR[MAX_LINE_LENGTH];
	const WCHAR* fileSz = file.c_str();

	item = GetControl(Id_AuthorLabel);
	if (GetPrivateProfileString(L"Metadata", L"Author", NULL, buffer, MAX_LINE_LENGTH, fileSz) == 0)
	{
		// For backwards compatibility.
		GetPrivateProfileString(L"Rainmeter", L"Author", NULL, buffer, MAX_LINE_LENGTH, fileSz);
	}
	SetWindowText(item, buffer);

	item = GetControl(Id_AddMetadataLink);
	if (GetPrivateProfileSection(L"Metadata", buffer, 8, fileSz) > 0)
	{
		ShowWindow(item, SW_HIDE);

		// Set metadata
		item = GetControl(Id_VersionLabel);
		GetPrivateProfileString(L"Metadata", L"Version", NULL, buffer, MAX_LINE_LENGTH, fileSz);
		SetWindowText(item, buffer);

		item = GetControl(Id_LicenseLabel);
		GetPrivateProfileString(L"Metadata", L"License", NULL, buffer, MAX_LINE_LENGTH, fileSz);
		SetWindowText(item, buffer);

		item = GetControl(Id_DescriptionLabel);
		std::wstring text;
		if (GetPrivateProfileString(L"Metadata", L"Information", NULL, buffer, MAX_LINE_LENGTH, fileSz) > 0)
		{
			text = buffer;
		}
		else
		{
			// For backwards compatibility
			GetPrivateProfileString(L"Metadata", L"Description", NULL, buffer, MAX_LINE_LENGTH, fileSz);
			text = buffer;

			if (GetPrivateProfileString(L"Metadata", L"Instructions", NULL, buffer, MAX_LINE_LENGTH, fileSz) > 0)
			{
				text += L"\r\n\r\n";
				text += buffer;
			}
		}

		// Replace | with newline
		std::wstring::size_type pos;
		while ((pos = text.find_first_of(L'|')) != std::wstring::npos)
		{
			size_t count = (pos + 1 < text.length() && text[pos + 1] == L' ') ? 2 : 1;
			if (text[pos - 1] == L' ')
			{
				--pos;
				count += 1;
			}
			text.replace(pos, count, L"\r\n");
		}

		SetWindowText(item, text.c_str());

		int lines = Edit_GetLineCount(item);
		ShowScrollBar(item, SB_VERT, (BOOL)(lines > 6));
	}
	else
	{
		ShowWindow(item, SW_SHOWNORMAL);

		item = GetControl(Id_VersionLabel);
		SetWindowText(item, L"");

		item = GetControl(Id_LicenseLabel);
		SetWindowText(item, L"");

		item = GetControl(Id_DescriptionLabel);
		SetWindowText(item, L"");
		ShowScrollBar(item, SB_VERT, FALSE);
	}

	delete [] buffer;
}

std::wstring CDialogManage::CTabSkins::GetTreeSelectionPath(HWND tree)
{
	WCHAR buffer[MAX_PATH];

	// Get current selection name
	TVITEM tvi = {0};
	tvi.hItem = TreeView_GetSelection(tree);
	tvi.mask = TVIF_TEXT;
	tvi.pszText = buffer;
	tvi.cchTextMax = MAX_PATH;
	TreeView_GetItem(tree, &tvi);
	
	std::wstring path = buffer;
	while ((tvi.hItem = TreeView_GetParent(tree, tvi.hItem)) != NULL)
	{
		TreeView_GetItem(tree, &tvi);
		path.insert(0, 1, L'\\');
		path.insert(0, buffer);
	}

	return path;
}

/*
** Populates the treeview with folders and skins.
**
*/
int CDialogManage::CTabSkins::PopulateTree(HWND tree, TVINSERTSTRUCT& tvi, int index)
{
	int initialLevel = Rainmeter->m_SkinFolders[index].level;

	const size_t max = Rainmeter->m_SkinFolders.size();
	while (index < max)
	{
		const CRainmeter::SkinFolder& skinFolder = Rainmeter->m_SkinFolders[index];
		if (skinFolder.level != initialLevel)
		{
			return index - 1;
		}

		HTREEITEM oldParent = tvi.hParent;

		// Add folder
		tvi.item.iImage = tvi.item.iSelectedImage = 0;
		tvi.item.pszText = (WCHAR*)skinFolder.name.c_str();
		tvi.hParent = TreeView_InsertItem(tree, &tvi);

		// Add subfolders
		if ((index + 1) < max &&
			Rainmeter->m_SkinFolders[index + 1].level == initialLevel + 1)
		{
			index = PopulateTree(tree, tvi, index + 1);
		}

		// Add files
		tvi.item.iImage = tvi.item.iSelectedImage = 1;
		for (int i = 0, isize = (int)skinFolder.files.size(); i < isize; ++i)
		{
			tvi.item.pszText = (WCHAR*)skinFolder.files[i].c_str();
			TreeView_InsertItem(tree, &tvi);
		}

		tvi.hParent = oldParent;

		++index;
	}

	return index;
}

/*
** Selects an item in the treeview.
**
*/
void CDialogManage::CTabSkins::SelectTreeItem(HWND tree, HTREEITEM item, LPCWSTR name)
{
	WCHAR buffer[MAX_PATH];
	TVITEM tvi = {0};
	tvi.mask = TVIF_TEXT;
	tvi.hItem = item;
	tvi.pszText = buffer;

	const WCHAR* pos = wcschr(name, L'\\');
	if (pos)
	{
		const int folderLen = (int)(pos - name);
		tvi.cchTextMax = folderLen + 1;		// Length of folder name plus 1 for NULL

		// Find and expand the folder
		do
		{
			TreeView_GetItem(tree, &tvi);
			if (wcsncmp(buffer, name, folderLen) == 0)
			{
				if ((item = TreeView_GetChild(tree, tvi.hItem)) != NULL)
				{
					TreeView_Expand(tree, tvi.hItem, TVE_EXPAND);
					++pos;	// Skip the slash
					SelectTreeItem(tree, item, pos);
				}

				break;
			}
		}
		while ((tvi.hItem = TreeView_GetNextSibling(tree, tvi.hItem)) != NULL);
	}
	else
	{
		tvi.cchTextMax = MAX_PATH;

		// Find and select the file
		do
		{
			TreeView_GetItem(tree, &tvi);
			if (wcscmp(buffer, name) == 0)
			{
				TreeView_Select(tree, tvi.hItem, TVGN_CARET);
				break;
			}
		}
		while ((tvi.hItem = TreeView_GetNextSibling(tree, tvi.hItem)) != NULL);
	}
}

INT_PTR CDialogManage::CTabSkins::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return OnCommand(wParam, lParam);

	case WM_NOTIFY:
		return OnNotify(wParam, lParam);
	}

	return FALSE;
}

INT_PTR CDialogManage::CTabSkins::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (!m_HandleCommands)
	{
		// Values are being changed/reset, no need to apply changes.
		return FALSE;
	}

	switch (LOWORD(wParam))
	{
	case Id_ActiveSkinsButton:
		{
			HMENU menu = CreatePopupMenu();

			// Add active skins to menu
			std::map<std::wstring, CMeterWindow*>::const_iterator iter = Rainmeter->GetAllMeterWindows().begin();
			int index = 0;
			for ( ; iter != Rainmeter->GetAllMeterWindows().end(); ++iter)
			{
				std::wstring name = ((*iter).second)->GetFolderPath() + L'\\';
				name += ((*iter).second)->GetFileName();
				InsertMenu(menu, index, MF_BYPOSITION, ID_CONFIG_FIRST + index, name.c_str());
				++index;
			}

			if (index > 0)
			{
				RECT r;
				GetWindowRect((HWND)lParam, &r);

				// Show context menu
				TrackPopupMenu(
					menu,
					TPM_RIGHTBUTTON | TPM_LEFTALIGN,
					(*GetString(ID_STR_ISRTL) == L'1') ? r.right : r.left,
					--r.bottom,
					0,
					m_Window,
					NULL
				);
			}

			DestroyMenu(menu);
		}
		break;

	case Id_CreateSkinPackageButton:
		{
			std::wstring file = Rainmeter->GetPath() + L"SkinInstaller.exe";
			CCommandHandler::RunFile(file.c_str(), L"/Packager");
		}
		break;

	case Id_LoadButton:
		{
			if (!m_SkinWindow)
			{
				// Skin not active, load
				std::pair<int, int> indexes = Rainmeter->GetMeterWindowIndex(m_SkinFolderPath, m_SkinFileName);
				if (indexes.first != -1 && indexes.second != -1)
				{
					m_HandleCommands = false;
					Rainmeter->ActivateSkin(indexes.first, indexes.second);
					m_HandleCommands = true;

					// Fake selection change to update controls
					NMHDR nm;
					nm.code = TVN_SELCHANGED;
					nm.idFrom = Id_SkinsTreeView;
					nm.hwndFrom = GetControl(Id_SkinsTreeView);
					OnNotify(0, (LPARAM)&nm);
				}
			}
			else
			{
				m_HandleCommands = false;
				Rainmeter->DeactivateSkin(m_SkinWindow, -1);
			}
		}
		break;

	case Id_RefreshButton:
		if (m_SkinWindow)
		{
			m_SkinWindow->Refresh(false);
		}
		break;

	case Id_EditButton:
		Rainmeter->EditSkinFile(m_SkinFolderPath, m_SkinFileName);
		break;

	case Id_XPositionEdit:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			WCHAR buffer[32];
			m_IgnoreUpdate = true;
			int x = (GetWindowText((HWND)lParam, buffer, 32) > 0) ? _wtoi(buffer) : 0;
			m_SkinWindow->MoveWindow(x, m_SkinWindow->GetY());

			if (x > m_SkinWindow->GetX())
			{
				_itow_s(m_SkinWindow->GetX(), buffer, 10);
				Edit_SetText((HWND)lParam, buffer);
			}
		}
		break;

	case Id_YPositionEdit:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			WCHAR buffer[32];
			m_IgnoreUpdate = true;
			int y = (GetWindowText((HWND)lParam, buffer, 32) > 0) ? _wtoi(buffer) : 0;
			m_SkinWindow->MoveWindow(m_SkinWindow->GetX(), y);

			if (y > m_SkinWindow->GetY())
			{
				_itow_s(m_SkinWindow->GetY(), buffer, 10);
				Edit_SetText((HWND)lParam, buffer);
			}
		}
		break;

	case Id_LoadOrderEdit:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			if (m_IgnoreUpdate)
			{
				// To avoid infinite loop after setting value below
				m_IgnoreUpdate = false;
			}
			else
			{
				// Convert text to number and set it to get rid of extra chars
				WCHAR buffer[32];
				int len = GetWindowText((HWND)lParam, buffer, 32);
				if ((len == 0) || (len == 1 && buffer[0] == L'-'))
				{
					// Ignore if empty or if - is only char
					break;
				}

				// Get selection
				DWORD sel = Edit_GetSel((HWND)lParam);

				// Reset value (to get rid of invalid chars)
				m_IgnoreUpdate = true;
				int value = _wtoi(buffer);

				_itow_s(value, buffer, 10);
				SetWindowText((HWND)lParam, buffer);

				// Reset selection
				Edit_SetSel((HWND)lParam, LOWORD(sel), HIWORD(sel));

				WritePrivateProfileString(m_SkinFolderPath.c_str(), L"LoadOrder", buffer, Rainmeter->GetIniFile().c_str());
				std::pair<int, int> indexes = Rainmeter->GetMeterWindowIndex(m_SkinWindow);
				if (indexes.first != -1)
				{
					Rainmeter->SetLoadOrder(indexes.first, value);

					std::multimap<int, CMeterWindow*> windows;
					Rainmeter->GetMeterWindowsByLoadOrder(windows);

					CSystem::PrepareHelperWindow();

					// Reorder window z-position to reflect load order
					std::multimap<int, CMeterWindow*>::const_iterator iter = windows.begin();
					for ( ; iter != windows.end(); ++iter)
					{
						CMeterWindow* mw = (*iter).second;
						mw->ChangeZPos(mw->GetWindowZPosition(), true);
					}
				}
			}
		}
		break;

	case Id_DisplayMonitorButton:
		{
			static const MenuTemplate s_Menu[] =
			{
				MENU_ITEM(IDM_SKIN_MONITOR_PRIMARY, ID_STR_USEDEFAULTMONITOR),
				MENU_ITEM(ID_MONITOR_FIRST, ID_STR_VIRTUALSCREEN),
				MENU_SEPARATOR(),
				MENU_SEPARATOR(),
				MENU_ITEM(IDM_SKIN_MONITOR_AUTOSELECT, ID_STR_AUTOSELECTMONITOR)
			};

			HMENU menu = MenuTemplate::CreateMenu(s_Menu, _countof(s_Menu), GetString);
			if (menu)
			{
				Rainmeter->CreateMonitorMenu(menu, m_SkinWindow);

				RECT r;
				GetWindowRect((HWND)lParam, &r);

				// Show context menu
				TrackPopupMenu(
					menu,
					TPM_RIGHTBUTTON | TPM_LEFTALIGN,
					(*GetString(ID_STR_ISRTL) == L'1') ? r.right : r.left,
					--r.bottom,
					0,
					m_Window,
					NULL
				);

				DestroyMenu(menu);
			}
		}
		break;

	case Id_DraggableCheckBox:
		m_IgnoreUpdate = true;
		m_SkinWindow->SetWindowDraggable(!m_SkinWindow->GetWindowDraggable());
		break;

	case Id_ClickThroughCheckBox:
		m_IgnoreUpdate = true;
		m_SkinWindow->SetClickThrough(!m_SkinWindow->GetClickThrough());
		break;

	case Id_KeepOnScreenCheckBox:
		m_IgnoreUpdate = true;
		m_SkinWindow->SetKeepOnScreen(!m_SkinWindow->GetKeepOnScreen());
		break;

	case Id_SavePositionCheckBox:
		m_IgnoreUpdate = true;
		m_SkinWindow->SetSavePosition(!m_SkinWindow->GetSavePosition());
		break;

	case Id_SnapToEdgesCheckBox:
		m_IgnoreUpdate = true;
		m_SkinWindow->SetSnapEdges(!m_SkinWindow->GetSnapEdges());
		break;

	case Id_ZPositionDropDownList:
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			m_IgnoreUpdate = true;
			ZPOSITION zpos = (ZPOSITION)(ComboBox_GetCurSel((HWND)lParam) - 2);
			m_SkinWindow->SetWindowZPosition(zpos);
		}
		break;

	case Id_TransparencyDropDownList:
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			m_IgnoreUpdate = true;
			int sel = ComboBox_GetCurSel((HWND)lParam) + IDM_SKIN_TRANSPARENCY_0;
			SendMessage(m_SkinWindow->GetWindow(), WM_COMMAND, sel, 0);
		}
		break;

	case Id_OnHoverDropDownList:
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			m_IgnoreUpdate = true;
			HIDEMODE hide = (HIDEMODE)ComboBox_GetCurSel((HWND)lParam);
			m_SkinWindow->SetWindowHide(hide);
		}
		break;

	case IDM_MANAGESKINSMENU_EXPAND:
		{
			HWND tree = GetControl(Id_SkinsTreeView);
			HTREEITEM item = TreeView_GetSelection(tree);
			TreeView_Expand(tree, item, TVE_TOGGLE);
		}
		break;

	case IDM_MANAGESKINSMENU_OPENFOLDER:
		{
			HWND tree = GetControl(Id_SkinsTreeView);
			Rainmeter->OpenSkinFolder(GetTreeSelectionPath(tree));
		}
		break;

	default:
		if (wParam >= ID_CONFIG_FIRST && wParam <= ID_CONFIG_LAST)
		{
			std::map<std::wstring, CMeterWindow*>::const_iterator iter = Rainmeter->GetAllMeterWindows().begin();
			int index = (int)wParam - ID_CONFIG_FIRST;
			int i = 0;
			for ( ; iter != Rainmeter->GetAllMeterWindows().end(); ++iter)
			{
				if (i == index)
				{
					std::wstring name = ((*iter).second)->GetFolderPath() + L'\\';
					name += ((*iter).second)->GetFileName();

					HWND item = GetControl(Id_SkinsTreeView);
					SelectTreeItem(item, TreeView_GetRoot(item), name.c_str());
					break;
				}

				++i;
			}
		}
		else if (wParam == IDM_SKIN_MONITOR_AUTOSELECT ||
			wParam == IDM_SKIN_MONITOR_PRIMARY ||
			wParam >= ID_MONITOR_FIRST && wParam <= ID_MONITOR_LAST)
		{
			if (m_SkinWindow)
			{
				SendMessage(m_SkinWindow->GetWindow(), WM_COMMAND, wParam, 0);
			}
			break;
		}

		return 1;
	}

	return 0;
}

INT_PTR CDialogManage::CTabSkins::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR nm = (LPNMHDR)lParam;
	switch (nm->code)
	{
	case NM_CLICK:
		if (nm->idFrom == Id_AddMetadataLink)
		{
			std::wstring file = Rainmeter->GetSkinPath() + m_SkinFolderPath;
			file += L'\\';
			file += m_SkinFileName;
			const WCHAR* str = L"\r\n"  // Hack to add below [Rainmeter].
				L"[Metadata]\r\n"
				L"Name=\r\n"
				L"Author=\r\n"
				L"Information=\r\n"
				L"License=\r\n"
				L"Version";
			WritePrivateProfileString(L"Rainmeter", str, L"", file.c_str());
			SendMessage(m_Window, WM_COMMAND, MAKEWPARAM(Id_EditButton, 0), 0);
			ShowWindow(nm->hwndFrom, SW_HIDE);
		}
		break;

	case NM_DBLCLK:
		if (nm->idFrom == Id_SkinsTreeView && !m_SkinFileName.empty())
		{
			OnCommand(MAKEWPARAM(Id_LoadButton, 0), 0);
		}
		break;

	case NM_RCLICK:
		if (nm->idFrom == Id_SkinsTreeView)
		{
			POINT pt = CSystem::GetCursorPosition();

			TVHITTESTINFO ht;
			ht.pt = pt;
			ScreenToClient(nm->hwndFrom, &ht.pt);

			if (TreeView_HitTest(nm->hwndFrom, &ht) && !(ht.flags & TVHT_NOWHERE))
			{
				TreeView_SelectItem(nm->hwndFrom, ht.hItem);

				TVITEM tvi = {0};
				tvi.hItem = TreeView_GetSelection(nm->hwndFrom);
				tvi.mask = TVIF_STATE;

				if (TreeView_GetItem(nm->hwndFrom, &tvi))
				{
					HMENU menu = NULL;
					MENUITEMINFO mii = {0};
					mii.cbSize = sizeof(MENUITEMINFO);
					mii.fMask = MIIM_STRING;

					if (m_SkinFileName.empty())
					{
						// Folder menu.
						static const MenuTemplate s_Menu[] =
						{
							MENU_ITEM(IDM_MANAGESKINSMENU_EXPAND, ID_STR_EXPAND),
							MENU_ITEM(IDM_MANAGESKINSMENU_OPENFOLDER, ID_STR_OPENFOLDER),
						};

						menu = MenuTemplate::CreateMenu(s_Menu, _countof(s_Menu), GetString);
						SetMenuDefaultItem(menu, IDM_MANAGESKINSMENU_EXPAND, MF_BYCOMMAND);

						if (tvi.state & TVIS_EXPANDED)
						{
							mii.dwTypeData = GetString(ID_STR_COLLAPSE);
							SetMenuItemInfo(menu, IDM_MANAGESKINSMENU_EXPAND, MF_BYCOMMAND, &mii);
						}
					}
					else
					{
						// Skin menu.
						static const MenuTemplate s_Menu[] =
						{
							MENU_ITEM(IDM_MANAGESKINSMENU_LOAD, ID_STR_LOAD),
							MENU_ITEM(IDM_MANAGESKINSMENU_REFRESH, ID_STR_REFRESH),
							MENU_ITEM(IDM_MANAGESKINSMENU_EDIT, ID_STR_EDIT),
						};

						menu = MenuTemplate::CreateMenu(s_Menu, _countof(s_Menu), GetString);
						SetMenuDefaultItem(menu, IDM_MANAGESKINSMENU_LOAD, MF_BYCOMMAND);

						if (m_SkinWindow)
						{
							mii.dwTypeData = GetString(ID_STR_UNLOAD);
							SetMenuItemInfo(menu, IDM_MANAGESKINSMENU_LOAD, MF_BYCOMMAND, &mii);
						}
						else
						{
							EnableMenuItem(menu, IDM_MANAGESKINSMENU_REFRESH, MF_BYCOMMAND | MF_GRAYED);
						}
					}

					// Show context menu
					TrackPopupMenu(
						menu,
						TPM_RIGHTBUTTON | TPM_LEFTALIGN,
						pt.x,
						pt.y,
						0,
						m_Window,
						NULL
					);

					DestroyMenu(menu);
				}
			}
		}
		break;

	case TVN_SELCHANGED:
		if (nm->idFrom == Id_SkinsTreeView)
		{
			m_SkinWindow = NULL;
			m_SkinFileName.clear();
			m_SkinFolderPath.clear();

			// Temporarily disable handling commands
			m_HandleCommands = false;

			WCHAR buffer[MAX_PATH];

			// Get current selection name
			TVITEM tvi = {0};
			tvi.hItem = TreeView_GetSelection(nm->hwndFrom);
			tvi.mask = TVIF_TEXT | TVIF_CHILDREN;
			tvi.pszText = buffer;
			tvi.cchTextMax = MAX_PATH;
			TreeView_GetItem(nm->hwndFrom, &tvi);

			if (tvi.cChildren == 0)
			{
				// Current selection is file
				m_SkinFileName = buffer;
				tvi.mask = TVIF_TEXT;
			
				// Loop through parents to get skin folder
				m_SkinFolderPath.clear();
				while ((tvi.hItem = TreeView_GetParent(nm->hwndFrom, tvi.hItem)) != NULL)
				{
					TreeView_GetItem(nm->hwndFrom, &tvi);
					m_SkinFolderPath.insert(0, 1, L'\\');
					m_SkinFolderPath.insert(0, buffer);
				}

				m_SkinFolderPath.resize(m_SkinFolderPath.length() - 1);  // Get rid of trailing slash

				ReadSkin();
			}
			else
			{
				DisableControls(true);
			}

			m_HandleCommands = true;
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------------
//
//                                Layouts tab
//
// -----------------------------------------------------------------------------------------------

/*
** Constructor.
**
*/
CDialogManage::CTabLayouts::CTabLayouts() : CTab()
{
}

void CDialogManage::CTabLayouts::Create(HWND owner)
{
	CTab::CreateTabWindow(15, 30, 470, 260, owner);

	static const ControlTemplate::Control s_Controls[] =
	{
		CT_GROUPBOX(-1, ID_STR_SAVENEWTHEME,
			0, 0, 230, 150,
			WS_VISIBLE, 0),
		CT_LABEL(-1, ID_STR_THEMEDESCRIPTION,
			6, 16, 205, 44,
			WS_VISIBLE, 0),
		CT_CHECKBOX(Id_SaveEmptyThemeCheckBox, ID_STR_SAVEASEMPTYTHEME,
			6, 70, 220, 9,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_CHECKBOX(Id_ExcludeUnusedSkinsCheckBox, ID_STR_EXCLUDEUNUSEDSKINS,
			6, 83, 220, 9,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_CHECKBOX(Id_IncludeWallpaperCheckBox, ID_STR_INCLUDEWALLPAPER,
			6, 96, 220, 9,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_LABEL(-1, ID_STR_NAMESC,
			6, 115, 100, 9,
			WS_VISIBLE, 0),
		CT_EDIT(Id_NameLabel, 0,
			6, 128, 162, 14,
			WS_VISIBLE | WS_TABSTOP, WS_EX_CLIENTEDGE),
		CT_BUTTON(Id_SaveButton, ID_STR_SAVE,
			172, 128, 50, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),

		CT_GROUPBOX(-1, ID_STR_SAVEDTHEMES,
			238, 0, 230, 150,
			WS_VISIBLE, 0),
		CT_LISTBOX(Id_List, 0,
			244, 16, 160, 125,
			WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_SORT | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, WS_EX_CLIENTEDGE),
		CT_BUTTON(Id_LoadButton, ID_STR_LOAD,
			410, 16, 50, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_BUTTON(Id_EditButton, ID_STR_EDIT,
			410, 34, 50, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_BUTTON(Id_DeleteButton, ID_STR_DELETE,
			410, 52, 50, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0)
	};

	CreateControls(s_Controls, _countof(s_Controls), c_Dialog->m_Font, GetString);
}

void CDialogManage::CTabLayouts::Initialize()
{
	HWND item  = GetControl(Id_List);
	const std::vector<std::wstring>& layouts = Rainmeter->GetAllLayouts();
	for (int i = 0, isize = layouts.size(); i < isize; ++i)
	{
		ListBox_AddString(item, layouts[i].c_str());
	}

	m_Initialized = true;
}

INT_PTR CDialogManage::CTabLayouts::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return OnCommand(wParam, lParam);
	}

	return FALSE;
}

INT_PTR CDialogManage::CTabLayouts::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case Id_SaveEmptyThemeCheckBox:
		{
			BOOL state = !(Button_GetCheck((HWND)lParam) == BST_CHECKED);

			HWND item = GetControl(Id_ExcludeUnusedSkinsCheckBox);
			EnableWindow(item, state);
			Button_SetCheck(item, BST_UNCHECKED);

			item = GetControl(Id_IncludeWallpaperCheckBox);
			EnableWindow(item, state);
			Button_SetCheck(item, BST_UNCHECKED);
		}
		break;

	case Id_NameLabel:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			WCHAR buffer[32];
			int len = Edit_GetText((HWND)lParam, buffer, 32);

			// Disable save button if no text or if backup
			BOOL state = (len > 0 && _wcsicmp(buffer, L"@Backup") != 0);
			EnableWindow(GetControl(Id_SaveButton), state);
		}
		break;

	case Id_List:
		if (HIWORD(wParam) == LBN_SELCHANGE)
		{
			// Ignore clicks that don't hit items
			if (ListBox_GetCurSel((HWND)lParam) != LB_ERR)
			{
				HWND item = GetControl(Id_LoadButton);
				EnableWindow(item, TRUE);
				item = GetControl(Id_DeleteButton);
				EnableWindow(item, TRUE);
				item = GetControl(Id_EditButton);
				EnableWindow(item, TRUE);
				
				const std::vector<std::wstring>& layouts = Rainmeter->GetAllLayouts();
				item  = GetControl(Id_List);
				int sel = ListBox_GetCurSel(item);
				
				item = GetControl(Id_NameLabel);
				Edit_SetText(item, layouts[sel].c_str());
			}
		}
		break;

	case Id_SaveButton:
		{
			WCHAR buffer[MAX_PATH];
			HWND item = GetControl(Id_NameLabel);
			Edit_GetText(item, buffer, MAX_PATH);

			std::wstring layout = buffer;
			std::wstring path = Rainmeter->GetLayoutPath();
			CreateDirectory(path.c_str(), 0);

			path += layout;
			bool alreadyExists = (_waccess(path.c_str(), 0) != -1);
			if (alreadyExists)
			{
				std::wstring text = GetFormattedString(ID_STR_THEMEALREADYEXISTS, layout.c_str());
				if (Rainmeter->ShowMessage(m_Window, text.c_str(), MB_ICONWARNING | MB_YESNO) != IDYES)
				{
					// Cancel
					break;
				}
			}
			else
			{
				// Make sure path exists
				CreateDirectory(path.c_str(), NULL);
			}

			path += L"\\Rainmeter.ini";

			item = GetControl(Id_SaveEmptyThemeCheckBox);
			if (Button_GetCheck(item) != BST_CHECKED)
			{
				if (!CSystem::CopyFiles(Rainmeter->GetIniFile(), path))
				{
					std::wstring text = GetFormattedString(ID_STR_THEMESAVEFAIL, path.c_str());
					Rainmeter->ShowMessage(m_Window, text.c_str(), MB_OK | MB_ICONERROR);
					break;
				}

				// Exclude unused skins
				item = GetControl(Id_ExcludeUnusedSkinsCheckBox);
				if (Button_GetCheck(item) == BST_CHECKED)
				{
					CConfigParser parser;
					parser.Initialize(path);

					// Remove sections with Active=0
					std::list<std::wstring>::const_iterator iter = parser.GetSections().begin();
					for ( ; iter != parser.GetSections().end(); ++iter)
					{
						if (parser.GetValue(*iter, L"Active", L"") == L"0")
						{
							WritePrivateProfileString((*iter).c_str(), NULL, NULL, path.c_str());
						}
					}
				}

				// Save wallpaper
				item = GetControl(Id_IncludeWallpaperCheckBox);
				if (Button_GetCheck(item) == BST_CHECKED)
				{
					// Get current wallpaper
					if (SystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, &buffer, 0))
					{
						std::wstring::size_type pos = path.find_last_of(L'\\');
						path.replace(pos + 1, path.length() - pos - 1, L"Wallpaper.bmp");
						CSystem::CopyFiles((std::wstring)buffer, path);
					}
				}
			}
			else
			{
				// Create empty layout
				HANDLE file = CreateFile(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if (file == INVALID_HANDLE_VALUE)
				{
					std::wstring text = GetFormattedString(ID_STR_THEMESAVEFAIL, path.c_str());
					Rainmeter->ShowMessage(m_Window, text.c_str(), MB_OK | MB_ICONERROR);
					break;
				}

				CloseHandle(file);
			}

			if (!alreadyExists)
			{
				item = GetControl(Id_List);
				ListBox_AddString(item, layout.c_str());

				Rainmeter->ScanForLayouts();
			}
		}
		break;

	case Id_LoadButton:
		{
			HWND item  = GetControl(Id_List);
			int sel = ListBox_GetCurSel(item);
			Rainmeter->LoadLayout(Rainmeter->m_Layouts[sel]);
		}
		break;

	case Id_EditButton:
		{
			HWND item  = GetControl(Id_List);
			int sel = ListBox_GetCurSel(item);
			const std::vector<std::wstring>& layouts = Rainmeter->GetAllLayouts();

			std::wstring args = L"\"" + Rainmeter->GetLayoutPath();
			args += layouts[sel];
			args += L"\\Rainmeter.ini";
			args += L'"';
			CCommandHandler::RunFile(Rainmeter->GetSkinEditor().c_str(), args.c_str());
		}
		break;

	case Id_DeleteButton:
		{
			HWND item  = GetControl(Id_List);
			int sel = ListBox_GetCurSel(item);
			std::vector<std::wstring>& layouts = const_cast<std::vector<std::wstring>&>(Rainmeter->GetAllLayouts());

			std::wstring text = GetFormattedString(ID_STR_THEMEDELETE, layouts[sel].c_str());
			if (Rainmeter->ShowMessage(m_Window, text.c_str(), MB_ICONQUESTION | MB_YESNO) != IDYES)
			{
				// Cancel
				break;
			}

			std::wstring folder = Rainmeter->GetLayoutPath();
			folder += layouts[sel];

			if (CSystem::RemoveFolder(folder))
			{
				ListBox_DeleteString(item, sel);

				// Remove layout from vector
				std::vector<std::wstring>::iterator iter = layouts.begin();
				for ( ; iter != layouts.end(); ++iter)
				{
					if (wcscmp(layouts[sel].c_str(), (*iter).c_str()) == 0)
					{
						layouts.erase(iter);
						break;
					}
				}

				EnableWindow(GetControl(Id_LoadButton), FALSE);
				EnableWindow(GetControl(Id_DeleteButton), FALSE);
				EnableWindow(GetControl(Id_EditButton), FALSE);
			}
		}
		break;

	default:
		return 1;
	}

	return 0;
}

// -----------------------------------------------------------------------------------------------
//
//                                Settings tab
//
// -----------------------------------------------------------------------------------------------

/*
** Constructor.
**
*/
CDialogManage::CTabSettings::CTabSettings() : CTab()
{
}

void CDialogManage::CTabSettings::Create(HWND owner)
{
	CTab::CreateTabWindow(15, 30, 470, 260, owner);

	// FIXME: Temporary hack.
	short buttonWidth = (short)_wtoi(GetString(ID_STR_NUM_BUTTONWIDTH));

	const ControlTemplate::Control s_Controls[] =
	{
		CT_GROUPBOX(-1, ID_STR_GENERAL,
			0, 0, 468, 118,
			WS_VISIBLE, 0),
		CT_LABEL(-1, ID_STR_LANGUAGESC,
			6, 16, 87, 14,
			WS_VISIBLE, 0),
		CT_COMBOBOX(Id_LanguageDropDownList, 0,
			87, 13, 222, 14,
			WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL, 0),
		CT_LABEL(-1, ID_STR_EDITORSC,
			6, 37, 87, 9,
			WS_VISIBLE, 0),
		CT_EDIT(Id_EditorEdit, 0,
			87, 34, 222, 14,
			WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_READONLY, WS_EX_CLIENTEDGE),
		CT_BUTTON(Id_EditorBrowseButton, ID_STR_ELLIPSIS,
			313, 34, 25, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_CHECKBOX(Id_CheckForUpdatesCheckBox, ID_STR_CHECKFORUPDATES,
			6, 55, 150, 9,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_CHECKBOX(Id_LockSkinsCheckBox, ID_STR_DISABLEDRAGGING,
			6, 68, 150, 9,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_CHECKBOX(Id_ShowTrayIconCheckBox, ID_STR_SHOWNOTIFICATIONAREAICON,
			6, 81, 150, 9,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_BUTTON(Id_ResetStatisticsButton, ID_STR_RESETSTATISTICS,
			6, 97, buttonWidth + 20, 14,
			WS_VISIBLE | WS_TABSTOP, 0),

		CT_GROUPBOX(-1, ID_STR_LOGGING,
			0, 125, 468, 66,
			WS_VISIBLE, 0),
		CT_CHECKBOX(Id_VerboseLoggingCheckbox, ID_STR_DEBUGMODE,
			6, 141, 200, 9,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_CHECKBOX(Id_LogToFileCheckBox, ID_STR_LOGTOFILE,
			6, 154, 200, 9,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_BUTTON(Id_ShowLogFileButton, ID_STR_SHOWLOGFILE,
			6, 170, buttonWidth + 20, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_BUTTON(Id_DeleteLogFileButton, ID_STR_DELETELOGFILE,
			buttonWidth + 30, 170, buttonWidth + 20, 14,
			WS_VISIBLE | WS_TABSTOP, 0)
	};

	CreateControls(s_Controls, _countof(s_Controls), c_Dialog->m_Font, GetString);
}

void CDialogManage::CTabSettings::Initialize()
{
	// Scan for languages
	HWND item = GetControl(Id_LanguageDropDownList);

	std::wstring files = Rainmeter->GetPath() + L"Languages\\*.dll";
	WIN32_FIND_DATA fd;
	HANDLE hSearch = FindFirstFile(files.c_str(), &fd);
	if (hSearch != INVALID_HANDLE_VALUE)
	{
		do
		{
			WCHAR* pos = wcschr(fd.cFileName, L'.');
			if (pos)
			{
				LCID lcid = (LCID)wcstoul(fd.cFileName, &pos, 10);
				if (pos != fd.cFileName &&
					_wcsicmp(pos, L".dll") == 0 &&
					GetLocaleInfo(lcid, LOCALE_SENGLISHLANGUAGENAME, fd.cFileName, MAX_PATH) > 0)
				{
					// Strip brackets in language name
					std::wstring text = fd.cFileName;
					text += L" - ";

					GetLocaleInfo(lcid, LOCALE_SNATIVELANGUAGENAME, fd.cFileName, MAX_PATH);
					text += fd.cFileName;

					int index = ComboBox_AddString(item, text.c_str());
					ComboBox_SetItemData(item, index, (LPARAM)lcid);

					if (lcid == Rainmeter->GetResourceLCID())
					{
						ComboBox_SetCurSel(item, index);
					}
				}
			}
		}
		while (FindNextFile(hSearch, &fd));

		FindClose(hSearch);
	}

	Button_SetCheck(GetControl(Id_CheckForUpdatesCheckBox), !Rainmeter->GetDisableVersionCheck());
	Button_SetCheck(GetControl(Id_LockSkinsCheckBox), Rainmeter->GetDisableDragging());
	Button_SetCheck(GetControl(Id_LogToFileCheckBox), CLogger::GetInstance().IsLogToFile());
	Button_SetCheck(GetControl(Id_VerboseLoggingCheckbox), Rainmeter->GetDebug());

	BOOL isLogFile = (_waccess(CLogger::GetInstance().GetLogFilePath().c_str(), 0) != -1);
	EnableWindow(GetControl(Id_ShowLogFileButton), isLogFile);
	EnableWindow(GetControl(Id_DeleteLogFileButton), isLogFile);

	Edit_SetText(GetControl(Id_EditorEdit), Rainmeter->GetSkinEditor().c_str());

	bool iconEnabled = Rainmeter->GetTrayWindow()->IsTrayIconEnabled();
	Button_SetCheck(GetControl(Id_ShowTrayIconCheckBox), iconEnabled);

	m_Initialized = true;
}

INT_PTR CDialogManage::CTabSettings::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return OnCommand(wParam, lParam);
	}

	return FALSE;
}

INT_PTR CDialogManage::CTabSettings::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (!m_Initialized)
	{
		return FALSE;
	}

	switch (LOWORD(wParam))
	{
	case Id_LanguageDropDownList:
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			int sel = ComboBox_GetCurSel((HWND)lParam);
			LCID lcid = (LCID)ComboBox_GetItemData((HWND)lParam, sel);
			if (lcid != Rainmeter->m_ResourceLCID)
			{
				WCHAR buffer[16];
				_ultow(lcid, buffer, 10);
				WritePrivateProfileString(L"Rainmeter", L"Language", buffer, Rainmeter->GetIniFile().c_str());

				std::wstring resource = Rainmeter->GetPath() + L"Languages\\";
				resource += buffer;
				resource += L".dll";
				FreeLibrary(Rainmeter->m_ResourceInstance);
				Rainmeter->m_ResourceInstance = LoadLibraryEx(resource.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
				Rainmeter->m_ResourceLCID = lcid;

				if (CDialogAbout::GetDialog())
				{
					int sel = TabCtrl_GetCurSel(CDialogAbout::GetDialog()->GetControl(CDialogManage::Id_Tab));
					SendMessage(CDialogAbout::GetDialog()->GetWindow(), WM_CLOSE, 0, 0);
					if (sel == 0)
					{
						Rainmeter->DelayedExecuteCommand(L"!About");
					}
					else if (sel == 1)
					{
						Rainmeter->DelayedExecuteCommand(L"!About Skins");
					}
					else if (sel == 2)
					{
						Rainmeter->DelayedExecuteCommand(L"!About Plugins");
					}
					else //if (sel == 3)
					{
						Rainmeter->DelayedExecuteCommand(L"!About Version");
					}
				}

				SendMessage(c_Dialog->GetWindow(), WM_CLOSE, 0, 0);
				Rainmeter->DelayedExecuteCommand(L"!Manage Settings");
			}
		}
		break;

	case Id_CheckForUpdatesCheckBox:
		Rainmeter->SetDisableVersionCheck(!Rainmeter->GetDisableVersionCheck());
		break;

	case Id_LockSkinsCheckBox:
		Rainmeter->SetDisableDragging(!Rainmeter->GetDisableDragging());
		break;

	case Id_ResetStatisticsButton:
		Rainmeter->ResetStats();
		break;

	case Id_ShowLogFileButton:
		Rainmeter->ShowLogFile();
		break;

	case Id_DeleteLogFileButton:
		CLogger::GetInstance().DeleteLogFile();
		if (_waccess(CLogger::GetInstance().GetLogFilePath().c_str(), 0) == -1)
		{
			Button_SetCheck(GetControl(Id_LogToFileCheckBox), BST_UNCHECKED);
			EnableWindow(GetControl(Id_ShowLogFileButton), FALSE);
			EnableWindow(GetControl(Id_DeleteLogFileButton), FALSE);
		}
		break;

	case Id_LogToFileCheckBox:
		if (CLogger::GetInstance().IsLogToFile())
		{
			CLogger::GetInstance().StopLogFile();
		}
		else
		{
			CLogger::GetInstance().StartLogFile();
			if (_waccess(CLogger::GetInstance().GetLogFilePath().c_str(), 0) != -1)
			{
				EnableWindow(GetControl(Id_ShowLogFileButton), TRUE);
				EnableWindow(GetControl(Id_DeleteLogFileButton), TRUE);
			}
		}
		break;

	case Id_VerboseLoggingCheckbox:
		Rainmeter->SetDebug(!Rainmeter->GetDebug());
		break;

	case Id_EditorEdit:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			WCHAR buffer[MAX_PATH];
			if (GetWindowText((HWND)lParam, buffer, _countof(buffer)) > 0)
			{
				Rainmeter->SetSkinEditor(buffer);
			}
		}
		break;

	case Id_EditorBrowseButton:
		{
			WCHAR buffer[MAX_PATH];
			buffer[0] = L'\0';
			
			std::wstring editor = Rainmeter->GetSkinEditor();
			editor = editor.substr(0, editor.find_last_of(L"/\\")).c_str(); 

			OPENFILENAME ofn = { sizeof(OPENFILENAME) };
			ofn.Flags = OFN_FILEMUSTEXIST;
			ofn.lpstrFilter = L"Executable File (.exe)\0*.exe";
			ofn.lpstrTitle = L"Select executable file";
			ofn.lpstrDefExt = L"exe";
			ofn.lpstrInitialDir = editor.c_str();
			ofn.nFilterIndex = 0;
			ofn.lpstrFile = buffer;
			ofn.nMaxFile = _countof(buffer);
			ofn.hwndOwner = c_Dialog->GetWindow();

			if (!GetOpenFileName(&ofn))
			{
				break;
			}

			Edit_SetText(GetControl(Id_EditorEdit), buffer);
		}
		break;	

	case Id_ShowTrayIconCheckBox:
		Rainmeter->GetTrayWindow()->SetTrayIcon(!Rainmeter->GetTrayWindow()->IsTrayIconEnabled());
		break;

	default:
		return 1;
	}

	return 0;
}