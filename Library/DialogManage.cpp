/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../Common/MenuTemplate.h"
#include "Rainmeter.h"
#include "Skin.h"
#include "System.h"
#include "TrayIcon.h"
#include "Measure.h"
#include "resource.h"
#include "DialogManage.h"
#include "DialogAbout.h"
#include "DialogNewSkin.h"
#include "../Version.h"
#include <Commdlg.h>

WINDOWPLACEMENT DialogManage::c_WindowPlacement = {0};
DialogManage* DialogManage::c_Dialog = nullptr;

DialogManage::DialogManage() : Dialog()
{
}

DialogManage::~DialogManage()
{
}

/*
** Opens the Manage dialog by tab name.
**
*/
void DialogManage::Open(const WCHAR* name)
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
void DialogManage::Open(int tab)
{
	if (!c_Dialog)
	{
		c_Dialog = new DialogManage();
	}

	c_Dialog->ShowDialogWindow(
		GetString(ID_STR_MANAGERAINMETER),
		0, 0, 510, 322,
		DS_CENTER | WS_POPUP | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU,
		WS_EX_APPWINDOW | WS_EX_CONTROLPARENT | ((*GetString(ID_STR_ISRTL) == L'1') ? WS_EX_LAYOUTRTL : 0),
		GetRainmeter().GetWindow());

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
void DialogManage::OpenSkin(Skin* skin)
{
	Open();

	if (c_Dialog)
	{
		std::wstring name = skin->GetFolderPath() + L'\\';
		name += skin->GetFileName();

		HWND item = c_Dialog->m_TabSkins.GetControl(TabSkins::Id_SkinsTreeView);
		c_Dialog->m_TabSkins.SelectTreeItem(item, TreeView_GetRoot(item), name.c_str());
	}
}

/*
** Opens the Manage dialog tab with parameters
**
*/
void DialogManage::Open(const WCHAR* tabName, const WCHAR* param1, const WCHAR* param2)
{
	Open(tabName);

	if (c_Dialog)
	{
		// "Skins" tab
		if (_wcsicmp(tabName, L"Skins") == 0)
		{
			// |param1| represents the config (ie. "illustro\Clock")
			// |param2| represents the file (ie. "Clock.ini")

			std::wstring name = param1;

			if (param2)
			{
				name += L'\\';
				name += param2;
			}

			HWND item = c_Dialog->m_TabSkins.GetControl(TabSkins::Id_SkinsTreeView);
			c_Dialog->m_TabSkins.SelectTreeItem(item, TreeView_GetRoot(item), name.c_str());
		}
		// Future use: Allow optional params for different tabs
		//else if (_wcsicmp(tabName, L"Layouts") == 0)
	}
}

void DialogManage::UpdateSelectedSkinOptions(Skin* skin)
{
	if (c_Dialog && c_Dialog->m_TabSkins.IsInitialized())
	{
		c_Dialog->m_TabSkins.UpdateSelected(skin);
	}
}

void DialogManage::UpdateSkins(Skin* skin, bool deleted)
{
	if (c_Dialog && c_Dialog->m_TabSkins.IsInitialized())
	{
		c_Dialog->m_TabSkins.Update(skin, deleted);
	}
}

void DialogManage::UpdateLayouts()
{
	if (c_Dialog && c_Dialog->m_TabLayouts.IsInitialized())
	{
		c_Dialog->m_TabLayouts.Update();
	}
}

Dialog::Tab& DialogManage::GetActiveTab()
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

INT_PTR DialogManage::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			c_Dialog = nullptr;
		}
		return TRUE;
	}

	return FALSE;
}

INT_PTR DialogManage::OnInitDialog(WPARAM wParam, LPARAM lParam)
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
			397, 303, 50, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_BUTTON(Id_CloseButton, ID_STR_CLOSE,
			453, 303, 50, 14,
			WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0),
		CT_TAB(Id_Tab, 0,
			6, 6, 498, 293,
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

	item = m_TabSkins.GetControl(TabSkins::Id_FileLabel);
	SendMessage(item, WM_SETFONT, (WPARAM)m_FontBold, 0);

	// Use arrows instead of plus/minus in the tree for Vista+
	item = m_TabSkins.GetControl(TabSkins::Id_SkinsTreeView);
	SetWindowTheme(item, L"explorer", nullptr);

	if (c_WindowPlacement.length == 0)
	{
		c_WindowPlacement.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(m_Window, &c_WindowPlacement);
	}

	SetWindowPlacement(m_Window, &c_WindowPlacement);

	return FALSE;
}

INT_PTR DialogManage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case Id_RefreshAllButton:
		GetRainmeter().RefreshAll();
		break;

	case Id_EditSettingsButton:
		GetRainmeter().EditSettings();
		break;

	case Id_OpenLogButton:
		DialogAbout::Open();
		break;

	case Id_CloseButton:
		HandleMessage(WM_CLOSE, 0, 0);
		break;

	case Id_HelpButton:
		{
			std::wstring url = L"https://docs.rainmeter.net/manual/user-interface/manage#";

			Tab& tab = GetActiveTab();
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
			ShellExecute(m_Window, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

INT_PTR DialogManage::OnNotify(WPARAM wParam, LPARAM lParam)
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

HBRUSH DialogManage::TabSkins::s_NewSkinBkBrush = NULL;
COLORREF DialogManage::TabSkins::s_NewSkinBkColor = RGB(229, 241, 251); // default color

DialogManage::TabSkins::TabSkins() : Tab(),
	m_SkinWindow(),
	m_HandleCommands(false),
	m_IgnoreUpdate(false)
{
}

DialogManage::TabSkins::~TabSkins()
{
	HWND item = GetControl(Id_NewSkinButton);
	RemoveWindowSubclass(item, &NewSkinButtonSubclass, 1);

	if (s_NewSkinBkBrush)
	{
		DeleteObject(s_NewSkinBkBrush);
		s_NewSkinBkBrush = nullptr;
	}
}

void DialogManage::TabSkins::Create(HWND owner)
{
	Tab::CreateTabWindow(15, 30, 480, 260, owner);

	// FIXME: Temporary hack.
	short labelWidth = (short)_wtoi(GetString(ID_STR_NUM_LABELWIDTH));

	const ControlTemplate::Control s_Controls[] =
	{
		CT_BUTTON(Id_ActiveSkinsButton, ID_STR_ACTIVESKINS,
			0, 0, 134, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_ICON(Id_NewSkinButton, 0,
			138, 0, 18, 14,
			WS_VISIBLE | WS_TABSTOP | SS_ICON | SS_CENTERIMAGE | SS_NOTIFY, 0),
		CT_TREEVIEW(Id_SkinsTreeView, 0,
			0, 18, 155, 221,
			WS_VISIBLE | WS_TABSTOP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS | WS_VSCROLL, WS_EX_CLIENTEDGE),
		CT_BUTTON(Id_CreateSkinPackageButton, ID_STR_CREATERMSKINPACKAGE,
			0, 244, 156, 14,
			WS_VISIBLE | WS_TABSTOP, 0),

		CT_LABEL(Id_FileLabel, ID_STR_ELLIPSIS,
			175, 0, 130, 14,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),
		CT_LABEL(Id_ConfigLabel, 0,
			175, 15, 130, 14,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),
		CT_BUTTON(Id_LoadButton, ID_STR_LOAD,
			320, 0, 50, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_BUTTON(Id_RefreshButton, ID_STR_REFRESH,
			374, 0, 50, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_BUTTON(Id_EditButton, ID_STR_EDIT,
			428, 0, 50, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),

		CT_LABEL(-1, ID_STR_AUTHORSC,
			175, 30, 80, 9,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),
		CT_LABEL(Id_AuthorLabel, 0,
			240, 30, 245, 9,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),
		CT_LABEL(-1, ID_STR_VERSIONSC,
			175, 43, 80, 9,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),
		CT_LABEL(Id_VersionLabel, 0,
			240, 43, 245, 9,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),
		CT_LABEL(-1, ID_STR_LICENSESC,
			175, 56, 80, 9,
			WS_VISIBLE | WS_TABSTOP | SS_NOPREFIX, 0),
		CT_LABEL(Id_LicenseLabel, 0,
			240, 56, 245, 9,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),
		CT_LABEL(-1, ID_STR_INFORMATIONSC,
			175, 69, 80, 9,
			WS_VISIBLE | SS_ENDELLIPSIS | SS_NOPREFIX, 0),
		CT_EDIT(Id_DescriptionLabel, 0,
			238, 69, 238, 64,
			WS_VISIBLE | ES_MULTILINE | ES_READONLY, 0),
		CT_LINKLABEL(Id_AddMetadataLink, ID_STR_ADDMETADATA,
			175, 142, 150, 9,
			0, 0),

		CT_LINEH(-1, ID_STR_COORDINATESSC,
			175, 156, 304, 1,
			WS_VISIBLE, 0),

		CT_LABEL(-1, ID_STR_COORDINATESSC,
			175, 169, labelWidth, 9,
			WS_VISIBLE, 0),
		CT_EDIT(Id_XPositionEdit, 0,
			175 + labelWidth, 166, 38, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, WS_EX_CLIENTEDGE),
		CT_EDIT(Id_YPositionEdit, 0,
			175 + labelWidth + 42, 166, 38, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, WS_EX_CLIENTEDGE),
		CT_LABEL(-1, ID_STR_POSITIONSC,
			175, 190, labelWidth, 9,
			WS_VISIBLE, 0),
		CT_COMBOBOX(Id_ZPositionDropDownList, 0,
			175 + labelWidth, 187, 80, 14,
			WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL | WS_DISABLED, 0),
		CT_LABEL(-1, ID_STR_LOADORDERSC,
			175, 208, labelWidth, 9,
			WS_VISIBLE, 0),
		CT_EDIT(Id_LoadOrderEdit, 0,
			175 + labelWidth, 205, 80, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, WS_EX_CLIENTEDGE),
		CT_LABEL(-1, ID_STR_TRANSPARENCYSC,
			175, 229, labelWidth, 9,
			WS_VISIBLE, 0),
		CT_COMBOBOX(Id_TransparencyDropDownList, 0,
			175 + labelWidth, 226, 80, 14,
			WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL | WS_DISABLED, 0),
		CT_LABEL(-1, ID_STR_ONHOVERSC,
			175, 247, labelWidth, 9,
			WS_VISIBLE, 0),
		CT_COMBOBOX(Id_OnHoverDropDownList, 0,
			175 + labelWidth, 244, 80, 14,
			WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL | WS_DISABLED, 0),

		CT_BUTTON(Id_DisplayMonitorButton, ID_STR_DISPLAYMONITOR,
			360, 166, 118, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_CHECKBOX(Id_DraggableCheckBox, ID_STR_DRAGGABLE,
			360, 185, 118, 9,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_CHECKBOX(Id_ClickThroughCheckBox, ID_STR_CLICKTHROUGH,
			360, 198, 118, 9,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_CHECKBOX(Id_KeepOnScreenCheckBox, ID_STR_KEEPONSCREEN,
			360, 211, 118, 9,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_CHECKBOX(Id_SavePositionCheckBox, ID_STR_SAVEPOSITION,
			360, 224, 118, 9,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_CHECKBOX(Id_SnapToEdgesCheckBox, ID_STR_SNAPTOEDGES,
			360, 237, 118, 9,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_CHECKBOX(Id_FavoriteCheckBox, ID_STR_FAVORITE,
			360, 250, 118, 9,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0)
	};

	CreateControls(s_Controls, _countof(s_Controls), c_Dialog->m_Font, GetString);

	// Create tooltips for 'New Skin' button
	HWND item = GetControl(Id_NewSkinButton);

	HWND hwndTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		m_Window, NULL,
		GetModuleHandle(NULL), NULL);

	if (!hwndTip) return;

	TOOLINFO toolInfo = { 0 };
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = m_Window;
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	toolInfo.uId = (UINT_PTR)item;
	toolInfo.lpszText = GetString(ID_STR_CREATENEWSKIN);
	SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

	SetWindowSubclass(item, &NewSkinButtonSubclass, 1, 0);
}

void DialogManage::TabSkins::Initialize()
{
	BUTTON_SPLITINFO bsi;
	bsi.mask = BCSIF_SIZE;
	bsi.size.cx = 20;
	bsi.size.cy = 14;

	HWND item = GetControl(Id_ActiveSkinsButton);
	Dialog::SetMenuButton(item);

	item = GetControl(Id_DisplayMonitorButton);
	Dialog::SetMenuButton(item);

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
	Update(nullptr, false);

	// Apply icon to new skin button
	hIcon = GetIcon(IDI_ADDFOLDER);
	SendDlgItemMessage(m_Window, Id_NewSkinButton, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hIcon);

	// Get rid of the EDITTEXT control border
	item = GetControl(Id_DescriptionLabel);
	SetWindowLongPtr(item, GWL_EXSTYLE, GetWindowLongPtr(item, GWL_EXSTYLE) &~ WS_EX_CLIENTEDGE);
	SetWindowPos(item, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER); 

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
	ComboBox_AddString(item, L"~100%");

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

void DialogManage::TabSkins::UpdateSelected(Skin* skin)
{
	if (m_SkinWindow && m_SkinWindow == skin)
	{
		bool selected = skin->IsSelected();

		HWND item = GetControl(Id_DraggableCheckBox);
		EnableWindow(item, selected ? FALSE : TRUE);
		item = GetControl(Id_KeepOnScreenCheckBox);
		EnableWindow(item, selected ? FALSE : TRUE);
		item = GetControl(Id_ClickThroughCheckBox);
		EnableWindow(item, selected ? FALSE : TRUE);
	}
}

/*
** Updates metadata and settings when changed.
**
*/
void DialogManage::TabSkins::Update(Skin* skin, bool deleted)
{
	if (skin)
	{
		if (!deleted && m_IgnoreUpdate)
		{
			// Changed setting from dialog, no need to update
			m_IgnoreUpdate = false;
		}
		else if (m_SkinWindow && m_SkinWindow == skin) 
		{
			// Update from currently open skin
			m_HandleCommands = false;
			if (deleted)
			{
				DisableControls();
				m_SkinWindow = nullptr;
			}
			else
			{
				SetControls();
			}
			m_HandleCommands = true;
		}
		else if (wcscmp(skin->GetFolderPath().c_str(), m_SkinFolderPath.c_str()) == 0 &&
				 wcscmp(skin->GetFileName().c_str(), m_SkinFileName.c_str()) == 0)
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

		if (!GetRainmeter().m_SkinRegistry.IsEmpty())
		{
			PopulateTree(item, tvi);
		}
	}
}

void DialogManage::TabSkins::SetControls()
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
		if (GetRainmeter().GetDisableDragging())
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

		item = GetControl(Id_FavoriteCheckBox);
		EnableWindow(item, TRUE);
		Button_SetCheck(item, m_SkinWindow->GetFavorite());

		item = GetControl(Id_TransparencyDropDownList);
		EnableWindow(item, TRUE);
		int alpha = m_SkinWindow->GetAlphaValue();
		if (alpha <= 1)	// ~100%
		{
			ComboBox_SetCurSel(item, 10);
		}
		else
		{
			int value = (int)(10 - alpha / 25.5);
			value = min(9, value);
			value = max(0, value);
			ComboBox_SetCurSel(item, value);
		}

		item = GetControl(Id_ZPositionDropDownList);
		EnableWindow(item, TRUE);
		ComboBox_SetCurSel(item, m_SkinWindow->GetWindowZPosition() + 2);

		item = GetControl(Id_LoadOrderEdit);
		EnableWindow(item, TRUE);
		_itow_s(GetRainmeter().GetLoadOrder(m_SkinFolderPath), buffer, 10);
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

void DialogManage::TabSkins::DisableControls(bool clear)
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

	item = GetControl(Id_FavoriteCheckBox);
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

void DialogManage::TabSkins::ReadSkin()
{
	HWND item = GetControl(Id_FileLabel);
	SetWindowText(item, m_SkinFileName.c_str());

	PathSetDlgItemPath(m_Window, Id_ConfigLabel, m_SkinFolderPath.c_str());

	item = GetControl(Id_EditButton);
	EnableWindow(item, TRUE);

	std::wstring file = GetRainmeter().GetSkinPath() + m_SkinFolderPath;
	file += L'\\';
	file += m_SkinFileName;
	m_SkinWindow = GetRainmeter().GetSkinByINI(file);
	if (!m_SkinWindow)
	{
		DisableControls();
	}

	SetControls();

	WCHAR* buffer = new WCHAR[MAX_LINE_LENGTH];
	const WCHAR* fileSz = file.c_str();

	item = GetControl(Id_AuthorLabel);
	if (GetPrivateProfileString(L"Metadata", L"Author", nullptr, buffer, MAX_LINE_LENGTH, fileSz) == 0)
	{
		// For backwards compatibility.
		GetPrivateProfileString(L"Rainmeter", L"Author", nullptr, buffer, MAX_LINE_LENGTH, fileSz);
	}
	SetWindowText(item, buffer);

	item = GetControl(Id_AddMetadataLink);
	if (GetPrivateProfileSection(L"Metadata", buffer, 8, fileSz) > 0)
	{
		ShowWindow(item, SW_HIDE);

		// Set metadata
		item = GetControl(Id_VersionLabel);
		GetPrivateProfileString(L"Metadata", L"Version", nullptr, buffer, MAX_LINE_LENGTH, fileSz);
		SetWindowText(item, buffer);

		item = GetControl(Id_LicenseLabel);
		GetPrivateProfileString(L"Metadata", L"License", nullptr, buffer, MAX_LINE_LENGTH, fileSz);
		SetWindowText(item, buffer);

		item = GetControl(Id_DescriptionLabel);
		std::wstring text;
		if (GetPrivateProfileString(L"Metadata", L"Information", nullptr, buffer, MAX_LINE_LENGTH, fileSz) > 0)
		{
			text = buffer;
		}
		else
		{
			// For backwards compatibility
			GetPrivateProfileString(L"Metadata", L"Description", nullptr, buffer, MAX_LINE_LENGTH, fileSz);
			text = buffer;

			if (GetPrivateProfileString(L"Metadata", L"Instructions", nullptr, buffer, MAX_LINE_LENGTH, fileSz) > 0)
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

LRESULT CALLBACK DialogManage::TabSkins::NewSkinButtonSubclass(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uId, DWORD_PTR data)
{
	static bool hasEntered = false;

	switch (msg)
	{
	case WM_MOUSEMOVE:
		{
			// Only re-create brush if needed
			if (hasEntered) break;

			hasEntered = true;

			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = TME_HOVER | TME_LEAVE;
			tme.dwHoverTime = 1;
			tme.hwndTrack = hwnd;
			TrackMouseEvent(&tme);

			if (s_NewSkinBkBrush) DeleteObject(s_NewSkinBkBrush);
			s_NewSkinBkBrush = CreateSolidBrush(s_NewSkinBkColor);
			InvalidateRect(hwnd, 0, TRUE);
		}
		break;

	case WM_MOUSELEAVE:
		{
			hasEntered = false;

			if (s_NewSkinBkBrush)
			{
				DeleteObject(s_NewSkinBkBrush);
				s_NewSkinBkBrush = NULL;
				InvalidateRect(hwnd, 0, TRUE);
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

std::wstring DialogManage::TabSkins::GetTreeSelectionPath(HWND tree)
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
	while ((tvi.hItem = TreeView_GetParent(tree, tvi.hItem)) != nullptr)
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
int DialogManage::TabSkins::PopulateTree(HWND tree, TVINSERTSTRUCT& tvi, int index)
{
	int initialLevel = GetRainmeter().m_SkinRegistry.GetFolder(index).level;

	const int max = (int)GetRainmeter().m_SkinRegistry.GetFolderCount();
	while (index < max)
	{
		const auto& skinFolder = GetRainmeter().m_SkinRegistry.GetFolder(index);
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
			GetRainmeter().m_SkinRegistry.GetFolder(index + 1).level == initialLevel + 1)
		{
			index = PopulateTree(tree, tvi, index + 1);
		}

		// Add files
		tvi.item.iImage = tvi.item.iSelectedImage = 1;
		for (int i = 0, isize = (int)skinFolder.files.size(); i < isize; ++i)
		{
			tvi.item.pszText = (WCHAR*)skinFolder.files[i].filename.c_str();
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
void DialogManage::TabSkins::SelectTreeItem(HWND tree, HTREEITEM item, LPCWSTR name)
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
		tvi.cchTextMax = folderLen + 1;		// Length of folder name plus 1 for nullptr

		// Find and expand the folder
		do
		{
			TreeView_GetItem(tree, &tvi);
			if (wcsncmp(buffer, name, folderLen) == 0)
			{
				if ((item = TreeView_GetChild(tree, tvi.hItem)) != nullptr)
				{
					TreeView_Expand(tree, tvi.hItem, TVE_EXPAND);
					TreeView_Select(tree, tvi.hItem, TVGN_CARET);
					++pos;	// Skip the slash
					SelectTreeItem(tree, item, pos);
				}

				break;
			}
		}
		while ((tvi.hItem = TreeView_GetNextSibling(tree, tvi.hItem)) != nullptr);
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
		while ((tvi.hItem = TreeView_GetNextSibling(tree, tvi.hItem)) != nullptr);
	}
}

INT_PTR DialogManage::TabSkins::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return OnCommand(wParam, lParam);

	case WM_NOTIFY:
		return OnNotify(wParam, lParam);

	case WM_CTLCOLORSTATIC:
		{
			HWND hwnd = (HWND)lParam;
			if (GetDlgCtrlID(hwnd) == Id_NewSkinButton)
			{
				return (INT_PTR)s_NewSkinBkBrush;
			}
		}
		break;
	}

	return FALSE;
}

INT_PTR DialogManage::TabSkins::OnCommand(WPARAM wParam, LPARAM lParam)
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
			std::map<std::wstring, Skin*>::const_iterator iter = GetRainmeter().GetAllSkins().begin();
			int index = 0;
			for ( ; iter != GetRainmeter().GetAllSkins().end(); ++iter)
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
					nullptr
				);
			}

			DestroyMenu(menu);
		}
		break;

	case Id_NewSkinButton:
		DialogNewSkin::Open(L"New", nullptr);
		break;

	case Id_CreateSkinPackageButton:
		{
			std::wstring file = GetRainmeter().GetPath() + L"SkinInstaller.exe";
			CommandHandler::RunFile(file.c_str(), L"/Packager");
		}
		break;

	case Id_LoadButton:
		{
			if (!m_SkinWindow)
			{
				// Skin not active, load
				const SkinRegistry::Indexes indexes =
					GetRainmeter().m_SkinRegistry.FindIndexes(m_SkinFolderPath, m_SkinFileName);
				if (indexes.IsValid())
				{
					m_HandleCommands = false;
					GetRainmeter().ActivateSkin(indexes.folder, indexes.file);
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
				GetRainmeter().DeactivateSkin(m_SkinWindow, -1);
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
		GetRainmeter().EditSkinFile(m_SkinFolderPath, m_SkinFileName);
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

				WritePrivateProfileString(m_SkinFolderPath.c_str(), L"LoadOrder", buffer, GetRainmeter().GetIniFile().c_str());
				const SkinRegistry::Indexes indexes = GetRainmeter().m_SkinRegistry.FindIndexes(
					m_SkinWindow->GetFolderPath(), m_SkinWindow->GetFileName());
				if (indexes.IsValid())
				{
					GetRainmeter().SetLoadOrder(indexes.folder, value);

					std::multimap<int, Skin*> windows;
					GetRainmeter().GetSkinsByLoadOrder(windows);

					System::PrepareHelperWindow();

					// Reorder window z-position to reflect load order
					std::multimap<int, Skin*>::const_iterator iter = windows.begin();
					for ( ; iter != windows.end(); ++iter)
					{
						Skin* skin = (*iter).second;
						skin->ChangeZPos(skin->GetWindowZPosition(), true);
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
				ContextMenu::CreateMonitorMenu(menu, m_SkinWindow);

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
					nullptr
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

	case Id_FavoriteCheckBox:
		m_IgnoreUpdate = true;
		m_SkinWindow->SetFavorite(!m_SkinWindow->GetFavorite());
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
			GetRainmeter().OpenSkinFolder(GetTreeSelectionPath(tree));
		}
		break;

	case IDM_CREATENEWSKIN:
		{
			HWND tree = GetControl(Id_SkinsTreeView);
			std::wstring path = GetTreeSelectionPath(tree);
			DialogNewSkin::Open(L"New", path.c_str());
		}
		break;

	default:
		if (wParam >= ID_CONFIG_FIRST && wParam <= ID_CONFIG_LAST)
		{
			std::map<std::wstring, Skin*>::const_iterator iter = GetRainmeter().GetAllSkins().begin();
			int index = (int)wParam - ID_CONFIG_FIRST;
			int i = 0;
			for ( ; iter != GetRainmeter().GetAllSkins().end(); ++iter)
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

INT_PTR DialogManage::TabSkins::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR nm = (LPNMHDR)lParam;
	switch (nm->code)
	{
	case NM_CLICK:
		if (nm->idFrom == Id_AddMetadataLink)
		{
			std::wstring file = GetRainmeter().GetSkinPath() + m_SkinFolderPath;
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
			POINT pt = System::GetCursorPosition();

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
					HMENU menu = nullptr;
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
							MENU_ITEM(IDM_CREATENEWSKIN, ID_STR_CREATENEWSKIN)
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
						nullptr
					);

					DestroyMenu(menu);
				}
			}
		}
		break;

	case TVN_SELCHANGED:
		if (nm->idFrom == Id_SkinsTreeView)
		{
			m_SkinWindow = nullptr;
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
				while ((tvi.hItem = TreeView_GetParent(nm->hwndFrom, tvi.hItem)) != nullptr)
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

DialogManage::TabLayouts::TabLayouts() : Tab()
{
}

void DialogManage::TabLayouts::Create(HWND owner)
{
	Tab::CreateTabWindow(15, 30, 480, 260, owner);

	static const ControlTemplate::Control s_Controls[] =
	{
		CT_GROUPBOX(-1, ID_STR_SAVENEWTHEME,
			0, 0, 235, 150,
			WS_VISIBLE, 0),
		CT_LABEL(-1, ID_STR_THEMEDESCRIPTION,
			6, 16, 210, 44,
			WS_VISIBLE, 0),
		CT_CHECKBOX(Id_SaveEmptyThemeCheckBox, ID_STR_SAVEASEMPTYTHEME,
			6, 70, 225, 9,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_CHECKBOX(Id_ExcludeUnusedSkinsCheckBox, ID_STR_EXCLUDEUNUSEDSKINS,
			6, 83, 225, 9,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_CHECKBOX(Id_IncludeWallpaperCheckBox, ID_STR_INCLUDEWALLPAPER,
			6, 96, 225, 9,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_LABEL(-1, ID_STR_NAMESC,
			6, 115, 105, 9,
			WS_VISIBLE, 0),
		CT_EDIT(Id_NameLabel, 0,
			6, 128, 167, 14,
			WS_VISIBLE | WS_TABSTOP, WS_EX_CLIENTEDGE),
		CT_BUTTON(Id_SaveButton, ID_STR_SAVE,
			177, 128, 50, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),

		CT_GROUPBOX(-1, ID_STR_SAVEDTHEMES,
			243, 0, 235, 150,
			WS_VISIBLE, 0),
		CT_LISTBOX(Id_List, 0,
			249, 16, 165, 125,
			WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, WS_EX_CLIENTEDGE),
		CT_BUTTON(Id_LoadButton, ID_STR_LOAD,
			420, 16, 50, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_BUTTON(Id_EditButton, ID_STR_EDIT,
			420, 34, 50, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_BUTTON(Id_DeleteButton, ID_STR_DELETE,
			420, 52, 50, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0)
	};

	CreateControls(s_Controls, _countof(s_Controls), c_Dialog->m_Font, GetString);
}

void DialogManage::TabLayouts::Initialize()
{
	HWND item = GetControl(Id_List);
	ListBox_ResetContent(item);
	const std::vector<std::wstring>& layouts = GetRainmeter().GetAllLayouts();
	for (size_t i = 0, isize = layouts.size(); i < isize; ++i)
	{
		ListBox_AddString(item, layouts[i].c_str());
	}

	// Assure buttons are disabled
	item = GetControl(Id_LoadButton);
	EnableWindow(item, FALSE);
	item = GetControl(Id_DeleteButton);
	EnableWindow(item, FALSE);
	item = GetControl(Id_EditButton);
	EnableWindow(item, FALSE);

	m_Initialized = true;
}

void DialogManage::TabLayouts::Update()
{
	Initialize();
}

INT_PTR DialogManage::TabLayouts::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return OnCommand(wParam, lParam);
	}

	return FALSE;
}

INT_PTR DialogManage::TabLayouts::OnCommand(WPARAM wParam, LPARAM lParam)
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
				
				const std::vector<std::wstring>& layouts = GetRainmeter().GetAllLayouts();
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
			std::wstring path = GetRainmeter().GetLayoutPath();
			CreateDirectory(path.c_str(), 0);

			path += layout;
			bool alreadyExists = (_waccess(path.c_str(), 0) != -1);
			if (alreadyExists)
			{
				std::wstring text = GetFormattedString(ID_STR_THEMEALREADYEXISTS, layout.c_str());
				if (GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONWARNING | MB_YESNO) != IDYES)
				{
					// Cancel
					break;
				}
			}
			else
			{
				// Make sure path exists
				CreateDirectory(path.c_str(), nullptr);
			}

			path += L"\\Rainmeter.ini";

			item = GetControl(Id_SaveEmptyThemeCheckBox);
			if (Button_GetCheck(item) != BST_CHECKED)
			{
				if (!System::CopyFiles(GetRainmeter().GetIniFile(), path))
				{
					std::wstring text = GetFormattedString(ID_STR_THEMESAVEFAIL, path.c_str());
					GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_OK | MB_ICONERROR);
					break;
				}

				// Exclude unused skins
				item = GetControl(Id_ExcludeUnusedSkinsCheckBox);
				if (Button_GetCheck(item) == BST_CHECKED)
				{
					ConfigParser parser;
					parser.Initialize(path);

					// Remove sections with Active=0
					std::list<std::wstring>::const_iterator iter = parser.GetSections().begin();
					for ( ; iter != parser.GetSections().end(); ++iter)
					{
						if (parser.GetValue(*iter, L"Active", L"") == L"0")
						{
							WritePrivateProfileString((*iter).c_str(), nullptr, nullptr, path.c_str());
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
						System::CopyFiles((std::wstring)buffer, path);
					}
				}
			}
			else
			{
				// Create empty layout
				HANDLE file = CreateFile(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
				if (file == INVALID_HANDLE_VALUE)
				{
					std::wstring text = GetFormattedString(ID_STR_THEMESAVEFAIL, path.c_str());
					GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_OK | MB_ICONERROR);
					break;
				}

				CloseHandle(file);
			}

			if (!alreadyExists)
			{
				item = GetControl(Id_List);
				ListBox_AddString(item, layout.c_str());

				GetRainmeter().ScanForLayouts();
			}
		}
		break;

	case Id_LoadButton:
		{
			HWND item  = GetControl(Id_List);
			int sel = ListBox_GetCurSel(item);
			GetRainmeter().LoadLayout(GetRainmeter().m_Layouts[sel]);
		}
		break;

	case Id_EditButton:
		{
			HWND item  = GetControl(Id_List);
			int sel = ListBox_GetCurSel(item);
			const std::vector<std::wstring>& layouts = GetRainmeter().GetAllLayouts();

			std::wstring args = L"\"" + GetRainmeter().GetLayoutPath();
			args += layouts[sel];
			args += L"\\Rainmeter.ini";
			args += L'"';
			CommandHandler::RunFile(GetRainmeter().GetSkinEditor().c_str(), args.c_str());
		}
		break;

	case Id_DeleteButton:
		{
			HWND item  = GetControl(Id_List);
			int sel = ListBox_GetCurSel(item);
			std::vector<std::wstring>& layouts = const_cast<std::vector<std::wstring>&>(GetRainmeter().GetAllLayouts());

			std::wstring text = GetFormattedString(ID_STR_THEMEDELETE, layouts[sel].c_str());
			if (GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONQUESTION | MB_YESNO) != IDYES)
			{
				// Cancel
				break;
			}

			std::wstring folder = GetRainmeter().GetLayoutPath();
			folder += layouts[sel];

			if (System::RemoveFolder(folder))
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

DialogManage::TabSettings::TabSettings() : Tab()
{
}

void DialogManage::TabSettings::Create(HWND owner)
{
	Tab::CreateTabWindow(15, 30, 480, 260, owner);

	// FIXME: Temporary hack.
	short buttonWidth = (short)_wtoi(GetString(ID_STR_NUM_BUTTONWIDTH));

	const ControlTemplate::Control s_Controls[] =
	{
		CT_GROUPBOX(-1, ID_STR_GENERAL,
			0, 0, 478, 118,
			WS_VISIBLE, 0),
		CT_LABEL(-1, ID_STR_LANGUAGESC,
			6, 16, 107, 14,
			WS_VISIBLE, 0),
		CT_COMBOBOX(Id_LanguageDropDownList, 0,
			107, 13, 250, 14,
			WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL, 0),
		CT_LABEL(-1, ID_STR_EDITORSC,
			6, 37, 107, 9,
			WS_VISIBLE, 0),
		CT_EDIT(Id_EditorEdit, 0,
			107, 34, 250, 14,
			WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_READONLY, WS_EX_CLIENTEDGE),
		CT_BUTTON(Id_EditorBrowseButton, ID_STR_ELLIPSIS,
			361, 34, 25, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_CHECKBOX(Id_CheckForUpdatesCheckBox, ID_STR_CHECKFORUPDATES,
			6, 55, 200, 9,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_CHECKBOX(Id_LockSkinsCheckBox, ID_STR_DISABLEDRAGGING,
			6, 68, 200, 9,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_CHECKBOX(Id_ShowTrayIconCheckBox, ID_STR_SHOWNOTIFICATIONAREAICON,
			6, 81, 200, 9,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_BUTTON(Id_ResetStatisticsButton, ID_STR_RESETSTATISTICS,
			6, 97, buttonWidth + 20, 14,
			WS_VISIBLE | WS_TABSTOP, 0),

		CT_GROUPBOX(-1, ID_STR_LOGGING,
			0, 125, 478, 66,
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

void DialogManage::TabSettings::Initialize()
{
	// Scan for languages
	HWND item = GetControl(Id_LanguageDropDownList);

	std::wstring files = GetRainmeter().GetPath() + L"Languages\\*.dll";
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

					GetLocaleInfo(lcid, LOCALE_SNATIVEDISPLAYNAME, fd.cFileName, MAX_PATH);
					text += fd.cFileName;

					int index = ComboBox_AddString(item, text.c_str());
					ComboBox_SetItemData(item, index, (LPARAM)lcid);

					if (lcid == GetRainmeter().GetResourceLCID())
					{
						ComboBox_SetCurSel(item, index);
					}
				}
			}
		}
		while (FindNextFile(hSearch, &fd));

		FindClose(hSearch);
	}

	Button_SetCheck(GetControl(Id_CheckForUpdatesCheckBox), !GetRainmeter().GetDisableVersionCheck());
	Button_SetCheck(GetControl(Id_LockSkinsCheckBox), GetRainmeter().GetDisableDragging());
	Button_SetCheck(GetControl(Id_LogToFileCheckBox), GetLogger().IsLogToFile());
	Button_SetCheck(GetControl(Id_VerboseLoggingCheckbox), GetRainmeter().GetDebug());

	BOOL isLogFile = (_waccess(GetLogger().GetLogFilePath().c_str(), 0) != -1);
	EnableWindow(GetControl(Id_ShowLogFileButton), isLogFile);
	EnableWindow(GetControl(Id_DeleteLogFileButton), isLogFile);

	Edit_SetText(GetControl(Id_EditorEdit), GetRainmeter().GetSkinEditor().c_str());

	bool iconEnabled = GetRainmeter().GetTrayIcon()->IsTrayIconEnabled();
	Button_SetCheck(GetControl(Id_ShowTrayIconCheckBox), iconEnabled);

	m_Initialized = true;
}

INT_PTR DialogManage::TabSettings::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return OnCommand(wParam, lParam);
	}

	return FALSE;
}

INT_PTR DialogManage::TabSettings::OnCommand(WPARAM wParam, LPARAM lParam)
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
			if (lcid != GetRainmeter().m_ResourceLCID)
			{
				WCHAR buffer[16];
				_ultow(lcid, buffer, 10);
				WritePrivateProfileString(L"Rainmeter", L"Language", buffer, GetRainmeter().GetIniFile().c_str());

				std::wstring resource = GetRainmeter().GetPath() + L"Languages\\";
				resource += buffer;
				resource += L".dll";
				FreeLibrary(GetRainmeter().m_ResourceInstance);
				GetRainmeter().m_ResourceInstance = LoadLibraryEx(resource.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
				GetRainmeter().m_ResourceLCID = lcid;

				if (DialogAbout::GetDialog())
				{
					int sel = TabCtrl_GetCurSel(DialogAbout::GetDialog()->GetControl(DialogManage::Id_Tab));
					SendMessage(DialogAbout::GetDialog()->GetWindow(), WM_CLOSE, 0, 0);
					if (sel == 0)
					{
						GetRainmeter().DelayedExecuteCommand(L"!About");
					}
					else if (sel == 1)
					{
						GetRainmeter().DelayedExecuteCommand(L"!About Skins");
					}
					else if (sel == 2)
					{
						GetRainmeter().DelayedExecuteCommand(L"!About Plugins");
					}
					else //if (sel == 3)
					{
						GetRainmeter().DelayedExecuteCommand(L"!About Version");
					}
				}

				SendMessage(c_Dialog->GetWindow(), WM_CLOSE, 0, 0);
				GetRainmeter().DelayedExecuteCommand(L"!Manage Settings");
			}
		}
		break;

	case Id_CheckForUpdatesCheckBox:
		GetRainmeter().SetDisableVersionCheck(!GetRainmeter().GetDisableVersionCheck());
		break;

	case Id_LockSkinsCheckBox:
		GetRainmeter().SetDisableDragging(!GetRainmeter().GetDisableDragging());
		break;

	case Id_ResetStatisticsButton:
		GetRainmeter().ResetStats();
		break;

	case Id_ShowLogFileButton:
		GetRainmeter().ShowLogFile();
		break;

	case Id_DeleteLogFileButton:
		GetLogger().DeleteLogFile();
		if (_waccess(GetLogger().GetLogFilePath().c_str(), 0) == -1)
		{
			Button_SetCheck(GetControl(Id_LogToFileCheckBox), BST_UNCHECKED);
			EnableWindow(GetControl(Id_ShowLogFileButton), FALSE);
			EnableWindow(GetControl(Id_DeleteLogFileButton), FALSE);
		}
		break;

	case Id_LogToFileCheckBox:
		if (GetLogger().IsLogToFile())
		{
			GetLogger().StopLogFile();
		}
		else
		{
			GetLogger().StartLogFile();
			if (_waccess(GetLogger().GetLogFilePath().c_str(), 0) != -1)
			{
				EnableWindow(GetControl(Id_ShowLogFileButton), TRUE);
				EnableWindow(GetControl(Id_DeleteLogFileButton), TRUE);
			}
		}
		break;

	case Id_VerboseLoggingCheckbox:
		GetRainmeter().SetDebug(!GetRainmeter().GetDebug());
		break;

	case Id_EditorEdit:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			WCHAR buffer[MAX_PATH];
			if (GetWindowText((HWND)lParam, buffer, _countof(buffer)) > 0)
			{
				GetRainmeter().SetSkinEditor(buffer);
			}
		}
		break;

	case Id_EditorBrowseButton:
		{
			WCHAR buffer[MAX_PATH];
			buffer[0] = L'\0';
			
			std::wstring editor = GetRainmeter().GetSkinEditor();
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
		{
			bool isTrayEnabled = GetRainmeter().GetTrayIcon()->IsTrayIconEnabled();
			GetRainmeter().GetTrayIcon()->SetTrayIcon(!isTrayEnabled);

			if (isTrayEnabled && GetRainmeter().GetAllSkins().empty())
			{
				GetRainmeter().GetTrayIcon()->SetTrayIcon(true, true);
			}
		}
		break;

	default:
		return 1;
	}

	return 0;
}
