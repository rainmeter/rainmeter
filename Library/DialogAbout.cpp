/*
  Copyright (C) 2011 Birunthan Mohanathas, Kimmo Pekkola

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
#include "DialogAbout.h"
#include "../Version.h"

#define WM_DELAYED_CLOSE WM_APP + 0

extern CRainmeter* Rainmeter;

WINDOWPLACEMENT CDialogAbout::c_WindowPlacement = {0};
CDialogAbout* CDialogAbout::c_Dialog = NULL;

/*
** CDialogAbout
**
** Constructor.
**
*/
CDialogAbout::CDialogAbout(HWND wnd) : CDialog(wnd),
	m_TabLog(),
	m_TabMeasures(),
	m_TabPlugins(),
	m_TabVersion()
{
}

/*
** ~CDialogAbout
**
** Destructor.
**
*/
CDialogAbout::~CDialogAbout()
{
	delete m_TabLog;
	delete m_TabMeasures;
	delete m_TabPlugins;
	delete m_TabVersion;
}

/*
** Open
**
** Opens the About dialog.
**
*/
void CDialogAbout::Open(int tab)
{
	if (!c_Dialog)
	{
		HINSTANCE instance = Rainmeter->GetInstance();
		HWND owner = Rainmeter->GetTrayWindow()->GetWindow();
		if (!CreateDialog(instance, MAKEINTRESOURCE(IDD_ABOUT_DIALOG), owner, DlgProc)) return;
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
	nm.idFrom = IDC_ABOUT_TAB;
	nm.hwndFrom = GetDlgItem(c_Dialog->m_Window, IDC_ABOUT_TAB);
	TabCtrl_SetCurSel(nm.hwndFrom, tab);
	c_Dialog->OnNotify(0, (LPARAM)&nm);
}

void CDialogAbout::AddLogItem(int level, LPCWSTR time, LPCWSTR message)
{
	if (c_Dialog && c_Dialog->m_TabLog && c_Dialog->m_TabLog->IsInitialized())
	{
		c_Dialog->m_TabLog->AddItem(level, time, message);
	}
}

void CDialogAbout::UpdateSkins()
{
	if (c_Dialog && c_Dialog->m_TabMeasures && c_Dialog->m_TabMeasures->IsInitialized())
	{
		c_Dialog->m_TabMeasures->UpdateSkinList();
	}
}

void CDialogAbout::UpdateMeasures(LPCTSTR entryName)
{
	if (c_Dialog && c_Dialog->m_TabMeasures && c_Dialog->m_TabMeasures->IsInitialized())
	{
		c_Dialog->m_TabMeasures->UpdateMeasureList(entryName);
	}
}

/*
** DlgProc
**
** Dialog procedure for the About dialog.
**
*/
INT_PTR CALLBACK CDialogAbout::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!c_Dialog)
	{
		if (uMsg == WM_INITDIALOG)
		{
			c_Dialog = new CDialogAbout(hWnd);
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

		case WM_GETMINMAXINFO:
			{
				MINMAXINFO* mmi = (MINMAXINFO*)lParam;
				mmi->ptMinTrackSize.x = 550;
				mmi->ptMinTrackSize.y = 350;
			}
			return TRUE;

		case WM_SIZE:
			{
				if (wParam != SIZE_MINIMIZED)
				{
					int w = LOWORD(lParam);
					int h = HIWORD(lParam);
					RECT r;

					HWND item = GetDlgItem(hWnd, IDC_ABOUT_TAB);
					SetWindowPos(item, NULL, 0, 0, w - 18, h - 47, SWP_NOMOVE | SWP_NOZORDER);

					item = GetDlgItem(hWnd, IDCLOSE);
					GetClientRect(item, &r);
					SetWindowPos(item, NULL, w - r.right - 9, h - r.bottom - 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

					w -= 48;
					h -= 100;
					c_Dialog->m_TabLog->Resize(w, h);
					c_Dialog->m_TabMeasures->Resize(w, h);
					c_Dialog->m_TabPlugins->Resize(w, h);
					c_Dialog->m_TabVersion->Resize(w, h);
				}
			}
			return TRUE;

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

INT_PTR CDialogAbout::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	HICON hIcon = LoadIcon(Rainmeter->GetInstance(), MAKEINTRESOURCE(IDI_TRAY));
	SendMessage(m_Window, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	HWND item = GetDlgItem(m_Window, IDC_ABOUT_TAB);
	TCITEM tci = {0};
	tci.mask = TCIF_TEXT;
	tci.pszText = L"Log";
	TabCtrl_InsertItem(item, 0, &tci);
	tci.pszText = L"Measures";
	TabCtrl_InsertItem(item, 1, &tci);
	tci.pszText = L"Plugins";
	TabCtrl_InsertItem(item, 2, &tci);
	tci.pszText = L"Version";
	TabCtrl_InsertItem(item, 3, &tci);

	HINSTANCE instance = Rainmeter->GetInstance();
	m_TabLog = new CTabLog(CreateDialog(instance, MAKEINTRESOURCE(IDD_ABOUTLOG_DIALOG), m_Window, CTabLog::DlgProc));
	m_TabMeasures = new CTabMeasures(CreateDialog(instance, MAKEINTRESOURCE(IDD_ABOUTMEASURES_DIALOG), m_Window, CTabMeasures::DlgProc));
	m_TabPlugins = new CTabPlugins(CreateDialog(instance, MAKEINTRESOURCE(IDD_ABOUTPLUGINS_DIALOG), m_Window, CTabPlugins::DlgProc));
	m_TabVersion = new CTabVersion(CreateDialog(instance, MAKEINTRESOURCE(IDD_ABOUTVERSION_DIALOG), m_Window, CTabVersion::DlgProc));

	if (CSystem::GetOSPlatform() >= OSPLATFORM_VISTA)
	{
		// Use UI font (Segoe UI) on Vista+
		SetDialogFont();
	}

	if (c_WindowPlacement.length == 0)
	{
		c_WindowPlacement.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(m_Window, &c_WindowPlacement);
	}
	else
	{
		SetWindowPlacement(m_Window, &c_WindowPlacement);
	}

	return TRUE;
}

INT_PTR CDialogAbout::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDCLOSE:
		PostMessage(m_Window, WM_DELAYED_CLOSE, 0, 0);
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

INT_PTR CDialogAbout::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR nm = (LPNMHDR)lParam;
	switch (nm->idFrom)
	{
	case IDC_ABOUT_TAB:
		if (nm->code == TCN_SELCHANGE)
		{
			CTab* tab;
			int sel = TabCtrl_GetCurSel(nm->hwndFrom);
			if (sel == 0)
			{
				tab = m_TabLog;
			}
			else if (sel == 1)
			{
				tab = m_TabMeasures;
			}
			else if (sel == 2)
			{
				tab = m_TabPlugins;
			}
			else // if (sel == 3)
			{
				tab = m_TabVersion;
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
//                                Log tab
//
// -----------------------------------------------------------------------------------------------

/*
** CTabLog
**
** Constructor.
**
*/
CDialogAbout::CTabLog::CTabLog(HWND wnd) : CTab(wnd),
	m_Error(true),
	m_Warning(true),
	m_Notice(true),
	m_Debug(true)
{
}

/*
** Initialize
**
** Called when tab is displayed.
**
*/
void CDialogAbout::CTabLog::Initialize()
{
	m_Initialized = true;

	// Add columns to the list view	
	HWND item = GetDlgItem(m_Window, IDC_ABOUTLOG_ITEMS_LISTVIEW);

	ListView_SetExtendedListViewStyleEx(item, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	// Set folder/.ini icons for tree list
	HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_COLOR32, 2, 10);
	HMODULE hDLL = LoadLibrary(L"user32.dll");

	HICON hIcon = (HICON)LoadImage(hDLL, MAKEINTRESOURCE(103), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	ImageList_AddIcon(hImageList, hIcon);
	DeleteObject(hIcon);

	hIcon = (HICON)LoadImage(hDLL, MAKEINTRESOURCE(101), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	ImageList_AddIcon(hImageList, hIcon);
	DeleteObject(hIcon);

	hIcon = (HICON)LoadImage(hDLL, MAKEINTRESOURCE(104), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR); 
	ImageList_AddIcon(hImageList, hIcon);
	DeleteObject(hIcon);

	FreeLibrary(hDLL);

	ListView_SetImageList(item, (WPARAM)hImageList, LVSIL_SMALL);

	LVCOLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;  // left-aligned column
	lvc.iSubItem = 0;
	lvc.pszText = L"Type";
	lvc.cx = 75;
	ListView_InsertColumn(item, 0, &lvc);
	lvc.iSubItem = 1;
	lvc.cx = 85;
	lvc.pszText = L"Time";
	ListView_InsertColumn(item, 1, &lvc);
	lvc.iSubItem = 2;
	lvc.cx = 370;
	lvc.pszText = L"Message";
	ListView_InsertColumn(item, 2, &lvc);

	// Add stored entires
	std::list<CRainmeter::LOG_INFO>::const_iterator iter = Rainmeter->GetAboutLogData().begin();
	for ( ; iter != Rainmeter->GetAboutLogData().end(); ++iter)
	{
		AddItem((*iter).level, (*iter).timestamp.c_str(), (*iter).message.c_str());
	}

	item = GetDlgItem(m_Window, IDC_ABOUTLOG_ERROR_CHECKBOX);
	Button_SetCheck(item, BST_CHECKED);

	item = GetDlgItem(m_Window, IDC_ABOUTLOG_WARNING_CHECKBOX);
	Button_SetCheck(item, BST_CHECKED);

	item = GetDlgItem(m_Window, IDC_ABOUTLOG_NOTICE_CHECKBOX);
	Button_SetCheck(item, BST_CHECKED);

	item = GetDlgItem(m_Window, IDC_ABOUTLOG_DEBUG_CHECKBOX);
	Button_SetCheck(item, BST_CHECKED);
}

/*
** Resize
**
** Resizes window and repositions controls.
**
*/
void CDialogAbout::CTabLog::Resize(int w, int h)
{
	SetWindowPos(m_Window, NULL, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);

	HWND item = GetDlgItem(m_Window, IDC_ABOUTLOG_ITEMS_LISTVIEW);
	SetWindowPos(item, NULL, 0, 0, w, h - 22, SWP_NOMOVE | SWP_NOZORDER);

	// Adjust third colum
	LVCOLUMN lvc;
	lvc.mask = LVCF_WIDTH;
	lvc.cx = w - 183;
	ListView_SetColumn(item, 2, &lvc);

	item = GetDlgItem(m_Window, IDC_ABOUTLOG_ERROR_CHECKBOX);
	SetWindowPos(item, NULL, 0, h - 15, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	item = GetDlgItem(m_Window, IDC_ABOUTLOG_WARNING_CHECKBOX);
	SetWindowPos(item, NULL, 75, h - 15, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	item = GetDlgItem(m_Window, IDC_ABOUTLOG_NOTICE_CHECKBOX);
	SetWindowPos(item, NULL, 150, h - 15, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	item = GetDlgItem(m_Window, IDC_ABOUTLOG_DEBUG_CHECKBOX);
	SetWindowPos(item, NULL, 225, h - 15, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

/*
** AddItem
**
** Adds item to log.
**
*/
void CDialogAbout::CTabLog::AddItem(int level, LPCWSTR time, LPCWSTR message)
{
	LVITEM vitem;
	vitem.mask = LVIF_IMAGE | LVIF_TEXT;
	vitem.iItem = 0;
	vitem.iSubItem = 0;

	switch (level)
	{
	case LOG_ERROR:
		if (!m_Error) return;
		vitem.pszText = L"Error";
		vitem.iImage = 0;
		break;

	case LOG_WARNING:
		if (!m_Error) return;
		vitem.pszText = L"Warning";
		vitem.iImage = 1;
		break;

	case LOG_NOTICE:
		if (!m_Notice) return;
		vitem.pszText = L"Notice";
		vitem.iImage = 2;
		break;

	case LOG_DEBUG:
		if (!m_Debug) return;
		vitem.pszText = L"Debug";
		vitem.iImage = -1;
		break;
	}
	
	HWND item = GetDlgItem(m_Window, IDC_ABOUTLOG_ITEMS_LISTVIEW);
	ListView_InsertItem(item, &vitem);
	ListView_SetItemText(item, vitem.iItem, 1, (WCHAR*)time);
	ListView_SetItemText(item, vitem.iItem, 2, (WCHAR*)message);
	if (!ListView_IsItemVisible(item, 0))
	{
		ListView_Scroll(item, 0, 16);
	}
}

/*
** DlgProc
**
** Dialog procedure for the log dialog.
**
*/
INT_PTR CALLBACK CDialogAbout::CTabLog::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return c_Dialog->m_TabLog->OnCommand(wParam, lParam);
	}

	return FALSE;
}

INT_PTR CDialogAbout::CTabLog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_ABOUTLOG_ERROR_CHECKBOX:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			m_Error = !m_Error;
		}
		break;

	case IDC_ABOUTLOG_WARNING_CHECKBOX:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			m_Warning = !m_Warning;
		}
		break;

	case IDC_ABOUTLOG_NOTICE_CHECKBOX:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			m_Notice = !m_Notice;
		}
		break;

	case IDC_ABOUTLOG_DEBUG_CHECKBOX:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			m_Debug = !m_Debug;
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------------
//
//                                Measures tab
//
// -----------------------------------------------------------------------------------------------

/*
** CTabMeasures
**
** Constructor.
**
*/
CDialogAbout::CTabMeasures::CTabMeasures(HWND wnd) : CTab(wnd)
{
}

/*
** Initialize
**
** Called when tab is displayed.
**
*/
void CDialogAbout::CTabMeasures::Initialize()
{
	m_Initialized = true;

	// Add columns to the list view	
	HWND item = GetDlgItem(m_Window, IDC_ABOUTMEASURES_ITEMS_LISTVIEW);
	ListView_SetExtendedListViewStyleEx(item, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	LVCOLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.iSubItem = 0;
	lvc.pszText = L"Name";
	lvc.cx = 120;
	lvc.fmt = LVCFMT_LEFT;  // left-aligned column
	ListView_InsertColumn(item, 0, &lvc);
	lvc.iSubItem = 1;
	lvc.cx = 90;
	lvc.pszText = L"Range";
	ListView_InsertColumn(item, 1, &lvc);
	lvc.iSubItem = 2;
	lvc.cx = 130;
	lvc.pszText = L"Value";
	ListView_InsertColumn(item, 2, &lvc);

	// Add entries for each config
	item = GetDlgItem(m_Window, IDC_ABOUTMEASURES_ITEMS_LISTBOX);
	const std::map<std::wstring, CMeterWindow*>& windows = Rainmeter->GetAllMeterWindows();
	std::map<std::wstring, CMeterWindow*>::const_iterator iter = windows.begin();
	for ( ; iter != windows.end(); ++iter)
	{
		CMeterWindow* meterWindow = (*iter).second;
		const std::wstring& skinName = meterWindow->GetSkinName();
		SendMessage(item, LB_ADDSTRING, NULL, (LPARAM)skinName.c_str());
		size_t namelength = skinName.length();

		int currwidth = (int)SendMessage(item, LB_GETHORIZONTALEXTENT, NULL, NULL);
		if (6 * (int)namelength > currwidth)
		{
			SendMessage(item, LB_SETHORIZONTALEXTENT, 6 * namelength, NULL);
		}
	}

	// Select first item
	SendMessage(item, LB_SETCURSEL, 0, 0);
}

/*
** Resize
**
** Resizes window and repositions controls.
**
*/
void CDialogAbout::CTabMeasures::Resize(int w, int h)
{
	SetWindowPos(m_Window, NULL, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);

	HWND item = GetDlgItem(m_Window, IDC_ABOUTMEASURES_ITEMS_LISTBOX);
	int wList = (w < 650) ? (w - 373) : 277;
	SetWindowPos(item, NULL, 0, 0, wList, h, SWP_NOMOVE | SWP_NOZORDER);

	item = GetDlgItem(m_Window, IDC_ABOUTMEASURES_ITEMS_LISTVIEW);
	SetWindowPos(item, NULL, (w < 650) ? (w - 365) : 285, 0, w - wList - 10, h, SWP_NOZORDER);

	// Adjust third column
	LVCOLUMN lvc;
	lvc.mask = LVCF_WIDTH;
	lvc.cx = w - wList - 243;
	ListView_SetColumn(item, 2, &lvc);
}

/*
** UpdateSkinList
**
** Updates the list of skins.
**
*/
void CDialogAbout::CTabMeasures::UpdateSkinList()
{
	WCHAR* selectedItemName = NULL;

	HWND item = GetDlgItem(m_Window, IDC_ABOUTMEASURES_ITEMS_LISTBOX);
	int selected = (int)SendMessage(item, LB_GETCURSEL, 0, 0);

	// Get current selected entry
	if (selected != LB_ERR)
	{
		int selectedItemLen = (int)SendMessage(item, LB_GETTEXTLEN, selected, 0);

		if (selectedItemLen != LB_ERR)
		{
			selectedItemName = new WCHAR[selectedItemLen + 1];

			if (LB_ERR != SendMessage(item, LB_GETTEXT, selected, (LPARAM)selectedItemName))
			{
				selectedItemName[selectedItemLen] = L'\0';
			}
			else
			{
				delete [] selectedItemName;
				selectedItemName = NULL;
			}
		}
	}

	// Delete all entries
	SendMessage(item, LB_RESETCONTENT, 0, 0);

	// TODO Move following to common
	const std::map<std::wstring, CMeterWindow*>& windows = Rainmeter->GetAllMeterWindows();
	std::map<std::wstring, CMeterWindow*>::const_iterator iter = windows.begin();
	for ( ; iter != windows.end(); ++iter)
	{
		CMeterWindow* meterWindow = (*iter).second;
		const std::wstring& skinName = meterWindow->GetSkinName();
		SendMessage(item, LB_ADDSTRING, NULL, (LPARAM)skinName.c_str());
		size_t namelength = skinName.length();

		int currwidth = (int)SendMessage(item, LB_GETHORIZONTALEXTENT, NULL, NULL);
		if (6 * (int)namelength > currwidth)
		{
			SendMessage(item, LB_SETHORIZONTALEXTENT, 6 * namelength, NULL);
		}
	}

	if (selectedItemName != NULL)
	{
		int sel = 0;
		SendMessage(item, LB_SETCURSEL, sel, 0);

		const std::map<std::wstring, CMeterWindow*>& windows = Rainmeter->GetAllMeterWindows();
		std::map<std::wstring, CMeterWindow*>::const_iterator iter = windows.begin();
		for ( ; iter != windows.end(); ++iter)
		{
			if (_wcsicmp(selectedItemName, (*iter).first.c_str()) == 0)
			{
				SendMessage(item, LB_SETCURSEL, sel, 0);
				break;
			}
			++sel;
		}

		delete [] selectedItemName;
	}
}

/*
** UpdateSkinList
**
** Updates the list of measures and values.
**
*/
void CDialogAbout::CTabMeasures::UpdateMeasureList(LPCTSTR entryName)
{
	HWND item = GetDlgItem(m_Window, IDC_ABOUTMEASURES_ITEMS_LISTBOX);
	int selected = (int)SendMessage(item, LB_GETCURSEL, NULL, NULL);

	const std::map<std::wstring, CMeterWindow*>& windows = Rainmeter->GetAllMeterWindows();
	std::map<std::wstring, CMeterWindow*>::const_iterator iter = windows.begin();
	for (int i = 0; iter != windows.end(); ++i, ++iter)
	{
		if (i == selected &&
			(entryName == NULL || _wcsicmp(entryName, (*iter).first.c_str()) == 0))
		{
			item = GetDlgItem(m_Window, IDC_ABOUTMEASURES_ITEMS_LISTVIEW);
			SendMessage(item, WM_SETREDRAW, 0, 0);
			int count = ListView_GetItemCount(item);
			int index = 0;

			CMeterWindow* meterWindow = (*iter).second;
			const std::list<CMeasure*>& measures = meterWindow->GetMeasures();
			std::list<CMeasure*>::const_iterator j = measures.begin();
			for ( ; j != measures.end(); ++j)
			{
				const WCHAR* name = (*j)->GetName();
				const WCHAR* val = (*j)->GetStats();

				WCHAR buffer[256];
				double minVal = (*j)->GetMinValue();
				double maxVal = (*j)->GetMaxValue();
				CMeasure::GetScaledValue(AUTOSCALE_ON, 1, minVal, buffer, _countof(buffer));
				std::wstring range = buffer;
				range += L" - ";
				CMeasure::GetScaledValue(AUTOSCALE_ON, 1, maxVal, buffer, _countof(buffer));
				range += buffer;

				if (name && *name)
				{
					if (index < count)
					{
						ListView_SetItemText(item, index, 0, (WCHAR*)name);
					}
					else
					{
						LVITEM vitem;
						vitem.mask = LVIF_TEXT;
						vitem.iItem = index;
						vitem.iSubItem = 0;
						vitem.pszText = (WCHAR*)name;
						ListView_InsertItem(item, &vitem);
					}

					ListView_SetItemText(item, index, 1, (WCHAR*)range.c_str());
					if (val)
					{
						ListView_SetItemText(item, index, 2, (WCHAR*)val);
					}
					++index;
				}
			}

			if (count > index)
			{
				// Delete unnecessary items
				for (int j = index; j < count; ++j)
				{
					ListView_DeleteItem(item, index);
				}
			}

			SendMessage(item, WM_SETREDRAW, 1, 0);
			break;
		}
	}
}

/*
** DlgProc
**
** Dialog procedure for the measures dialog.
**
*/
INT_PTR CALLBACK CDialogAbout::CTabMeasures::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return c_Dialog->m_TabMeasures->OnCommand(wParam, lParam);
	}

	return FALSE;
}

INT_PTR CDialogAbout::CTabMeasures::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_ABOUTMEASURES_ITEMS_LISTBOX:
		if (HIWORD(wParam) == LBN_SELCHANGE)
		{
			UpdateMeasureList(NULL);
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------------
//
//                                Plugins tab
//
// -----------------------------------------------------------------------------------------------

/*
** CTabPlugins
**
** Constructor.
**
*/
CDialogAbout::CTabPlugins::CTabPlugins(HWND wnd) : CTab(wnd)
{
}

/*
** Initialize
**
** Called when tab is displayed.
**
*/
void CDialogAbout::CTabPlugins::Initialize()
{
	m_Initialized = true;

	// Add columns to the list view
	HWND item = GetDlgItem(m_Window, IDC_ABOUTPLUGINS_ITEMS_LISTVIEW);

	LVCOLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.iSubItem = 0;
	lvc.pszText = L"Name";
	lvc.cx = 140;
	lvc.fmt = LVCFMT_LEFT;  // left-aligned column
	ListView_InsertColumn(item, 0, &lvc);
	lvc.iSubItem = 1;
	lvc.cx = 80;
	lvc.pszText = L"Version";
	ListView_InsertColumn(item, 1, &lvc);
	lvc.iSubItem = 2;
	lvc.cx = 310;
	lvc.pszText = L"Author";
	ListView_InsertColumn(item, 2, &lvc);

	LVITEM vitem;
	vitem.mask = LVIF_TEXT;
	vitem.iItem = 0;
	vitem.iSubItem = 0;

	// Scan for plugins
	WIN32_FIND_DATA fileData;      // Data structure describes the file found
	HANDLE hSearch;                // Search handle returned by FindFirstFile

	std::wstring files = Rainmeter->GetPluginPath() + L"*.dll";

	// Start searching for .ini files in the given directory.
	hSearch = FindFirstFile(files.c_str(), &fileData);
	int index = 0;
	do
	{
		if (hSearch == INVALID_HANDLE_VALUE) break;    // No more files found

		// Try to get the version and author
		std::wstring tmpSz = Rainmeter->GetPluginPath() + fileData.cFileName;
		DWORD err = 0;
		HMODULE dll = CSystem::RmLoadLibrary(tmpSz.c_str(), &err, true);
		if (dll)
		{
			vitem.iItem = index;
			vitem.pszText = fileData.cFileName;
			ListView_InsertItem(item, &vitem);

			GETPLUGINVERSION GetVersionFunc = (GETPLUGINVERSION)GetProcAddress(dll, "GetPluginVersion");
			if (GetVersionFunc)
			{
				UINT version = GetVersionFunc();
				WCHAR buffer[64];
				_snwprintf_s(buffer, _TRUNCATE, L"%u.%u", version / 1000, version % 1000);
				ListView_SetItemText(item, vitem.iItem, 1, buffer);
			}

			GETPLUGINAUTHOR GetAuthorFunc = (GETPLUGINAUTHOR)GetProcAddress(dll, "GetPluginAuthor");
			if (GetAuthorFunc)
			{
				LPCTSTR author = GetAuthorFunc();
				if (author && *author)
				{
					ListView_SetItemText(item, vitem.iItem, 2, (LPWSTR)author);
				}
			}

			++index;
			FreeLibrary(dll);
		}
		else
		{
			LogWithArgs(LOG_ERROR, L"Unable to load plugin: %s (%u)", tmpSz.c_str(), err);
		}
	}
	while (FindNextFile(hSearch, &fileData));

	FindClose(hSearch);
}

/*
** Resize
**
** Resizes window and repositions controls.
**
*/
void CDialogAbout::CTabPlugins::Resize(int w, int h)
{
	SetWindowPos(m_Window, NULL, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);

	HWND item = GetDlgItem(m_Window, IDC_ABOUTPLUGINS_ITEMS_LISTVIEW);
	SetWindowPos(item, NULL, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);

	// Adjust third colum
	LVCOLUMN lvc;
	lvc.mask = LVCF_WIDTH;
	lvc.cx = w - 242;
	ListView_SetColumn(item, 2, &lvc);
}

/*
** DlgProc
**
** Dialog procedure for the Plugins tab.
**
*/
INT_PTR CALLBACK CDialogAbout::CTabPlugins::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

// -----------------------------------------------------------------------------------------------
//
//                                Version tab
//
// -----------------------------------------------------------------------------------------------

/*
** CTabVersion
**
** Constructor.
**
*/
CDialogAbout::CTabVersion::CTabVersion(HWND wnd) : CTab(wnd)
{
}

/*
** Initialize
**
** Called when tab is displayed.
**
*/
void CDialogAbout::CTabVersion::Initialize()
{
	m_Initialized = true;

	HWND item = GetDlgItem(m_Window, IDC_ABOUTVERSION_VERSION_TEXT);
	WCHAR tmpSz[128];
	_snwprintf_s(tmpSz, _TRUNCATE, L"%s %s%s r%i %s (%s)", APPNAME, APPVERSION, revision_beta ? L" beta" : L"", revision_number, APPBITS, APPDATE);
	SetWindowText(item, tmpSz);

	item = GetDlgItem(m_Window, IDC_ABOUTVERSION_PATHS_TEXT);
	std::wstring text = L"Path: " + Rainmeter->GetPath();
	text += L"\r\nSettings: ";
	text += Rainmeter->GetSettingsPath();
	text += L"\r\nSkins: ";
	text += Rainmeter->GetSkinPath();
	SetWindowText(item, text.c_str());
}

/*
** Resize
**
** Resizes window and repositions controls.
**
*/
void CDialogAbout::CTabVersion::Resize(int w, int h)
{
	SetWindowPos(m_Window, NULL, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);
}

/*
** DlgProc
**
** Dialog procedure for the Version tab.
**
*/
INT_PTR CALLBACK CDialogAbout::CTabVersion::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return c_Dialog->m_TabVersion->OnCommand(wParam, lParam);

	case WM_NOTIFY:
		return c_Dialog->m_TabVersion->OnNotify(wParam, lParam);
	}

	return FALSE;
}

INT_PTR CDialogAbout::CTabVersion::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_ABOUTVERSION_COPY_BUTTON:
		{
			WCHAR tmpSz[128];
			_snwprintf_s(tmpSz, _TRUNCATE, L"%s %s%s r%i %s (%s)", APPNAME, APPVERSION, revision_beta ? L" beta" : L"", revision_number, APPBITS, APPDATE);
			std::wstring text = tmpSz;
			text += L"\r\nPath: " + Rainmeter->GetPath();
			text += L"\r\nSettings: ";
			text += Rainmeter->GetSettingsPath();
			text += L"\r\nSkins: ";
			text += Rainmeter->GetSkinPath();
			CSystem::SetClipboardText(text);
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

INT_PTR CDialogAbout::CTabVersion::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR nm = (LPNMHDR)lParam;
	switch (nm->code)
	{
	case NM_CLICK:
		LSExecute(NULL, ((PNMLINK)lParam)->item.szUrl, SW_SHOWNORMAL);
		break;

	default:
		return FALSE;
	}

	return TRUE;
}
