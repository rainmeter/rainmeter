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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "StdAfx.h"
#include "Rainmeter.h"
#include "System.h"
#include "MeterWindow.h"
#include "Measure.h"
#include "resource.h"
#include "DialogManage.h"
#include "../Version.h"

#define WM_DELAYED_CLOSE WM_APP + 0

extern CRainmeter* Rainmeter;

WINDOWPLACEMENT CDialogManage::c_WindowPlacement = {0};
CDialogManage* CDialogManage::c_Dialog = NULL;

/*
** CDialogManage
**
** Constructor.
**
*/
CDialogManage::CDialogManage(HWND wnd) : CDialog(wnd),
	m_TabSkins(),
	m_TabThemes(),
	m_TabSettings()
{
}

/*
** ~CDialogManage
**
** Destructor.
**
*/
CDialogManage::~CDialogManage()
{
	delete m_TabSkins;
	delete m_TabThemes;
	delete m_TabSettings;
}

/*
** Open
**
** Opens the Manage dialog.
**
*/
void CDialogManage::Open(int tab)
{
	if (!c_Dialog)
	{
		HINSTANCE instance = Rainmeter->GetResourceInstance();
		HWND owner = Rainmeter->GetTrayWindow()->GetWindow();
		if (!CreateDialog(instance, MAKEINTRESOURCE(IDD_MANAGE_DIALOG), owner, DlgProc)) return;
	}
	else
	{
		if (!IsZoomed(c_Dialog->m_Window))
		{
			ShowWindow(c_Dialog->m_Window, SW_SHOWNORMAL);
		}
	}

	SetForegroundWindow(c_Dialog->m_Window);

	// Fake WM_NOTIFY to change tab
	NMHDR nm;
	nm.code = TCN_SELCHANGE;
	nm.idFrom = IDC_MANAGE_TAB;
	nm.hwndFrom = GetDlgItem(c_Dialog->m_Window, IDC_MANAGE_TAB);
	TabCtrl_SetCurSel(nm.hwndFrom, tab);
	c_Dialog->OnNotify(0, (LPARAM)&nm);
}

/*
** OpenSkin
**
** Opens the Manage dialog Skins tab with skin selected.
**
*/
void CDialogManage::OpenSkin(CMeterWindow* meterWindow)
{
	Open();

	if (c_Dialog && c_Dialog->m_TabSkins)
	{
		std::wstring name = meterWindow->GetSkinName() + L"\\";
		name += meterWindow->GetSkinIniFile();

		HWND item = GetDlgItem(c_Dialog->m_TabSkins->GetWindow(), IDC_MANAGESKINS_SKINS_TREEVIEW);
		c_Dialog->m_TabSkins->SelectTreeItem(item, TreeView_GetRoot(item), name.c_str());
	}
}

/*
** UpdateSkins
**
** Updates Skins tab.
**
*/
void CDialogManage::UpdateSkins(CMeterWindow* meterWindow, bool deleted)
{
	if (c_Dialog && c_Dialog->m_TabSkins && c_Dialog->m_TabSkins->IsInitialized())
	{
		c_Dialog->m_TabSkins->Update(meterWindow, deleted);
	}
}

std::wstring GetTreeSelectionPath(HWND tree)
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
		path.insert(0, L"\\");
		path.insert(0, buffer);
	}

	return path;
}

/*
** DlgProc
**
** Dialog procedure for the Manage dialog.
**
*/
INT_PTR CALLBACK CDialogManage::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!c_Dialog)
	{
		if (uMsg == WM_INITDIALOG)
		{
			c_Dialog = new CDialogManage(hWnd);
			return c_Dialog->OnInitDialog(wParam, lParam);
		}
	}
	else
	{
		switch (uMsg)
		{
		case WM_COMMAND:
			return c_Dialog->OnCommand(wParam, lParam);

		case WM_NOTIFY:
			return c_Dialog->OnNotify(wParam, lParam);

		case WM_CLOSE:
			PostMessage(hWnd, WM_DELAYED_CLOSE, 0, 0);
			return TRUE;

		case WM_DESTROY:
			delete c_Dialog;
			c_Dialog = NULL;
			return FALSE;

		case WM_DELAYED_CLOSE:
			GetWindowPlacement(hWnd, &c_WindowPlacement);
			if (c_WindowPlacement.showCmd == SW_SHOWMINIMIZED)
			{
				c_WindowPlacement.showCmd = SW_SHOWNORMAL;
			}
			DestroyWindow(hWnd);
			return TRUE;
		}
	}

	return FALSE;
}

INT_PTR CDialogManage::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	HICON hIcon = LoadIcon(Rainmeter->GetInstance(), MAKEINTRESOURCE(IDI_TRAY));
	SendMessage(m_Window, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	std::wstring tmpSz;
	HWND item = GetDlgItem(m_Window, IDC_MANAGE_TAB);
	TCITEM tci = {0};
	tci.mask = TCIF_TEXT;
	tci.pszText = GetString(ID_STR_SKINS, tmpSz);
	TabCtrl_InsertItem(item, 0, &tci);
	tci.pszText = GetString(ID_STR_THEMES, tmpSz);
	TabCtrl_InsertItem(item, 1, &tci);
	tci.pszText = GetString(ID_STR_SETTINGS, tmpSz);
	TabCtrl_InsertItem(item, 2, &tci);

	HINSTANCE instance = Rainmeter->GetResourceInstance();
	m_TabSkins = new CTabSkins(CreateDialog(instance, MAKEINTRESOURCE(IDD_MANAGESKINS_DIALOG), m_Window, CTabSkins::DlgProc));
	m_TabThemes = new CTabThemes(CreateDialog(instance, MAKEINTRESOURCE(IDD_MANAGETHEMES_DIALOG), m_Window, CTabThemes::DlgProc));
	m_TabSettings = new CTabSettings(CreateDialog(instance, MAKEINTRESOURCE(IDD_MANAGESETTINGS_DIALOG), m_Window, CTabSettings::DlgProc));

	if (CSystem::GetOSPlatform() >= OSPLATFORM_VISTA)
	{
		// Use UI font (Segoe UI) on Vista+
		SetDialogFont();

		// Use arrows instead of plus/minus in the tree for Vista+
		item = GetDlgItem(m_TabSkins->GetWindow(), IDC_MANAGESKINS_SKINS_TREEVIEW);
		SetWindowTheme(item, L"explorer", NULL);
	}

	item = GetDlgItem(m_TabSkins->GetWindow(), IDC_MANAGESKINS_FILE_TEXT);
	SendMessage(item, WM_SETFONT, (WPARAM)m_FontBold, 0);

	if (c_WindowPlacement.length == 0)
	{
		c_WindowPlacement.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(m_Window, &c_WindowPlacement);
	}

	SetWindowPlacement(m_Window, &c_WindowPlacement);

	return TRUE;
}

INT_PTR CDialogManage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_REFRESHALL_BUTTON:
		Rainmeter->RefreshAll();
		break;

	case IDC_EDITSETTINGS_BUTTON:
		{
			std::wstring command = Rainmeter->GetConfigEditor() + L" \"";
			command += Rainmeter->GetIniFile();
			command += L"\"";
			LSExecute(Rainmeter->GetTrayWindow()->GetWindow(), command.c_str(), SW_SHOWNORMAL);
		}
		break;

	case IDC_OPENLOG_BUTTON:
		RainmeterAboutWide();
		break;

	case IDCLOSE:
		PostMessage(m_Window, WM_DELAYED_CLOSE, 0, 0);
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
	case IDC_MANAGE_TAB:
		if (nm->code == TCN_SELCHANGE)
		{
			CTab* tab;
			int sel = TabCtrl_GetCurSel(nm->hwndFrom);
			if (sel == 0)
			{
				tab = m_TabSkins;
			}
			else if (sel == 1)
			{
				tab = m_TabThemes;
			}
			else // if (sel == 2)
			{
				tab = m_TabSettings;
			}

			if (tab)
			{
				if (!tab->IsInitialized())
				{
					tab->Initialize();
				}
				BringWindowToTop(tab->GetWindow());
			}
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------------
//
//                                Skins tab
//
// -----------------------------------------------------------------------------------------------

/*
** CTabSkins
**
** Constructor.
**
*/
CDialogManage::CTabSkins::CTabSkins(HWND wnd) : CTab(wnd),
	m_SkinWindow(),
	m_HandleCommands(false),
	m_IgnoreUpdate(false)
{
}

/*
** Initialize
**
** Called when tab is displayed.
**
*/
void CDialogManage::CTabSkins::Initialize()
{
	m_Initialized = true;

	std::wstring tmpSz;
	GetString(ID_STR_ACTIVESKINS, tmpSz);
	if (CSystem::GetOSPlatform() >= OSPLATFORM_VISTA)
	{
		// Arrow down
		tmpSz += L" \x25BE";
	}
	HWND item = GetDlgItem(m_Window, IDC_MANAGESKINS_ACTIVESKINS_BUTTON);
	SetWindowText(item, tmpSz.c_str());

	// Load folder/.ini icons from shell32
	HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_COLOR32, 2, 10);
	HMODULE hDLL = LoadLibrary(L"shell32.dll");

	HICON hIcon = (HICON)LoadImage(hDLL, MAKEINTRESOURCE(4), IMAGE_ICON, 16, 16, LR_SHARED);
	ImageList_AddIcon(hImageList, hIcon);
	hIcon = (HICON)LoadImage(hDLL, MAKEINTRESOURCE(151), IMAGE_ICON, 16, 16, LR_SHARED); 
	ImageList_AddIcon(hImageList, hIcon);

	FreeLibrary(hDLL);

	// Apply icons and populate tree
	item = GetDlgItem(m_Window, IDC_MANAGESKINS_SKINS_TREEVIEW);
	TreeView_SetImageList(item, hImageList, TVSIL_NORMAL);
	UpdateSkins(NULL);

	// Get rid of the EDITTEXT control border
	item = GetDlgItem(m_Window, IDC_MANAGESKINS_DESCRIPTION_TEXT);
	SetWindowLong(item, GWL_EXSTYLE, GetWindowLong(item, GWL_EXSTYLE) &~ WS_EX_CLIENTEDGE);
	SetWindowPos(item, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER); 

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_ADDMETADATA_LINK);
	ShowWindow(item, SW_HIDE);

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_DISPLAYMONITOR_BUTTON);
	GetString(ID_STR_DISPLAYMONITOR, tmpSz);
	if (CSystem::GetOSPlatform() >= OSPLATFORM_VISTA)
	{
		// Arrow down
		tmpSz += L" \x25BE";
	}
	SetWindowText(item, tmpSz.c_str());

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_TRANSPARENCY_COMBOBOX);
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

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_ZPOSITION_COMBOBOX);
	ComboBox_AddString(item, GetString(ID_STR_ONDESKTOP, tmpSz));
	ComboBox_AddString(item, GetString(ID_STR_BOTTOM, tmpSz));
	ComboBox_AddString(item, GetString(ID_STR_NORMAL, tmpSz));
	ComboBox_AddString(item, GetString(ID_STR_TOPMOST, tmpSz));
	ComboBox_AddString(item, GetString(ID_STR_STAYTOPMOST, tmpSz));

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_ONHOVER_COMBOBOX);
	ComboBox_AddString(item, GetString(ID_STR_DONOTHING, tmpSz));
	ComboBox_AddString(item, GetString(ID_STR_HIDE, tmpSz));
	ComboBox_AddString(item, GetString(ID_STR_FADEIN, tmpSz));
	ComboBox_AddString(item, GetString(ID_STR_FADEOUT, tmpSz));

	m_HandleCommands = true;
}

/*
** Update
**
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
		else if (wcscmp(meterWindow->GetSkinName().c_str(), m_SkinName.c_str()) == 0 &&
				 wcscmp(meterWindow->GetSkinIniFile().c_str(), m_FileName.c_str()) == 0)
		{
			ReadSkin();
		}
	}
	else
	{
		// Populate tree
		HWND item = GetDlgItem(m_Window, IDC_MANAGESKINS_SKINS_TREEVIEW);
		TreeView_DeleteAllItems(item);

		TV_INSERTSTRUCT tvi = {0};
		tvi.hParent = NULL;
		tvi.hInsertAfter = TVI_LAST;
		tvi.item.mask= TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvi.item.iImage = tvi.item.iSelectedImage= 0;
		PopulateTree(item, tvi, Rainmeter->m_ConfigMenu);
	}
}

void CDialogManage::CTabSkins::SetControls()
{
	WCHAR buffer[64];

	HWND item = GetDlgItem(m_Window, IDC_MANAGESKINS_EDIT_BUTTON);
	EnableWindow(item, TRUE);

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_LOAD_BUTTON);
	EnableWindow(item, TRUE);

	if (m_SkinWindow)
	{
		SetWindowText(item, GetString(ID_STR_UNLOAD, buffer, 64));

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_REFRESH_BUTTON);
		EnableWindow(item, TRUE);

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_X_TEXT);
		EnableWindow(item, TRUE);
		_itow(m_SkinWindow->GetX(), buffer, 10);
		SetWindowText(item, buffer);

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_Y_TEXT);
		EnableWindow(item, TRUE);
		_itow(m_SkinWindow->GetY(), buffer, 10);
		SetWindowText(item, buffer);

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_DISPLAYMONITOR_BUTTON);
		EnableWindow(item, TRUE);

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_DRAGGABLE_CHECKBOX);
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

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_CLICKTHROUGH_CHECKBOX);
		EnableWindow(item, TRUE);
		Button_SetCheck(item, m_SkinWindow->GetClickThrough());

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_KEEPONSCREEN_CHECKBOX);
		EnableWindow(item, TRUE);
		Button_SetCheck(item, m_SkinWindow->GetKeepOnScreen());

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_SAVEPOSITION_CHECKBOX);
		EnableWindow(item, TRUE);
		Button_SetCheck(item, m_SkinWindow->GetSavePosition());

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_SNAPTOEDGES_CHECKBOX);
		EnableWindow(item, TRUE);
		Button_SetCheck(item, m_SkinWindow->GetSnapEdges());

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_TRANSPARENCY_COMBOBOX);
		EnableWindow(item, TRUE);
		int value = (int)(10 - m_SkinWindow->GetAlphaValue() / 25.5);
		value = min(9, value);
		value = max(0, value);
		ComboBox_SetCurSel(item, value);

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_ZPOSITION_COMBOBOX);
		EnableWindow(item, TRUE);
		ComboBox_SetCurSel(item, m_SkinWindow->GetWindowZPosition() + 2);

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_LOADORDER_TEXT);
		EnableWindow(item, TRUE);
		_itow(Rainmeter->GetLoadOrder(m_SkinName), buffer, 10);
		SetWindowText(item, buffer);

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_ONHOVER_COMBOBOX);
		EnableWindow(item, TRUE);
		ComboBox_SetCurSel(item, m_SkinWindow->GetWindowHide());
	}
	else
	{
		SetWindowText(item, GetString(ID_STR_LOAD, buffer, 64));
	}
}

void CDialogManage::CTabSkins::DisableControls(bool clear)
{
	HWND item = GetDlgItem(m_Window, IDC_MANAGESKINS_LOAD_BUTTON);
	WCHAR buffer[64];
	SetWindowText(item, GetString(ID_STR_LOAD, buffer, 64));

	if (clear)
	{
		EnableWindow(item, FALSE);

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_EDIT_BUTTON);
		EnableWindow(item, FALSE);
		
		item = GetDlgItem(m_Window, IDC_MANAGESKINS_FILE_TEXT);
		SetWindowText(item, L"N/A");

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_CONFIG_TEXT);
		SetWindowText(item, L"N/A");

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_AUTHOR_TEXT);
		SetWindowText(item, L"");

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_VERSION_TEXT);
		SetWindowText(item, L"");

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_LICENSE_TEXT);
		SetWindowText(item, L"");

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_DESCRIPTION_TEXT);
		SetWindowText(item, L"");
		ShowScrollBar(item, SB_VERT, FALSE);

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_ADDMETADATA_LINK);
		ShowWindow(item, SW_HIDE);
	}
	else
	{
		EnableWindow(item, TRUE);
	}

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_REFRESH_BUTTON);
	EnableWindow(item, FALSE);

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_X_TEXT);
	SetWindowText(item, L"");
	EnableWindow(item, FALSE);

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_Y_TEXT);
	SetWindowText(item, L"");
	EnableWindow(item, FALSE);

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_DISPLAYMONITOR_BUTTON);
	EnableWindow(item, FALSE);

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_DRAGGABLE_CHECKBOX);
	EnableWindow(item, FALSE);
	Button_SetCheck(item, BST_UNCHECKED);

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_CLICKTHROUGH_CHECKBOX);
	EnableWindow(item, FALSE);
	Button_SetCheck(item, BST_UNCHECKED);

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_KEEPONSCREEN_CHECKBOX);
	EnableWindow(item, FALSE);
	Button_SetCheck(item, BST_UNCHECKED);

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_SAVEPOSITION_CHECKBOX);
	EnableWindow(item, FALSE);
	Button_SetCheck(item, BST_UNCHECKED);

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_SNAPTOEDGES_CHECKBOX);
	EnableWindow(item, FALSE);
	Button_SetCheck(item, BST_UNCHECKED);

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_TRANSPARENCY_COMBOBOX);
	EnableWindow(item, FALSE);
	ComboBox_SetCurSel(item, -1);

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_ZPOSITION_COMBOBOX);
	EnableWindow(item, FALSE);
	ComboBox_SetCurSel(item, -1);

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_LOADORDER_TEXT);
	SetWindowText(item, L"");
	EnableWindow(item, FALSE);

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_ONHOVER_COMBOBOX);
	EnableWindow(item, FALSE);
	ComboBox_SetCurSel(item, -1);
}

void CDialogManage::CTabSkins::ReadSkin()
{
	HWND item = GetDlgItem(m_Window, IDC_MANAGESKINS_FILE_TEXT);
	SetWindowText(item, m_FileName.c_str());

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_CONFIG_TEXT);
	SetWindowText(item, m_SkinName.c_str());

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_EDIT_BUTTON);
	EnableWindow(item, TRUE);

	std::wstring file = Rainmeter->GetSkinPath() + m_SkinName;
	file += L"\\";
	file += m_FileName;
	m_SkinWindow = Rainmeter->GetMeterWindowByINI(file);
	if (!m_SkinWindow)
	{
		DisableControls();
	}

	SetControls();

	WCHAR* buffer = new WCHAR[MAX_LINE_LENGTH];

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_AUTHOR_TEXT);
	GetPrivateProfileString(L"Rainmeter", L"Author", NULL, buffer, MAX_LINE_LENGTH, file.c_str());
	SetWindowText(item, buffer);

	item = GetDlgItem(m_Window, IDC_MANAGESKINS_ADDMETADATA_LINK);
	if (GetPrivateProfileSection(L"Metadata", buffer, 8, file.c_str()) > 0)
	{
		ShowWindow(item, SW_HIDE);

		// Set metadata
		item = GetDlgItem(m_Window, IDC_MANAGESKINS_VERSION_TEXT);
		GetPrivateProfileString(L"Metadata", L"Version", NULL, buffer, MAX_LINE_LENGTH, file.c_str());
		SetWindowText(item, buffer);

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_LICENSE_TEXT);
		GetPrivateProfileString(L"Metadata", L"License", NULL, buffer, MAX_LINE_LENGTH, file.c_str());
		SetWindowText(item, buffer);

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_DESCRIPTION_TEXT);
		std::wstring text;
		if (GetPrivateProfileString(L"Metadata", L"Information", NULL, buffer, MAX_LINE_LENGTH, file.c_str()) > 0)
		{
			text = buffer;
		}
		else
		{
			// For backwards compatibility
			GetPrivateProfileString(L"Metadata", L"Description", NULL, buffer, MAX_LINE_LENGTH, file.c_str());
			text = buffer;

			if (GetPrivateProfileString(L"Metadata", L"Instructions", NULL, buffer, MAX_LINE_LENGTH, file.c_str()) > 0)
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

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_VERSION_TEXT);
		SetWindowText(item, L"");

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_LICENSE_TEXT);
		SetWindowText(item, L"");

		item = GetDlgItem(m_Window, IDC_MANAGESKINS_DESCRIPTION_TEXT);
		SetWindowText(item, L"");
		ShowScrollBar(item, SB_VERT, FALSE);
	}

	delete [] buffer;
}

/*
** PopulateTree
**
** Populates the treeview with folders and skins.
**
*/
void CDialogManage::CTabSkins::PopulateTree(HWND tree, TV_INSERTSTRUCT& tvi, const std::vector<CRainmeter::CONFIGMENU>& configMenuData)
{
	if (!configMenuData.empty())
	{
		for (int i = 0, isize = (int)configMenuData.size(); i < isize; ++i)
		{
			if (configMenuData[i].index == -1)
			{
				tvi.item.iImage = tvi.item.iSelectedImage = 0;
				tvi.item.pszText = const_cast<WCHAR*>(configMenuData[i].name.c_str());
				HTREEITEM hOldParent = tvi.hParent;
				tvi.hParent = (HTREEITEM)SendMessage(tree, TVM_INSERTITEM, 0, (LPARAM)&tvi);
				PopulateTree(tree, tvi, configMenuData[i].children);
				tvi.hParent = hOldParent;
			}
			else
			{
				tvi.item.iImage = tvi.item.iSelectedImage = 1;
				tvi.item.pszText = const_cast<WCHAR*>(configMenuData[i].name.c_str());
				SendMessage(tree, TVM_INSERTITEM, 0, (LPARAM)&tvi);
			}
		}
	}
}

/*
** SelectTreeItem
**
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
	tvi.cchTextMax = MAX_PATH;

	std::wstring tmpSz = name;
	std::wstring::size_type pos = tmpSz.find_first_of(L'\\');
	if (pos != std::wstring::npos)
	{
		tmpSz.resize(pos);

		// Find and expand the folder
		do
		{
			TreeView_GetItem(tree, &tvi);
			if (wcscmp(buffer, tmpSz.c_str()) == 0)
			{
				if ((item = TreeView_GetChild(tree, tvi.hItem)) != NULL)
				{
					TreeView_Expand(tree, tvi.hItem, TVE_EXPAND);
					name += tmpSz.length() + 1;
					SelectTreeItem(tree, item, name);
				}

				break;
			}
		}
		while ((tvi.hItem = TreeView_GetNextSibling(tree, tvi.hItem)) != NULL);
	}
	else
	{
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

/*
** DlgProc
**
** Dialog procedure for the Skins tab.
**
*/
INT_PTR CALLBACK CDialogManage::CTabSkins::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return c_Dialog->m_TabSkins->OnCommand(wParam, lParam);

	case WM_NOTIFY:
		return c_Dialog->m_TabSkins->OnNotify(wParam, lParam);
	}

	return FALSE;
}

INT_PTR CDialogManage::CTabSkins::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (!m_HandleCommands)
	{
		// Values are being changed/reset, no need to apply changes
		return FALSE;
	}

	switch (LOWORD(wParam))
	{
	case IDC_MANAGESKINS_ACTIVESKINS_BUTTON:
		{
			HMENU menu = CreatePopupMenu();

			// Add active skins to menu
			std::map<std::wstring, CMeterWindow*>::const_iterator iter = Rainmeter->GetAllMeterWindows().begin();
			int index = 0;
			for ( ; iter != Rainmeter->GetAllMeterWindows().end(); ++iter)
			{
				std::wstring name = ((*iter).second)->GetSkinName() + L"\\";
				name += ((*iter).second)->GetSkinIniFile();
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
					r.left,
					--r.bottom,
					0,
					m_Window,
					NULL
				);
			}

			DestroyMenu(menu);
		}
		break;

	case IDC_MANAGESKINS_LOAD_BUTTON:
		{
			if (!m_SkinWindow)
			{
				// Skin not active, load
				std::pair<int, int> indexes = Rainmeter->GetMeterWindowIndex(m_SkinName, m_FileName);
				if (indexes.first != -1 && indexes.second != -1)
				{
					Rainmeter->ActivateConfig(indexes.first, indexes.second);

					// Fake selection change to update controls
					NMHDR nm;
					nm.code = TVN_SELCHANGED;
					nm.idFrom = IDC_MANAGESKINS_SKINS_TREEVIEW;
					nm.hwndFrom = GetDlgItem(m_Window, IDC_MANAGESKINS_SKINS_TREEVIEW);
					OnNotify(0, (LPARAM)&nm);
				}
			}
			else
			{
				m_HandleCommands = false;
				Rainmeter->DeactivateConfig(m_SkinWindow, -1);
			}
		}
		break;

	case IDC_MANAGESKINS_REFRESH_BUTTON:
		m_SkinWindow->Refresh(false);
		break;

	case IDC_MANAGESKINS_EDIT_BUTTON:
		{
			std::wstring command = Rainmeter->GetConfigEditor() + L" \"";
			command += Rainmeter->GetSkinPath();
			command += m_SkinName;
			command += L"\\";
			command += m_FileName;
			command += L"\"";

			// If the skins are in the program folder start the editor as admin
			if (Rainmeter->GetPath() + L"Skins\\" == Rainmeter->GetSkinPath())
			{
				LSExecuteAsAdmin(NULL, command.c_str(), SW_SHOWNORMAL);
			}
			else
			{
				LSExecute(NULL, command.c_str(), SW_SHOWNORMAL);
			}
		}
		break;

	case IDC_MANAGESKINS_X_TEXT:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			WCHAR buffer[32];
			m_IgnoreUpdate = true;
			int x = (GetWindowText((HWND)lParam, buffer, 32) > 0) ? _wtoi(buffer) : 0;
			m_SkinWindow->MoveWindow(x, m_SkinWindow->GetY());

			if (x > m_SkinWindow->GetX())
			{
				_itow(m_SkinWindow->GetX(), buffer, 10);
				Edit_SetText((HWND)lParam, buffer);
			}
		}
		break;

	case IDC_MANAGESKINS_Y_TEXT:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			WCHAR buffer[32];
			m_IgnoreUpdate = true;
			int y = (GetWindowText((HWND)lParam, buffer, 32) > 0) ? _wtoi(buffer) : 0;
			m_SkinWindow->MoveWindow(m_SkinWindow->GetX(), y);

			if (y > m_SkinWindow->GetY())
			{
				_itow(m_SkinWindow->GetY(), buffer, 10);
				Edit_SetText((HWND)lParam, buffer);
			}
		}
		break;

	case IDC_MANAGESKINS_LOADORDER_TEXT:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			if (m_IgnoreUpdate)
			{
				// To avoid infinite loop after setting value below
				m_IgnoreUpdate = false;
			}
			else
			{
				m_IgnoreUpdate = true;

				// Convert text to number and set it to get rid of extra chars
				int value = GetDlgItemInt(m_Window, IDC_MANAGESKINS_LOADORDER_TEXT, NULL, TRUE);
				SetDlgItemInt(m_Window, IDC_MANAGESKINS_LOADORDER_TEXT, value, TRUE);

				// Move caret to end
				Edit_SetSel((HWND)lParam, 32, 32);

				WCHAR buffer[32];
				_itow(value, buffer, 10);

				WritePrivateProfileString(m_SkinName.c_str(), L"LoadOrder", buffer, Rainmeter->GetIniFile().c_str());
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

	case IDC_MANAGESKINS_DISPLAYMONITOR_BUTTON:
		{
			HMENU menu = LoadMenu(Rainmeter->GetResourceInstance(), MAKEINTRESOURCE(IDR_SKIN_MENU));
			if (menu)
			{
				HMENU subMenu = GetSubMenu(menu, 0);	// Skin menu
				subMenu = GetSubMenu(subMenu, 4); // Settings menu
				subMenu = GetSubMenu(subMenu, 0); // Position menu
				subMenu = GetSubMenu(subMenu, 0); // Display monitor menu
				Rainmeter->CreateMonitorMenu(subMenu, m_SkinWindow);

				RECT r;
				GetWindowRect((HWND)lParam, &r);

				// Show context menu
				TrackPopupMenu(
					subMenu,
					TPM_RIGHTBUTTON | TPM_LEFTALIGN,
					r.left,
					--r.bottom,
					0,
					m_Window,
					NULL
				);

				DestroyMenu(menu);
			}
		}
		break;

	case IDC_MANAGESKINS_DRAGGABLE_CHECKBOX:
		m_IgnoreUpdate = true;
		m_SkinWindow->SetWindowDraggable(!m_SkinWindow->GetWindowDraggable());
		break;

	case IDC_MANAGESKINS_CLICKTHROUGH_CHECKBOX:
		m_IgnoreUpdate = true;
		m_SkinWindow->SetClickThrough(!m_SkinWindow->GetClickThrough());
		break;

	case IDC_MANAGESKINS_KEEPONSCREEN_CHECKBOX:
		m_IgnoreUpdate = true;
		m_SkinWindow->SetKeepOnScreen(!m_SkinWindow->GetKeepOnScreen());
		break;

	case IDC_MANAGESKINS_SAVEPOSITION_CHECKBOX:
		m_IgnoreUpdate = true;
		m_SkinWindow->SetSavePosition(!m_SkinWindow->GetSavePosition());
		break;

	case IDC_MANAGESKINS_SNAPTOEDGES_CHECKBOX:
		m_IgnoreUpdate = true;
		m_SkinWindow->SetSnapEdges(!m_SkinWindow->GetSnapEdges());
		break;

	case IDC_MANAGESKINS_ZPOSITION_COMBOBOX:
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			m_IgnoreUpdate = true;
			ZPOSITION zpos = (ZPOSITION)(ComboBox_GetCurSel((HWND)lParam) - 2);
			m_SkinWindow->ChangeZPos(zpos);
		}
		break;

	case IDC_MANAGESKINS_TRANSPARENCY_COMBOBOX:
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			m_IgnoreUpdate = true;
			int sel = ComboBox_GetCurSel((HWND)lParam) + ID_CONTEXT_SKINMENU_TRANSPARENCY_0;
			SendMessage(m_SkinWindow->GetWindow(), WM_COMMAND, sel, 0);
		}
		break;

	case IDC_MANAGESKINS_ONHOVER_COMBOBOX:
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			m_IgnoreUpdate = true;
			HIDEMODE hide = (HIDEMODE)ComboBox_GetCurSel((HWND)lParam);
			m_SkinWindow->SetWindowHide(hide);
		}
		break;

	case ID_CONTEXT_MANAGESKINSMENU_EXPAND:
		{
			HWND tree = GetDlgItem(m_Window, IDC_MANAGESKINS_SKINS_TREEVIEW);
			HTREEITEM item = TreeView_GetSelection(tree);
			TreeView_Expand(tree, item, TVE_TOGGLE);
		}
		break;

	case ID_CONTEXT_MANAGESKINSMENU_OPENFOLDER:
		{
			HWND tree = GetDlgItem(m_Window, IDC_MANAGESKINS_SKINS_TREEVIEW);
			std::wstring command = L"\"" + Rainmeter->GetSkinPath();
			command += GetTreeSelectionPath(tree);
			command += L"\"";
			LSExecute(NULL, command.c_str(), SW_SHOWNORMAL);
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
					std::wstring name = ((*iter).second)->GetSkinName() + L"\\";
					name += ((*iter).second)->GetSkinIniFile();

					HWND item = GetDlgItem(m_Window, IDC_MANAGESKINS_SKINS_TREEVIEW);
					SelectTreeItem(item, TreeView_GetRoot(item), name.c_str());
					break;
				}

				++i;
			}
		}
		else if (wParam == ID_CONTEXT_SKINMENU_MONITOR_AUTOSELECT ||
			wParam == ID_CONTEXT_SKINMENU_MONITOR_PRIMARY ||
			wParam >= ID_MONITOR_FIRST && wParam <= ID_MONITOR_LAST)
		{
			SendMessage(m_SkinWindow->GetWindow(), WM_COMMAND, wParam, 0);
			break;
		}

		return FALSE;
	}

	return TRUE;
}

INT_PTR CDialogManage::CTabSkins::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR nm = (LPNMHDR)lParam;
	switch (nm->code)
	{
	case NM_CLICK:
		if (nm->idFrom == IDC_MANAGESKINS_ADDMETADATA_LINK)
		{
			std::wstring file = Rainmeter->GetSkinPath() + m_SkinName;
			file += L"\\";
			file += m_FileName;
			WritePrivateProfileString(L"Rainmeter", L"\r\n[Metadata]\r\nName=\r\nInformation=\r\nLicense=\r\nVersion", L"", file.c_str());
			SendMessage(m_Window, WM_COMMAND, MAKEWPARAM(IDC_MANAGESKINS_EDIT_BUTTON, 0), 0);
			ShowWindow(nm->hwndFrom, SW_HIDE);
		}
		break;

	case NM_DBLCLK:
		if (nm->idFrom == IDC_MANAGESKINS_SKINS_TREEVIEW && !m_FileName.empty())
		{
			OnCommand(MAKEWPARAM(IDC_MANAGESKINS_LOAD_BUTTON, 0), 0);
		}
		break;

	case NM_RCLICK:
		if (nm->idFrom == IDC_MANAGESKINS_SKINS_TREEVIEW)
		{
			POINT pt;
			GetCursorPos(&pt);

			TVHITTESTINFO ht;
			ht.pt = pt;
			ScreenToClient(nm->hwndFrom, &ht.pt);

			if (TreeView_HitTest(nm->hwndFrom, &ht) && !(ht.flags & TVHT_NOWHERE))
			{
				TreeView_SelectItem(nm->hwndFrom, ht.hItem);

				TVITEM tvi = {0};
				tvi.hItem = TreeView_GetSelection(nm->hwndFrom);
				tvi.mask = TVIF_STATE;

				HMENU menu = LoadMenu(Rainmeter->GetResourceInstance(), MAKEINTRESOURCE(IDR_MANAGESKINS_MENU));
				if (menu && TreeView_GetItem(nm->hwndFrom, &tvi))
				{
					std::wstring tmpSz;
					HMENU subMenu;
					MENUITEMINFO mii = {0};
					mii.cbSize = sizeof(MENUITEMINFO);
					mii.fMask = MIIM_STRING;

					if (m_FileName.empty())
					{
						// It's a folder
						subMenu = GetSubMenu(menu, 0);
						SetMenuDefaultItem(subMenu, ID_CONTEXT_MANAGESKINSMENU_EXPAND, MF_BYCOMMAND);

						if (tvi.state & TVIS_EXPANDED)
						{
							mii.dwTypeData = GetString(ID_STR_COLLAPSE, tmpSz);
							SetMenuItemInfo(subMenu, ID_CONTEXT_MANAGESKINSMENU_EXPAND, MF_BYCOMMAND, &mii);
						}
					}
					else
					{
						// It's a skin
						subMenu = GetSubMenu(menu, 1);
						SetMenuDefaultItem(subMenu, ID_CONTEXT_MANAGESKINSMENU_LOAD, MF_BYCOMMAND);

						if (m_SkinWindow)
						{
							mii.dwTypeData = GetString(ID_STR_UNLOAD, tmpSz);
							SetMenuItemInfo(subMenu, ID_CONTEXT_MANAGESKINSMENU_LOAD, MF_BYCOMMAND, &mii);
						}
						else
						{
							EnableMenuItem(subMenu, ID_CONTEXT_MANAGESKINSMENU_REFRESH, MF_BYCOMMAND | MF_GRAYED);
						}
					}

					// Show context menu
					TrackPopupMenu(
						subMenu,
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
		if (nm->idFrom == IDC_MANAGESKINS_SKINS_TREEVIEW)
		{
			m_SkinWindow = NULL;
			m_FileName.clear();
			m_SkinName.clear();

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
				m_FileName = buffer;
				tvi.mask = TVIF_TEXT;
			
				// Loop through parents to get config
				m_SkinName.clear();
				while ((tvi.hItem = TreeView_GetParent(nm->hwndFrom, tvi.hItem)) != NULL)
				{
					TreeView_GetItem(nm->hwndFrom, &tvi);
					m_SkinName.insert(0, L"\\");
					m_SkinName.insert(0, buffer);
				}

				m_SkinName.resize(m_SkinName.length() - 1);  // Get rid of trailing slash

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
//                                Themes tab
//
// -----------------------------------------------------------------------------------------------

/*
** CTabThemes
**
** Constructor.
**
*/
CDialogManage::CTabThemes::CTabThemes(HWND wnd) : CTab(wnd)
{
}

/*
** Initialize
**
** Called when tab is displayed.
**
*/
void CDialogManage::CTabThemes::Initialize()
{
	m_Initialized = true;

	HWND item  = GetDlgItem(m_Window, IDC_MANAGETHEMES_LIST);
	const std::vector<std::wstring>& themes = Rainmeter->GetAllThemes();
	for (int i = 0, isize = themes.size(); i < isize; ++i)
	{
		ListBox_AddString(item, themes[i].c_str());
	}
}

/*
** DlgProc
**
** Dialog procedure for the Themes tab.
**
*/
INT_PTR CALLBACK CDialogManage::CTabThemes::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return c_Dialog->m_TabThemes->OnCommand(wParam, lParam);
	}

	return FALSE;
}

INT_PTR CDialogManage::CTabThemes::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_MANAGETHEMES_EMPTYTHEME_CHECKBOX:
		{
			BOOL state = !(Button_GetCheck((HWND)lParam) == BST_CHECKED);

			HWND item = GetDlgItem(m_Window, IDC_MANAGETHEMES_UNUSEDSKINS_CHECKBOX);
			EnableWindow(item, state);
			Button_SetCheck(item, BST_UNCHECKED);

			item = GetDlgItem(m_Window, IDC_MANAGETHEMES_WALLPAPER_CHECKBOX);
			EnableWindow(item, state);
			Button_SetCheck(item, BST_UNCHECKED);
		}
		break;

	case IDC_MANAGETHEMES_NAME_TEXT:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			WCHAR buffer[32];
			Edit_GetText((HWND)lParam, buffer, 32);

			// Disable save button if no text or if name is "Backup"
			BOOL state = (wcslen(buffer) != 0 && _wcsicmp(buffer, L"Backup") != 0);
			EnableWindow(GetDlgItem(m_Window, IDC_MANAGETHEMES_SAVE_BUTTON), state);
		}
		break;

	case IDC_MANAGETHEMES_LIST:
		if (HIWORD(wParam) == LBN_SELCHANGE)
		{
			// Ignore clicks that don't hit items
			if (ListBox_GetCurSel((HWND)lParam) != LB_ERR)
			{
				HWND item = GetDlgItem(m_Window, IDC_MANAGETHEMES_LOAD_BUTTON);
				EnableWindow(item, TRUE);
				item = GetDlgItem(m_Window, IDC_MANAGETHEMES_DELETE_BUTTON);
				EnableWindow(item, TRUE);
				item = GetDlgItem(m_Window, IDC_MANAGETHEMES_EDIT_BUTTON);
				EnableWindow(item, TRUE);
				
				const std::vector<std::wstring>& themes = Rainmeter->GetAllThemes();
				item  = GetDlgItem(m_Window, IDC_MANAGETHEMES_LIST);
				int sel = ListBox_GetCurSel(item);
				
				item = GetDlgItem(m_Window, IDC_MANAGETHEMES_NAME_TEXT);
				Edit_SetText(item, themes[sel].c_str());
			}
		}
		break;

	case IDC_MANAGETHEMES_SAVE_BUTTON:
		{
			WCHAR buffer[MAX_PATH];
			HWND item = GetDlgItem(m_Window, IDC_MANAGETHEMES_NAME_TEXT);
			Edit_GetText(item, buffer, MAX_PATH);

			std::wstring theme = buffer;
			std::wstring path = Rainmeter->GetSettingsPath() + L"Themes\\";
			path += theme;
			bool alreadyExists = (_waccess(path.c_str(), 0) != -1);
			if (alreadyExists)
			{
				std::wstring text = GetFormattedString(ID_STR_THEMEALREADYEXISTS, theme.c_str());
				if (MessageBox(NULL, text.c_str(), APPNAME, MB_ICONWARNING | MB_YESNO | MB_TOPMOST) != IDYES)
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

			path += L"\\Rainmeter.thm";

			item = GetDlgItem(m_Window, IDC_MANAGETHEMES_EMPTYTHEME_CHECKBOX);
			if (Button_GetCheck(item) != BST_CHECKED)
			{
				if (!CSystem::CopyFiles(Rainmeter->GetIniFile(), path))
				{
					std::wstring text = GetFormattedString(ID_STR_THEMESAVEFAIL, path.c_str());
					MessageBox(NULL, text.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONERROR);
					break;
				}

				// Exclude unused skins
				item = GetDlgItem(m_Window, IDC_MANAGETHEMES_UNUSEDSKINS_CHECKBOX);
				if (Button_GetCheck(item) == BST_CHECKED)
				{
					CConfigParser parser;
					parser.Initialize(path.c_str(), Rainmeter);

					const std::vector<std::wstring>& sections = parser.GetSections();
					std::vector<std::wstring>::const_iterator iter = sections.begin();

					// Remove sections with Active=0
					for ( ; iter != sections.end(); ++iter)
					{
						if (parser.GetValue(*iter, L"Active", L"") == L"0")
						{
							WritePrivateProfileString((*iter).c_str(), NULL, NULL, path.c_str());
						}
					}
				}

				// Save wallpaper
				item = GetDlgItem(m_Window, IDC_MANAGETHEMES_WALLPAPER_CHECKBOX);
				if (Button_GetCheck(item) == BST_CHECKED)
				{
					// Get current wallpaper
					if (SystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, &buffer, 0))
					{
						std::wstring::size_type pos = path.find_last_of(L'\\');
						path.replace(pos + 1, path.length() - pos - 1, L"RainThemes.bmp");
						CSystem::CopyFiles((std::wstring)buffer, path);
					}
				}
			}
			else
			{
				// Create empty theme
				HANDLE file = CreateFile(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if (file == INVALID_HANDLE_VALUE)
				{
					std::wstring text = GetFormattedString(ID_STR_THEMESAVEFAIL, path.c_str());
					MessageBox(NULL, text.c_str(), APPNAME, MB_OK | MB_TOPMOST | MB_ICONERROR);
					break;
				}

				CloseHandle(file);
			}

			if (!alreadyExists)
			{
				item = GetDlgItem(m_Window, IDC_MANAGETHEMES_LIST);
				ListBox_AddString(item, theme.c_str());

				// Add theme to vector
				std::vector<std::wstring>& themes = Rainmeter->m_Themes;
				if (!themes.empty())
				{
					std::vector<std::wstring>::iterator iter = themes.begin();
					for ( ; iter != themes.end(); ++iter)
					{
						if (wcscmp(theme.c_str(), (*iter).c_str()) < 0)
						{
							themes.insert(iter, theme);
							return TRUE;
						}
					}
				}

				// If themes empty or if name is alphabetically after vector contents
				themes.push_back(theme);
			}
		}
		break;

	case IDC_MANAGETHEMES_LOAD_BUTTON:
		{
			HWND item  = GetDlgItem(m_Window, IDC_MANAGETHEMES_LIST);
			int sel = ListBox_GetCurSel(item);
			Rainmeter->LoadTheme(Rainmeter->m_Themes[sel]);
		}
		break;

	case IDC_MANAGETHEMES_EDIT_BUTTON:
		{
			HWND item  = GetDlgItem(m_Window, IDC_MANAGETHEMES_LIST);
			int sel = ListBox_GetCurSel(item);
			const std::vector<std::wstring>& themes = Rainmeter->GetAllThemes();

			std::wstring command = Rainmeter->GetConfigEditor() + L" \"";
			command += Rainmeter->GetSettingsPath();
			command += L"Themes\\";
			command += themes[sel];
			command += L"\\Rainmeter.thm\"";
			LSExecute(NULL, command.c_str(), SW_SHOWNORMAL);
		}
		break;

	case IDC_MANAGETHEMES_DELETE_BUTTON:
		{
			HWND item  = GetDlgItem(m_Window, IDC_MANAGETHEMES_LIST);
			int sel = ListBox_GetCurSel(item);
			std::vector<std::wstring>& themes = const_cast<std::vector<std::wstring>&>(Rainmeter->GetAllThemes());

			std::wstring text = GetFormattedString(ID_STR_THEMEDELETE, themes[sel].c_str());
			if (MessageBox(NULL, text.c_str(), APPNAME, MB_ICONQUESTION | MB_YESNO | MB_TOPMOST) != IDYES)
			{
				// Cancel
				break;
			}

			std::wstring folder = Rainmeter->GetSettingsPath() + L"Themes\\";
			folder += themes[sel];

			if (CSystem::RemoveFolder(folder))
			{
				ListBox_DeleteString(item, sel);

				// Remove theme from vector
				std::vector<std::wstring>::iterator iter = themes.begin();
				for ( ; iter != themes.end(); ++iter)
				{
					if (wcscmp(themes[sel].c_str(), (*iter).c_str()) == 0)
					{
						themes.erase(iter);
						break;
					}
				}

				EnableWindow(GetDlgItem(m_Window, IDC_MANAGETHEMES_LOAD_BUTTON), FALSE);
				EnableWindow(GetDlgItem(m_Window, IDC_MANAGETHEMES_DELETE_BUTTON), FALSE);
				EnableWindow(GetDlgItem(m_Window, IDC_MANAGETHEMES_EDIT_BUTTON), FALSE);
			}
		}
		break;

	case IDC_MANAGETHEMES_BACKUP_BUTTON:
		{
			std::wstring command = L"\"" + Rainmeter->GetAddonPath();
			command += L"RainBackup\\RainBackup.exe\"";
			LSExecute(NULL, command.c_str(), SW_SHOWNORMAL);
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------------
//
//                                Settings tab
//
// -----------------------------------------------------------------------------------------------

/*
** CTabSettings
**
** Constructor.
**
*/
CDialogManage::CTabSettings::CTabSettings(HWND wnd) : CTab(wnd)
{
}

/*
** Initialize
**
** Called when tab is displayed.
**
*/
void CDialogManage::CTabSettings::Initialize()
{
	m_Initialized = true;

	Button_SetCheck(GetDlgItem(m_Window, IDC_MANAGESETTINGS_CHECKUPDATES_CHECKBOX), !Rainmeter->GetDisableVersionCheck());
	Button_SetCheck(GetDlgItem(m_Window, IDC_MANAGESETTINGS_LOCKSKINS_CHECKBOX), Rainmeter->GetDisableDragging());
	Button_SetCheck(GetDlgItem(m_Window, IDC_MANAGESETTINGS_LOGTOFILE_CHECKBOX), Rainmeter->GetLogging());
	Button_SetCheck(GetDlgItem(m_Window, IDC_MANAGESETTINGS_VERBOSELOGGING_CHECKBOX), Rainmeter->GetDebug());

	BOOL isLogFile = (_waccess(Rainmeter->GetLogFile().c_str(), 0) != -1);
	EnableWindow(GetDlgItem(m_Window, IDC_MANAGESETTINGS_SHOWLOGFILE_BUTTON), isLogFile);
	EnableWindow(GetDlgItem(m_Window, IDC_MANAGESETTINGS_DELETELOGFILE_BUTTON), isLogFile);
}

/*
** DlgProc
**
** Dialog procedure for the Settings tab.
**
*/
INT_PTR CALLBACK CDialogManage::CTabSettings::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return c_Dialog->m_TabSettings->OnCommand(wParam, lParam);
	}

	return FALSE;
}

INT_PTR CDialogManage::CTabSettings::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_MANAGESETTINGS_CHECKUPDATES_CHECKBOX:
		Rainmeter->SetDisableVersionCheck(!Rainmeter->GetDisableVersionCheck());
		break;

	case IDC_MANAGESETTINGS_LOCKSKINS_CHECKBOX:
		Rainmeter->SetDisableDragging(!Rainmeter->GetDisableDragging());
		break;

	case IDC_MANAGESETTINGS_RESETSTATISTICS_BUTTON:
		Rainmeter->ResetStats();
		break;

	case IDC_MANAGESETTINGS_SHOWLOGFILE_BUTTON:
		{
			std::wstring command = Rainmeter->GetLogViewer() + Rainmeter->GetLogFile();
			LSExecute(NULL, command.c_str(), SW_SHOWNORMAL);
		}
		break;

	case IDC_MANAGESETTINGS_DELETELOGFILE_BUTTON:
		Rainmeter->DeleteLogFile();
		if (_waccess(Rainmeter->GetLogFile().c_str(), 0) == -1)
		{
			Button_SetCheck(GetDlgItem(m_Window, IDC_MANAGESETTINGS_LOGTOFILE_CHECKBOX), BST_UNCHECKED);
			EnableWindow(GetDlgItem(m_Window, IDC_MANAGESETTINGS_SHOWLOGFILE_BUTTON), FALSE);
			EnableWindow(GetDlgItem(m_Window, IDC_MANAGESETTINGS_DELETELOGFILE_BUTTON), FALSE);
		}
		break;

	case IDC_MANAGESETTINGS_LOGTOFILE_CHECKBOX:
		if (Rainmeter->GetLogging())
		{
			Rainmeter->StopLogging();
		}
		else
		{
			Rainmeter->StartLogging();
			if (_waccess(Rainmeter->GetLogFile().c_str(), 0) != -1)
			{
				EnableWindow(GetDlgItem(m_Window, IDC_MANAGESETTINGS_SHOWLOGFILE_BUTTON), TRUE);
				EnableWindow(GetDlgItem(m_Window, IDC_MANAGESETTINGS_DELETELOGFILE_BUTTON), TRUE);
			}
		}
		break;

	case IDC_MANAGESETTINGS_VERBOSELOGGING_CHECKBOX:
		Rainmeter->SetDebug(!Rainmeter->GetDebug());
		break;

	default:
		return FALSE;
	}

	return TRUE;
}
