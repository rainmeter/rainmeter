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
#include "DialogDebug.h"
#include "../Common/FileUtil.h"
#include "../Common/MathParser.h"
#include "../Common/StringUtil.h"

WINDOWPLACEMENT DialogDebug::c_WindowPlacement = { 0 };
DialogDebug* DialogDebug::c_Dialog = nullptr;

DialogDebug::DialogDebug() : Dialog(&c_WindowPlacement)
{
}

DialogDebug::~DialogDebug()
{
}

void DialogDebug::Open(int tab)
{
	if (!c_Dialog)
	{
		c_Dialog = new DialogDebug();
	}

	c_Dialog->ShowDialogWindow(
		GetString(IDS_Debug),
		0, 0, 600, 400,
		DS_CENTER | WS_POPUP | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
		WS_EX_APPWINDOW | WS_EX_CONTROLPARENT | (GetRainmeter().IsLanguageRTL() ? WS_EX_LAYOUTRTL : 0),
		nullptr);

	c_Dialog->SelectTab(tab);
}

void DialogDebug::Open(const WCHAR* name)
{
	int tab = 0;

	if (name)
	{
		if (_wcsicmp(name, L"Skins") == 0 ||
			_wcsicmp(name, L"Measures") == 0)	// For backwards compatibility
		{
			tab = 1;
		}
		else if (_wcsicmp(name, L"Plugins") == 0)
		{
			tab = 2;
		}
	}

	Open(tab);
}

/*
** Shows log if dialog isn't already open.
**
*/
void DialogDebug::ShowAboutLog()
{
	if (!c_Dialog)
	{
		Open();
	}
}

void DialogDebug::AddLogItem(Logger::Level level, LPCWSTR time, LPCWSTR source, LPCWSTR message)
{
	if (c_Dialog && c_Dialog->m_TabLog.IsInitialized())
	{
		c_Dialog->m_TabLog.AddItem(level, time, source, message);
	}
}

void DialogDebug::UpdateSkins()
{
	if (c_Dialog && c_Dialog->m_TabSkins.IsInitialized())
	{
		c_Dialog->m_TabSkins.UpdateSkinList();
	}
}

void DialogDebug::UpdateMeasures(Skin* skin)
{
	if (c_Dialog && c_Dialog->m_TabSkins.IsInitialized())
	{
		c_Dialog->m_TabSkins.UpdateMeasureList(skin);
	}
}

INT_PTR DialogDebug::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const INT_PTR baseResult = Dialog::HandleMessage(uMsg, wParam, lParam);

	switch (uMsg)
	{
	case WM_INITDIALOG:
		return OnInitDialog(wParam, lParam);

	case WM_COMMAND:
		return OnCommand(wParam, lParam);

	case WM_GETMINMAXINFO:
		{
			MINMAXINFO* mmi = (MINMAXINFO*)lParam;
			mmi->ptMinTrackSize.x = MulDiv(800, (int)m_Dpi, USER_DEFAULT_SCREEN_DPI);
			mmi->ptMinTrackSize.y = MulDiv(390, (int)m_Dpi, USER_DEFAULT_SCREEN_DPI);
		}
		return FALSE;

	case WM_SIZE:
		{
			if (wParam != SIZE_MINIMIZED)
			{
				Relayout();
			}
		}
		return TRUE;

	case WM_CLOSE:
		{
			delete c_Dialog;
			c_Dialog = nullptr;
		}
		return TRUE;
	}

	return baseResult;
}

INT_PTR DialogDebug::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	static const Control s_Controls[] =
	{
		Control::Button(Id_CloseButton, IDS_Close,
			544, 381, 50, 14,
			WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0,
			Control::ANCHOR_BOTTOM_RIGHT),
		Control::Tab(Id_Tab, 0,
			6, 6, 588, 371,
			WS_VISIBLE | WS_TABSTOP | TCS_FIXEDWIDTH, 0,
			Control::ANCHOR_ALL)  // Last for correct tab order.
	};

	CreateControls(s_Controls, _countof(s_Controls), GetString);

	AddTab(Id_Tab, m_TabLog, GetString(IDS_Log));
	AddTab(Id_Tab, m_TabSkins, GetString(IDS_Skins));
	AddTab(Id_Tab, m_TabPlugins, GetString(IDS_Plugins));
	HICON hIcon = GetIcon(IDI_RAINMETER, true);
	SendMessage(m_Window, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);  // Titlebar icon: 16x16
	SendMessage(m_Window, WM_SETICON, ICON_BIG, (LPARAM)hIcon);    // Taskbar icon:  32x32

	HWND item = GetControl(Id_CloseButton);
	SendMessage(m_Window, WM_NEXTDLGCTL, (WPARAM)item, TRUE);

	item = m_TabLog.GetControl(TabLog::Id_LogListView);
	SetWindowTheme(item, L"explorer", nullptr);
	item = m_TabSkins.GetControl(TabSkins::Id_SkinsListView);
	SetWindowTheme(item, L"explorer", nullptr);
	item = m_TabPlugins.GetControl(TabPlugins::Id_PluginsListView);
	SetWindowTheme(item, L"explorer", nullptr);

	return TRUE;
}

INT_PTR DialogDebug::OnCommand(WPARAM wParam, LPARAM lParam)
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

// -----------------------------------------------------------------------------------------------
//
//                                Log tab
//
// -----------------------------------------------------------------------------------------------

DialogDebug::TabLog::TabLog() : Tab(),
	m_Error(true),
	m_Warning(true),
	m_Notice(true),
	m_Debug(true),
	m_ImageList(nullptr)
{
}

DialogDebug::TabLog::~TabLog()
{
	DestroyImageList();
}

void DialogDebug::TabLog::Create(HWND owner)
{
	Tab::CreateTabWindow(15, 30, 570, 338, owner);

	// FIXME: Temporary hack.
	short buttonWidth = (short)GetRainmeter().GetLanguageButtonWidth();

	static const Control s_Controls[] =
	{
		Control::ListView(Id_LogListView, 0,
			0, 0, 568, 325,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | LVS_ICON | LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER, 0,
			Control::ANCHOR_ALL),
		Control::CheckBox(Id_ErrorCheckBox, IDS_Error,
			0, 329, 80, 14,
			WS_VISIBLE | WS_TABSTOP, 0, Control::ANCHOR_LEFT | Control::ANCHOR_BOTTOM),
		Control::CheckBox(Id_WarningCheckBox, IDS_Warning,
			80, 329, 80, 14,
			WS_VISIBLE | WS_TABSTOP, 0, Control::ANCHOR_LEFT | Control::ANCHOR_BOTTOM),
		Control::CheckBox(Id_NoticeCheckBox, IDS_Notice,
			160, 329, 80, 14,
			WS_VISIBLE | WS_TABSTOP, 0, Control::ANCHOR_LEFT | Control::ANCHOR_BOTTOM),
		Control::CheckBox(Id_DebugCheckBox, IDS_Debug,
			240, 329, 80, 14,
			WS_VISIBLE | WS_TABSTOP, 0, Control::ANCHOR_LEFT | Control::ANCHOR_BOTTOM),
		Control::Button(Id_ClearButton, IDS_Clear,
			(568 - buttonWidth), 329, buttonWidth, 14,
			WS_VISIBLE | WS_TABSTOP, 0, Control::ANCHOR_BOTTOM_RIGHT)
	};

	CreateControls(s_Controls, _countof(s_Controls), GetString);
}

/*
** Called when tab is displayed.
**
*/
void DialogDebug::TabLog::Initialize()
{
	// Add columns to the list view
	HWND item = GetControl(Id_LogListView);
	ListView_SetExtendedListViewStyleEx(item, 0, LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	CreateImageList();

	LVCOLUMN lvc = { 0 };
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;  // left-aligned column
	lvc.iSubItem = 0;
	lvc.cx = m_ControlTemplate.ScaleDialogUnits(75);
	lvc.pszText = (WCHAR*)GetString(IDS_Type);
	ListView_InsertColumn(item, 0, &lvc);
	lvc.iSubItem = 1;
	lvc.cx = m_ControlTemplate.ScaleDialogUnits(85);
	lvc.pszText = (WCHAR*)GetString(IDS_Time);
	ListView_InsertColumn(item, 1, &lvc);
	lvc.iSubItem = 2;
	lvc.cx = m_ControlTemplate.ScaleDialogUnits(225);
	lvc.pszText = (WCHAR*)GetString(IDS_Source);
	ListView_InsertColumn(item, 2, &lvc);
	lvc.iSubItem = 3;
	lvc.cx = m_ControlTemplate.ScaleDialogUnits(180);  // Resized later
	lvc.pszText = (WCHAR*)GetString(IDS_Message);
	ListView_InsertColumn(item, 3, &lvc);

	// Start 4th column at max width
	RECT rc;
	GetClientRect(m_Window, &rc);
	Relayout(rc.right, rc.bottom);

	// Add stored entires
	for (const auto& entry : GetLogger().GetEntries())
	{
		AddItem(entry.level, entry.timestamp.c_str(), entry.source.c_str(), entry.message.c_str());
	}

	item = GetControl(Id_ErrorCheckBox);
	Button_SetCheck(item, BST_CHECKED);

	item = GetControl(Id_WarningCheckBox);
	Button_SetCheck(item, BST_CHECKED);

	item = GetControl(Id_NoticeCheckBox);
	Button_SetCheck(item, BST_CHECKED);

	item = GetControl(Id_DebugCheckBox);
	Button_SetCheck(item, BST_CHECKED);

	m_Initialized = true;
}

void DialogDebug::TabLog::CreateImageList()
{
	HWND list = GetControl(Id_LogListView);
	const int iconSize = m_ControlTemplate.ScaleDialogUnits(16);
	HIMAGELIST imageList = ImageList_Create(iconSize, iconSize, ILC_COLOR32, 3, 1);
	HMODULE user = GetModuleHandle(L"user32");
	const UINT iconIds[] = { 103, 101, 104 };
	for (UINT iconId : iconIds)
	{
		HICON icon = (HICON)LoadImage(user, MAKEINTRESOURCE(iconId), IMAGE_ICON,
			iconSize, iconSize, LR_DEFAULTCOLOR);
		ImageList_AddIcon(imageList, icon);
		DestroyIcon(icon);
	}

	DestroyImageList();
	m_ImageList = imageList;
	ListView_SetImageList(list, m_ImageList, LVSIL_SMALL);
}

void DialogDebug::TabLog::DestroyImageList()
{
	if (m_ImageList)
	{
		HWND item = GetControl(Id_LogListView);
		ImageList_Destroy(ListView_SetImageList(item, nullptr, LVSIL_SMALL));
		m_ImageList = nullptr;
	}
}

void DialogDebug::TabLog::HandleDpiChange()
{
	HWND list = GetControl(Id_LogListView);
	ListView_SetColumnWidth(list, 0, m_ControlTemplate.ScaleDialogUnits(75));
	ListView_SetColumnWidth(list, 1, m_ControlTemplate.ScaleDialogUnits(85));
	ListView_SetColumnWidth(list, 2, m_ControlTemplate.ScaleDialogUnits(225));
	CreateImageList();

	RECT rect;
	GetClientRect(m_Window, &rect);
	Relayout(rect.right, rect.bottom);
}

void DialogDebug::TabLog::Relayout(int w, int h)
{
	Tab::Relayout(w, h);

	RECT r;
	LONG bottom;
	HWND item = GetControl(Id_ClearButton);
	GetClientRect(item, &r);
	bottom = r.bottom;

	SetWindowPos(item, nullptr, w - r.right, h - bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	item = GetControl(Id_ErrorCheckBox);
	GetClientRect(item, &r);
	SetWindowPos(item, nullptr, 0, h - bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	item = GetControl(Id_WarningCheckBox);
	SetWindowPos(item, nullptr, r.right, h - bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	item = GetControl(Id_NoticeCheckBox);
	SetWindowPos(item, nullptr, r.right * 2, h - bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	item = GetControl(Id_DebugCheckBox);
	SetWindowPos(item, nullptr, r.right * 3, h - bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	item = GetControl(Id_LogListView);
	SetWindowPos(item, nullptr, 0, 0, w, h - bottom - m_ControlTemplate.ScaleDialogUnits(10), SWP_NOMOVE | SWP_NOZORDER);

	// Adjust 4th colum
	LVCOLUMN lvc = { 0 };
	lvc.mask = LVCF_WIDTH;
	lvc.cx = w - m_ControlTemplate.ScaleDialogUnits(20) -
		(ListView_GetColumnWidth(item, 0) +
		 ListView_GetColumnWidth(item, 1) +
		 ListView_GetColumnWidth(item, 2));
	ListView_SetColumn(item, 3, &lvc);
}

/*
** Adds item to log.
**
*/
void DialogDebug::TabLog::AddItem(Logger::Level level, LPCWSTR time, LPCWSTR source, LPCWSTR message)
{
	WCHAR buffer[32] = { 0 };
	LVITEM vitem = { 0 };
	vitem.mask = LVIF_IMAGE | LVIF_TEXT;
	vitem.iItem = 0;
	vitem.iSubItem = 0;
	vitem.pszText = buffer;
	HWND item = nullptr;

	switch (level)
	{
	case Logger::Level::Error:
		if (!m_Error) return;
		item = GetControl(Id_ErrorCheckBox);
		vitem.iImage = 0;
		break;

	case Logger::Level::Warning:
		if (!m_Warning) return;
		item = GetControl(Id_WarningCheckBox);
		vitem.iImage = 1;
		break;

	case Logger::Level::Notice:
		if (!m_Notice) return;
		item = GetControl(Id_NoticeCheckBox);
		vitem.iImage = 2;
		break;

	case Logger::Level::Debug:
		if (!m_Debug) return;
		item = GetControl(Id_DebugCheckBox);
		vitem.iImage = I_IMAGENONE;
		break;
	}

	// ListView controls do not display the tab (\t) character,
	// so replace any tab characters with 4 spaces.
	std::wstring msg = message;
	size_t pos = 0;
	while ((pos = msg.find(L'\t', pos)) != std::string::npos)
	{
		msg.replace(pos, 1, L"    ");
		pos += 4;
	}

	if (!item) return;

	GetWindowText(item, buffer, 32);
	item = GetControl(Id_LogListView);
	ListView_InsertItem(item, &vitem);
	ListView_SetItemText(item, vitem.iItem, 1, (WCHAR*)time);
	ListView_SetItemText(item, vitem.iItem, 2, (WCHAR*)source);
	ListView_SetItemText(item, vitem.iItem, 3, (WCHAR*)msg.c_str());
	if (!ListView_IsItemVisible(item, 0))
	{
		ListView_Scroll(item, 0, 16);
	}
}

INT_PTR DialogDebug::TabLog::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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

INT_PTR DialogDebug::TabLog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case Id_ErrorCheckBox:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			m_Error = !m_Error;
		}
		break;

	case Id_WarningCheckBox:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			m_Warning = !m_Warning;
		}
		break;

	case Id_NoticeCheckBox:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			m_Notice = !m_Notice;
		}
		break;

	case Id_DebugCheckBox:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			m_Debug = !m_Debug;
		}
		break;

	case Id_ClearButton:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			HWND item = GetControl(Id_LogListView);
			ListView_DeleteAllItems(item);
		}
		break;

	case IDM_COPY:
		{
			HWND hwnd = GetControl(Id_LogListView);
			const int sel = ListView_GetNextItem(hwnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
			if (sel != -1)
			{
				WCHAR buffer[512] = { 0 };

				// Get message.
				ListView_GetItemText(hwnd, sel, 3, buffer, 512);
				std::wstring message = buffer;

				// Get source (if any).
				ListView_GetItemText(hwnd, sel, 2, buffer, 512);
				if (*buffer)
				{
					message += L" (";
					message += buffer;
					message += L')';
				}

				System::SetClipboardText(message);
			}
		}
		break;

	default:
		return 1;
	}

	return 0;
}

INT_PTR DialogDebug::TabLog::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR nm = (LPNMHDR)lParam;
	switch (nm->code)
	{
	case LVN_KEYDOWN:
		{
			NMLVKEYDOWN* lvkd = (NMLVKEYDOWN*)nm;
			if (lvkd->wVKey == 0x43 && // C key.
				IsCtrlKeyDown())
			{
				const int sel = ListView_GetNextItem(nm->hwndFrom, -1, LVNI_FOCUSED | LVNI_SELECTED);
				if (sel != -1)
				{
					WCHAR buffer[512] = { 0 };

					// Get message.
					ListView_GetItemText(nm->hwndFrom, sel, 3, buffer, 512);
					std::wstring message = buffer;

					// Get source (if any).
					ListView_GetItemText(nm->hwndFrom, sel, 2, buffer, 512);
					if (*buffer)
					{
						message += L" (";
						message += buffer;
						message += L')';
					}

					System::SetClipboardText(message);
				}
			}
		}
		break;

	case NM_RCLICK:
		if (nm->idFrom == Id_LogListView)
		{
			HWND hwnd = nm->hwndFrom;
			const int sel = ListView_GetNextItem(hwnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
			if (sel != -1)
			{
				NMITEMACTIVATE* item = (NMITEMACTIVATE*)lParam;

				LVITEM lvi = { 0 };
				lvi.mask = LVIF_GROUPID;
				lvi.iItem = item->iItem;
				lvi.iSubItem = 0;
				lvi.iGroupId = -1;
				ListView_GetItem(hwnd, &lvi);

				static const MenuTemplate s_MessageMenu[] =
				{
					MENU_ITEM(IDM_COPY, IDS_CopyToClipboard)
				};

				HMENU menu = MenuTemplate::CreateMenu(s_MessageMenu, _countof(s_MessageMenu), GetString);
				if (menu)
				{
					POINT pt = System::GetCursorPosition();

					// Show context menu
					TrackPopupMenu(
						menu,
						TPM_RIGHTBUTTON | TPM_LEFTALIGN,
						pt.x,
						pt.y,
						0,
						m_Window,
						nullptr);

					DestroyMenu(menu);
				}
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
//                                Measures tab
//
// -----------------------------------------------------------------------------------------------

DialogDebug::TabSkins::TabSkins() : Tab(),
	m_SkinWindow()
{
}

void DialogDebug::TabSkins::Create(HWND owner)
{
	Tab::CreateTabWindow(15, 30, 570, 338, owner);

	static const Control s_Controls[] =
	{
		Control::ListBox(Id_SkinsListBox, 0,
			0, 0, 120, 338,
			WS_VISIBLE | WS_TABSTOP | LBS_NOTIFY | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_HSCROLL, WS_EX_CLIENTEDGE,
			Control::ANCHOR_LEFT | Control::ANCHOR_TOP | Control::ANCHOR_BOTTOM),
		Control::ListView(Id_SkinsListView, 0,
			125, 0, 442, 250,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER, 0,
			Control::ANCHOR_ALL),
		Control::GroupBox(Id_EvaluateGroup, 0,
			125, 262, 442, 75,
			WS_VISIBLE, 0,
			Control::ANCHOR_LEFT | Control::ANCHOR_RIGHT | Control::ANCHOR_BOTTOM),
		Control::Edit(Id_EvaluateEdit, 0,
			132, 279, 340, 22,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_WANTRETURN, 0,
			Control::ANCHOR_LEFT | Control::ANCHOR_RIGHT | Control::ANCHOR_BOTTOM),
		Control::Edit(Id_EvaluateResult, 0,
			132, 307, 340, 22,
			WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY, 0,
			Control::ANCHOR_LEFT | Control::ANCHOR_RIGHT | Control::ANCHOR_BOTTOM),
		Control::RadioButton(Id_EvaluateStringRadio, 0,
			480, 279, 80, 14,
			WS_VISIBLE | WS_TABSTOP, 0,
			Control::ANCHOR_RIGHT | Control::ANCHOR_BOTTOM),
		Control::RadioButton(Id_EvaluateNumberRadio, 0,
			480, 292, 80, 14,
			WS_VISIBLE | WS_TABSTOP, 0,
			Control::ANCHOR_RIGHT | Control::ANCHOR_BOTTOM),
		Control::Button(Id_EvaluateExecuteButton, 0,
			480, 314, 80, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0,
			Control::ANCHOR_RIGHT | Control::ANCHOR_BOTTOM)
	};

	CreateControls(s_Controls, _countof(s_Controls), GetString);
}

void DialogDebug::TabSkins::Initialize()
{
	// Add columns to the list view
	HWND item = GetControl(Id_SkinsListView);
	ListView_SetExtendedListViewStyleEx(item, 0, LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	LVGROUP lvg = { 0 };
	lvg.cbSize = sizeof(LVGROUP);
	lvg.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
	lvg.state = lvg.stateMask = LVGS_NORMAL | LVGS_COLLAPSIBLE;
	lvg.iGroupId = 0;
	lvg.pszHeader = (WCHAR*)GetString(IDS_Measures);
	ListView_InsertGroup(item, 0, &lvg);
	lvg.iGroupId = 1;
	lvg.pszHeader = (WCHAR*)GetString(IDS_Variables);
	ListView_InsertGroup(item, 1, &lvg);

	ListView_EnableGroupView(item, TRUE);

	LVCOLUMN lvc = { 0 };
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.iSubItem = 0;
	lvc.cx = m_ControlTemplate.ScaleDialogUnits(120);
	lvc.pszText = (WCHAR*)GetString(IDS_Name);
	ListView_InsertColumn(item, 0, &lvc);
	lvc.iSubItem = 1;
	lvc.cx = m_ControlTemplate.ScaleDialogUnits(80);
	lvc.pszText = (WCHAR*)GetString(IDS_Range);
	ListView_InsertColumn(item, 1, &lvc);
	lvc.iSubItem = 2;
	lvc.cx = m_ControlTemplate.ScaleDialogUnits(90);
	lvc.pszText = (WCHAR*)GetString(IDS_Number);
	ListView_InsertColumn(item, 2, &lvc);
	lvc.iSubItem = 3;
	lvc.cx = m_ControlTemplate.ScaleDialogUnits(110);  // Resized later
	lvc.pszText = (WCHAR*)GetString(IDS_String);
	ListView_InsertColumn(item, 3, &lvc);

	// Start 4th column at max width
	RECT rc;
	GetClientRect(m_Window, &rc);
	Relayout(rc.right, rc.bottom);
	SetWindowText(GetControl(Id_EvaluateGroup), L"Evaluate");
	SetWindowText(GetControl(Id_EvaluateStringRadio), L"String");
	Button_SetCheck(GetControl(Id_EvaluateStringRadio), BST_CHECKED);
	SetWindowText(GetControl(Id_EvaluateNumberRadio), L"Number");
	SetWindowText(GetControl(Id_EvaluateExecuteButton), L"Execute");

	UpdateSkinList();

	m_Initialized = true;
}

void DialogDebug::TabSkins::Relayout(int w, int h)
{
	Tab::Relayout(w, h);

	// Adjust 4th column
	const int listWidth = m_ControlTemplate.ScaleDialogUnits(265);
	const int listGap = m_ControlTemplate.ScaleDialogUnits(10);
	HWND item = GetControl(Id_SkinsListView);
	LVCOLUMN lvc = { 0 };
	lvc.mask = LVCF_WIDTH;
	lvc.cx = w - listWidth - listGap - m_ControlTemplate.ScaleDialogUnits(20) -
		(ListView_GetColumnWidth(item, 0) +
		 ListView_GetColumnWidth(item, 1) +
		 ListView_GetColumnWidth(item, 2));
	ListView_SetColumn(item, 3, &lvc);
}

void DialogDebug::TabSkins::HandleDpiChange()
{
	HWND list = GetControl(Id_SkinsListView);
	ListView_SetColumnWidth(list, 0, m_ControlTemplate.ScaleDialogUnits(120));
	ListView_SetColumnWidth(list, 1, m_ControlTemplate.ScaleDialogUnits(80));
	ListView_SetColumnWidth(list, 2, m_ControlTemplate.ScaleDialogUnits(90));

	RECT rect;
	GetClientRect(m_Window, &rect);
	Relayout(rect.right, rect.bottom);
}

/*
** Updates the list of skins.
**
*/
void DialogDebug::TabSkins::UpdateSkinList()
{
	// Delete all entries
	HWND item = GetControl(Id_SkinsListBox);
	ListBox_ResetContent(item);

	// Add entries for each skin
	std::wstring::size_type maxLength = 0;
	bool found = false;
	for (const auto& iter : GetRainmeter().GetAllSkins())
	{
		const std::wstring& skinName = iter.first;
		std::wstring::size_type curLength = skinName.length();
		if (curLength > maxLength)
		{
			maxLength = curLength;
		}

		const WCHAR* name = skinName.c_str();
		int index = ListBox_AddString(item, name);
		if (!found && m_SkinWindow == iter.second)
		{
			found = true;
			m_SkinWindow = iter.second;
			ListBox_SetCurSel(item, index);
		}
	}

	ListBox_SetHorizontalExtent(item, 6 * maxLength);

	if (!found)
	{
		if (GetRainmeter().GetAllSkins().empty())
		{
			m_SkinWindow = nullptr;
			item = GetControl(Id_SkinsListView);
			ListView_DeleteAllItems(item);
		}
		else
		{
			// Default to first skin
			m_SkinWindow = GetRainmeter().GetAllSkins().begin()->second;
			ListBox_SetCurSel(item, 0);
			UpdateMeasureList(m_SkinWindow);
		}
	}
}

/*
** Updates the list of measures and values.
**
*/
void DialogDebug::TabSkins::UpdateMeasureList(Skin* skin)
{
	if (!skin)
	{
		// Find selected skin
		HWND item = GetControl(Id_SkinsListBox);
		int selected = (int)SendMessage(item, LB_GETCURSEL, 0, 0);

		const auto& windows = GetRainmeter().GetAllSkins();
		auto iter = windows.cbegin();
		while (selected && iter != windows.cend())
		{
			++iter;
			--selected;
		}

		m_SkinWindow = (*iter).second;
	}
	else if (skin != m_SkinWindow)
	{
		// Called by a skin other than currently visible one, so return
		return;
	}

	HWND item = GetControl(Id_SkinsListView);
	SendMessage(item, WM_SETREDRAW, FALSE, 0);
	int count = ListView_GetItemCount(item);

	LVITEM lvi = { 0 };
	lvi.mask = LVIF_TEXT | LVIF_GROUPID | LVIF_PARAM;
	lvi.iSubItem = 0;
	lvi.iItem = 0;
	lvi.lParam = 0;

	lvi.iGroupId = 0;
	const std::vector<Measure*>& measures = m_SkinWindow->GetMeasures();
	std::vector<Measure*>::const_iterator j = measures.begin();
	for ( ; j != measures.end(); ++j)
	{
		lvi.pszText = (WCHAR*)(*j)->GetName();

		if (lvi.iItem < count)
		{
			ListView_SetItem(item, &lvi);
		}
		else
		{
			ListView_InsertItem(item, &lvi);
		}

		// Range
		WCHAR buffer[256];
		Measure::GetScaledValue(AUTOSCALE_ON, 1, (*j)->GetMinValue(), buffer, _countof(buffer));
		std::wstring range = buffer;
		range += L"- ";  // GetScaledValue returns an extra space
		Measure::GetScaledValue(AUTOSCALE_ON, 1, (*j)->GetMaxValue(), buffer, _countof(buffer));
		range += buffer;

		// Number value
		int bufferLen = _snwprintf_s(buffer, _TRUNCATE, L"%.5f", (*j)->GetValue());
		Measure::RemoveTrailingZero(buffer, bufferLen);
		std::wstring numValue = buffer;

		// String value
		std::wstring strValue = (*j)->GetStringOrFormattedValue(AUTOSCALE_OFF, 1.0, -1, false);
		if (strValue.length() > 259)
		{
			strValue.erase(256);
			strValue += L"...";
		}

		ListView_SetItemText(item, lvi.iItem, 1, (WCHAR*)range.c_str());
		ListView_SetItemText(item, lvi.iItem, 2, (WCHAR*)numValue.c_str());
		ListView_SetItemText(item, lvi.iItem, 3, (WCHAR*)strValue.c_str());
		++lvi.iItem;
	}

	lvi.iGroupId = 1;
	const auto& variables = m_SkinWindow->GetParser().GetVariables();
	for (auto iter = variables.cbegin(); iter != variables.cend(); ++iter)
	{
		const WCHAR* name = (*iter).first.c_str();
		lvi.lParam = (LPARAM)name;

		if (wcscmp(name, L"@") == 0)
		{
			// Ignore reserved variables
			continue;
		}

		const std::wstring* tmpStr = m_SkinWindow->GetParser().GetVariableOriginalName((*iter).first);
		if (!tmpStr) continue;  // Variable name does not exist

		lvi.pszText = (WCHAR*)tmpStr->c_str();

		// Truncate and add "..." if necessary
		std::wstring valStr = (*iter).second;
		if (valStr.length() > 259)
		{
			valStr.erase(256);
			valStr += L"...";
		}

		if (lvi.iItem < count)
		{
			ListView_SetItem(item, &lvi);
		}
		else
		{
			ListView_InsertItem(item, &lvi);
		}

		ListView_SetItemText(item, lvi.iItem, 1, (WCHAR*)L"");
		ListView_SetItemText(item, lvi.iItem, 2, (WCHAR*)L"");
		ListView_SetItemText(item, lvi.iItem, 3, (WCHAR*)valStr.c_str());
		++lvi.iItem;
	}

	// Delete unnecessary items
	while (count > lvi.iItem)
	{
		ListView_DeleteItem(item, lvi.iItem);
		--count;
	}

	int selIndex = ListView_GetNextItem(item, -1, LVNI_FOCUSED | LVNI_SELECTED);

	ListView_SortItems(item, ListSortProc, 0);

	UINT state = selIndex == -1 ? 0U : LVIS_FOCUSED | LVIS_SELECTED;

	// Re-select previously selected item (or deselect group header)
	ListView_SetItemState(item, selIndex, state, LVIS_FOCUSED | LVIS_SELECTED);

	SendMessage(item, WM_SETREDRAW, TRUE, 0);

	UpdateEvaluationResult();
}

int CALLBACK DialogDebug::TabSkins::ListSortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	// Measures
	if (!lParam1 && !lParam2) return 0;
	if (!lParam1) return -1;
	if (!lParam2) return 1;

	// Variables
	return wcscmp((const WCHAR*)lParam1, (const WCHAR*)lParam2);
}

void DialogDebug::TabSkins::UpdateEvaluationResult()
{
	HWND item = GetControl(Id_EvaluateEdit);
	if (!item) return;

	const int length = GetWindowTextLength(item);
	std::wstring text(length + 1, L'\0');
	GetWindowText(item, &text[0], length + 1);
	text.resize(length);

	if (m_SkinWindow && !text.empty())
	{
		ConfigParser& parser = m_SkinWindow->GetParser();
		parser.ReplaceVariables(text, true);
		parser.ReplaceMeasures(text);

		if (Button_GetCheck(GetControl(Id_EvaluateNumberRadio)) == BST_CHECKED)
		{
			double value = 0.0;
			const WCHAR* str = text.c_str();
			if (*str == L'(')
			{
				MathParser::CheckedParse(str, &value);
			}
			else if (*str)
			{
				errno = 0;
				value = wcstod(str, nullptr);
			}

			WCHAR buffer[128] = { 0 };
			int bufferLen = _snwprintf_s(buffer, _TRUNCATE, L"%lf", value);
			Measure::RemoveTrailingZero(buffer, bufferLen);
			text = buffer;
		}
	}

	SetWindowText(GetControl(Id_EvaluateResult), text.c_str());
	EnableWindow(GetControl(Id_EvaluateExecuteButton), text.starts_with(L"[!"));
}

INT_PTR DialogDebug::TabSkins::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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

INT_PTR DialogDebug::TabSkins::OnCommand(WPARAM wParam, LPARAM lParam)
{
	auto getMeasure = [&]() -> Measure*
	{
		HWND hwnd = GetControl(Id_SkinsListView);
		const int sel = ListView_GetNextItem(hwnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
		if (sel != -1)
		{
			WCHAR buffer[512] = { 0 };
			ListView_GetItemText(hwnd, sel, 0, buffer, _countof(buffer));
			std::wstring temp = buffer;
			Measure* measure = m_SkinWindow->GetMeasure(temp);
			if (measure)
			{
				return measure;
			}
		}

		return nullptr;
	};

	switch (LOWORD(wParam))
	{
	case Id_SkinsListBox:
		if (HIWORD(wParam) == LBN_SELCHANGE)
		{
			UpdateMeasureList(nullptr);
		}
		break;

	case Id_EvaluateEdit:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			UpdateEvaluationResult();
		}
		break;

	case Id_EvaluateStringRadio:
	case Id_EvaluateNumberRadio:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			UpdateEvaluationResult();
		}
		break;

	case Id_EvaluateExecuteButton:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			HWND result = GetControl(Id_EvaluateResult);
			const int length = GetWindowTextLength(result);
			if (length > 0)
			{
				std::wstring command(length + 1, L'\0');
				GetWindowText(result, &command[0], length + 1);
				command.resize(length);
				if (command.starts_with(L"[!"))
				{
					GetRainmeter().ExecuteCommand(command.c_str(), m_SkinWindow);
				}
			}
		}
		break;

	case IDM_COPYMEASURENAME:
		{
			Measure* measure = getMeasure();
			if (measure)
			{
				System::SetClipboardText(measure->GetName());
			}
		}
		break;

	case IDM_COPYNUMBERVALUE:
		{
			Measure* measure = getMeasure();
			if (measure)
			{
				WCHAR buffer[256];
				int bufferLen = _snwprintf_s(buffer, _TRUNCATE, L"%.5f", measure->GetValue());
				Measure::RemoveTrailingZero(buffer, bufferLen);
				std::wstring numValue = buffer;
				System::SetClipboardText(numValue);
			}
		}
		break;

	case IDM_COPYSTRINGVALUE:
		{
			Measure* measure = getMeasure();
			if (measure)
			{
				std::wstring strValue = measure->GetStringOrFormattedValue(AUTOSCALE_OFF, 1.0, -1, false);
				System::SetClipboardText(strValue);
			}
		}
		break;

	case IDM_COPYRANGE:
		{
			Measure* measure = getMeasure();
			if (measure)
			{
				WCHAR buffer[256];
				Measure::GetScaledValue(AUTOSCALE_ON, 1, measure->GetMinValue(), buffer, _countof(buffer));
				std::wstring range = buffer;
				range += L"- ";  // GetScaledValue returns an extra space
				Measure::GetScaledValue(AUTOSCALE_ON, 1, measure->GetMaxValue(), buffer, _countof(buffer));
				range += buffer;
				System::SetClipboardText(range);
			}
		}
		break;

	case IDM_COPY:
		{
			// Copy variable to clipboard
			HWND hwnd = GetControl(Id_SkinsListView);
			const int sel = ListView_GetNextItem(hwnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
			if (sel != -1)
			{
				WCHAR buffer[512] = { 0 };
				ListView_GetItemText(hwnd, sel, 0, buffer, _countof(buffer));
				std::wstring var = buffer;
				std::wstring variable;
				if (m_SkinWindow->GetParser().GetVariable(var, variable))
				{
					System::SetClipboardText(variable);
				}
			}
		}
		break;

	default:
		return 1;
	}

	return 0;
}

INT_PTR DialogDebug::TabSkins::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR nm = (LPNMHDR)lParam;
	HWND hwnd = nm->hwndFrom;
	switch (nm->code)
	{
	case LVN_KEYDOWN:
		{
			// Copy measure string value or variable to clipboard via CTRL + C
			NMLVKEYDOWN* lvkd = (NMLVKEYDOWN*)nm;
			if (lvkd->wVKey == 0x43 && IsCtrlKeyDown())
			{
				int sel = ListView_GetNextItem(hwnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
				if (sel != -1)
				{
					WCHAR buffer[512] = { 0 };
					ListView_GetItemText(hwnd, sel, 0, buffer, _countof(buffer));
					std::wstring temp = buffer;

					LVITEM lvi = { 0 };
					lvi.mask = LVIF_GROUPID;
					lvi.iItem = sel;
					lvi.iSubItem = 0;
					lvi.iGroupId = -1;
					ListView_GetItem(hwnd, &lvi);
					if (lvi.iGroupId == 0)  // It's a measure
					{
						Measure* measure = m_SkinWindow->GetMeasure(temp);
						if (measure)
						{
							const std::wstring strValue = measure->GetStringOrFormattedValue(AUTOSCALE_OFF, 1.0, -1, false);
							System::SetClipboardText(strValue);
						}
					}
					else if (lvi.iGroupId == 1)  // It's a Variable
					{
						std::wstring variable;
						if (m_SkinWindow->GetParser().GetVariable(temp, variable))
						{
							System::SetClipboardText(variable);
						}
					}
				}
			}
		}
		break;

	case NM_RCLICK:
		if (nm->idFrom == Id_SkinsListView)
		{
			const int sel = ListView_GetNextItem(hwnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
			if (sel != -1)
			{
				NMITEMACTIVATE* item = (NMITEMACTIVATE*)lParam;

				LVITEM lvi = { 0 };
				lvi.mask = LVIF_GROUPID;
				lvi.iItem = item->iItem;
				lvi.iSubItem = 0;
				lvi.iGroupId = -1;
				ListView_GetItem(hwnd, &lvi);

				static const MenuTemplate s_MeasureMenu[] =
				{
					MENU_ITEM(IDM_COPYMEASURENAME, 0),
					MENU_ITEM(IDM_COPYNUMBERVALUE, 0),
					MENU_ITEM(IDM_COPYSTRINGVALUE, 0),
					MENU_ITEM(IDM_COPYRANGE, 0)
				};

				static const MenuTemplate s_VariableMenu[] =
				{
					MENU_ITEM(IDM_COPY, IDS_CopyToClipboard)
				};

				bool isMeasure = lvi.iGroupId == 0;
				HMENU menu = MenuTemplate::CreateMenu(
					isMeasure ? s_MeasureMenu : s_VariableMenu,
					isMeasure ? _countof(s_MeasureMenu) : _countof(s_VariableMenu),
					GetString);

				if (menu)
				{
					if (isMeasure)
					{
						auto setMenuItem = [&](const UINT id, const UINT cmd) -> void
						{
							std::wstring name = GetString(id);
							name += L": ";
							name += GetString(IDS_CopyToClipboard);
							ModifyMenu(menu, cmd, MF_BYCOMMAND, cmd, name.c_str());
						};

						setMenuItem(IDS_Measure, IDM_COPYMEASURENAME);
						setMenuItem(IDS_Number, IDM_COPYNUMBERVALUE);
						setMenuItem(IDS_String, IDM_COPYSTRINGVALUE);
						setMenuItem(IDS_Range, IDM_COPYRANGE);
					}

					POINT pt = System::GetCursorPosition();

					// Show context menu
					TrackPopupMenu(
						menu,
						TPM_RIGHTBUTTON | TPM_LEFTALIGN,
						pt.x,
						pt.y,
						0,
						m_Window,
						nullptr);

					DestroyMenu(menu);
				}
			}
		}
		break;

	case NM_CUSTOMDRAW:
		return OnCustomDraw(wParam, lParam);

	default:
		return FALSE;
	}

	return TRUE;
}

INT_PTR DialogDebug::TabSkins::OnCustomDraw(WPARAM wParam, LPARAM lParam)
{
	static COLORREF disabled = GetSysColor(COLOR_GRAYTEXT);
	static COLORREF paused = GetSysColor(COLOR_INFOBK);

	NMLVCUSTOMDRAW* lvcd = (NMLVCUSTOMDRAW*)lParam;
	HWND hwnd = lvcd->nmcd.hdr.hwndFrom;

	switch (lvcd->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		SetWindowLongPtr(m_Window, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
		return TRUE;

	case CDDS_ITEMPREPAINT:
		SetWindowLongPtr(m_Window, DWLP_MSGRESULT, CDRF_NOTIFYSUBITEMDRAW);
		return TRUE;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
		{
			// Only process 1st column
			if (lvcd->iSubItem != 0) return FALSE;

			// Only process items in 1st group
			WCHAR buffer[512] = { 0 };
			LVITEM lvi = { 0 };
			lvi.mask = LVIF_GROUPID | LVIF_TEXT;
			lvi.iSubItem = 0;
			lvi.iItem = (int)lvcd->nmcd.dwItemSpec;
			lvi.pszText = buffer;
			lvi.cchTextMax = _countof(buffer);
			ListView_GetItem(hwnd, &lvi);
			if (lvi.iGroupId != 0) return FALSE;

			std::wstring name = buffer;
			Measure* measure = m_SkinWindow->GetMeasure(name);
			if (!measure) return FALSE;

			const bool isDisabled = measure->IsDisabled();
			const bool isPaused = measure->IsPaused();

			if (isPaused) lvcd->clrTextBk = paused;
			if (isDisabled) lvcd->clrText = disabled;

			if (isDisabled || isPaused)
			{
				SetWindowLongPtr(m_Window, DWLP_MSGRESULT, CDRF_NEWFONT);
				return TRUE;
			}
		}
		break;
	}

	return FALSE;
}

// -----------------------------------------------------------------------------------------------
//
//                                Plugins tab
//
// -----------------------------------------------------------------------------------------------

DialogDebug::TabPlugins::TabPlugins() : Tab()
{
}

void DialogDebug::TabPlugins::Create(HWND owner)
{
	Tab::CreateTabWindow(15, 30, 570, 338, owner);

	static const Control s_Controls[] =
	{
		Control::ListView(Id_PluginsListView, 0,
			0, 0, 568, 338,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER, 0,
			Control::ANCHOR_ALL)
	};

	CreateControls(s_Controls, _countof(s_Controls), GetString);
}

void DialogDebug::TabPlugins::Initialize()
{
	// Add columns to the list view
	HWND item = GetControl(Id_PluginsListView);
	ListView_SetExtendedListViewStyleEx(item, 0, LVS_EX_INFOTIP | LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	LVGROUP lvg = { 0 };
	lvg.cbSize = sizeof(LVGROUP);
	lvg.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
	lvg.state = lvg.stateMask = LVGS_NORMAL | LVGS_COLLAPSIBLE;
	lvg.iGroupId = 0;
	lvg.pszHeader = (WCHAR*)GetString(IDS_ExternalPlugins);
	ListView_InsertGroup(item, 0, &lvg);
	lvg.iGroupId = 1;
	lvg.pszHeader = (WCHAR*)GetString(IDS_BuiltInPlugins);
	ListView_InsertGroup(item, 1, &lvg);

	ListView_EnableGroupView(item, TRUE);

	LVCOLUMN lvc = { 0 };
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;  // left-aligned column
	lvc.iSubItem = 0;
	lvc.cx = m_ControlTemplate.ScaleDialogUnits(140);
	lvc.pszText = (WCHAR*)GetString(IDS_Name);
	ListView_InsertColumn(item, 0, &lvc);
	lvc.iSubItem = 1;
	lvc.cx = m_ControlTemplate.ScaleDialogUnits(80);
	lvc.pszText = (WCHAR*)GetString(IDS_Version);
	ListView_InsertColumn(item, 1, &lvc);
	lvc.iSubItem = 2;
	lvc.cx = m_ControlTemplate.ScaleDialogUnits(250);  // Resized later
	lvc.pszText = (WCHAR*)GetString(IDS_Author);
	ListView_InsertColumn(item, 2, &lvc);

	LVITEM vitem = { 0 };
	vitem.mask = LVIF_TEXT | LVIF_GROUPID;
	vitem.iItem = 0;
	vitem.iSubItem = 0;

	int index = 0;

	auto findPlugins = [&](const std::wstring& pluginPath) -> void
	{
		std::wstring filter = pluginPath + L"*.dll";

		WIN32_FIND_DATA fd;
		HANDLE hSearch = FindFirstFile(filter.c_str(), &fd);
		if (hSearch == INVALID_HANDLE_VALUE)
		{
			return;
		}

		do
		{
			// Try to get the version and author
			std::wstring tmpSz = pluginPath + fd.cFileName;
			const WCHAR* path = tmpSz.c_str();

			WORD imageBitness = 0U;
			if (!FileUtil::GetBinaryFileBitness(path, imageBitness))
			{
				LogErrorF(L"Debug Dialog - Unable to load plugin: %s", fd.cFileName);
				continue;
			}

#ifdef _WIN64
			const WORD rainmeterBitness = IMAGE_FILE_MACHINE_AMD64;
#elif _WIN32
			const WORD rainmeterBitness = IMAGE_FILE_MACHINE_I386;
#endif

			if (rainmeterBitness != imageBitness)
			{
				LogErrorF(L"Debug Dialog - Incorrect bitness of plugin: %s", fd.cFileName);
				continue;
			}

			vitem.iItem = index;
			vitem.pszText = fd.cFileName;

			// Try to get version and author from file resources first
			DWORD handle = 0UL;
			DWORD versionSize = GetFileVersionInfoSize(path, &handle);
			if (versionSize)
			{
				bool found = false;
				void* data = new BYTE[versionSize];
				if (GetFileVersionInfo(path, 0, versionSize, data))
				{
					UINT len = 0U;
					struct LANGCODEPAGE
					{
						WORD wLanguage;
						WORD wCodePage;
					} *lcp = nullptr;

					if (VerQueryValue(data, L"\\VarFileInfo\\Translation", (LPVOID*)&lcp, &len))
					{
						WCHAR key[64] = { 0 };
						LPWSTR value = nullptr;

						_snwprintf_s(key, _TRUNCATE, L"\\StringFileInfo\\%04x%04x\\ProductName", lcp[0].wLanguage, lcp[0].wCodePage);
						if (VerQueryValue(data, (LPTSTR)(LPCTSTR)key, (void**)&value, &len) &&
							wcscmp(value, L"Rainmeter") == 0)
						{
							ListView_InsertItem(item, &vitem);
							++index;
							found = true;

							_snwprintf_s(key, _TRUNCATE, L"\\StringFileInfo\\%04x%04x\\FileVersion", lcp[0].wLanguage, lcp[0].wCodePage);
							if (VerQueryValue(data, (LPTSTR)(LPCTSTR)key, (void**)&value, &len))
							{
								ListView_SetItemText(item, vitem.iItem, 1, value);
							}

							_snwprintf_s(key, _TRUNCATE, L"\\StringFileInfo\\%04x%04x\\LegalCopyright", lcp[0].wLanguage, lcp[0].wCodePage);
							if (VerQueryValue(data, (LPTSTR)(LPCTSTR)key, (void**)&value, &len))
							{
								ListView_SetItemText(item, vitem.iItem, 2, value);
							}
						}
					}
				}

				delete [] data;
				data = nullptr;
				if (found) continue;
			}

			// Try old calling GetPluginVersion/GetPluginAuthor for backwards compatibility
			DWORD err = 0;
			HMODULE dll = System::RmLoadLibrary(path, &err);
			if (dll)
			{
				ListView_InsertItem(item, &vitem);
				++index;

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
						ListView_SetItemText(item, vitem.iItem, 2, (WCHAR*)author);
					}
				}

				FreeLibrary(dll);
			}
			else
			{
				LogErrorF(L"Debug Dialog - Unable to load plugin: %s (%u)", tmpSz.c_str(), err);
			}
		}
		while (FindNextFile(hSearch, &fd));
		FindClose(hSearch);
	};

	vitem.iGroupId = 1;
	findPlugins(GetRainmeter().GetPluginPath());

	// Add old plugins
	for (const auto oldDefaultPlugin : GetRainmeter().GetOldDefaultPlugins())
	{
		vitem.iItem = index;
		vitem.pszText = (LPWSTR)oldDefaultPlugin;
		ListView_InsertItem(item, &vitem);
		++index;
	}

	if (GetRainmeter().HasUserPluginPath())
	{
		vitem.iGroupId = 0;
		findPlugins(GetRainmeter().GetUserPluginPath());
	}

	// Force first column to fit contents
	ListView_SetColumnWidth(item, 0, LVSCW_AUTOSIZE);

	// Start 3rd column at max width
	RECT rc;
	GetClientRect(m_Window, &rc);
	Relayout(rc.right, rc.bottom);

	m_Initialized = true;
}

void DialogDebug::TabPlugins::Relayout(int w, int h)
{
	Tab::Relayout(w, h);

	HWND item = GetControl(Id_PluginsListView);
	SetWindowPos(item, nullptr, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);

	// Adjust third colum
	LVCOLUMN lvc = { 0 };
	lvc.mask = LVCF_WIDTH;
	lvc.cx = w - m_ControlTemplate.ScaleDialogUnits(20) -
		(ListView_GetColumnWidth(item, 0) +
		 ListView_GetColumnWidth(item, 1));
	ListView_SetColumn(item, 2, &lvc);
}

void DialogDebug::TabPlugins::HandleDpiChange()
{
	HWND list = GetControl(Id_PluginsListView);
	ListView_SetColumnWidth(list, 0, LVSCW_AUTOSIZE);
	ListView_SetColumnWidth(list, 1, m_ControlTemplate.ScaleDialogUnits(80));

	RECT rect;
	GetClientRect(m_Window, &rect);
	Relayout(rect.right, rect.bottom);
}

INT_PTR DialogDebug::TabPlugins::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
		return OnNotify(wParam, lParam);
	}

	return FALSE;
}

INT_PTR DialogDebug::TabPlugins::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR nm = (LPNMHDR)lParam;
	switch (nm->code)
	{
	case NM_CUSTOMDRAW:
		return OnCustomDraw(wParam, lParam);

	default:
		return FALSE;
	}

	return TRUE;
}

INT_PTR DialogDebug::TabPlugins::OnCustomDraw(WPARAM wParam, LPARAM lParam)
{
	static const COLORREF disabled = GetSysColor(COLOR_GRAYTEXT);

	NMLVCUSTOMDRAW* lvcd = (NMLVCUSTOMDRAW*)lParam;
	HWND hwnd = lvcd->nmcd.hdr.hwndFrom;

	switch (lvcd->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		SetWindowLongPtr(m_Window, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
		return TRUE;

	case CDDS_ITEMPREPAINT:
		SetWindowLongPtr(m_Window, DWLP_MSGRESULT, CDRF_NOTIFYSUBITEMDRAW);
		return TRUE;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
		{
			// Only process 1st column
			if (lvcd->iSubItem != 0) return FALSE;

			// Only process items in 2nd group
			WCHAR buffer[512] = { 0 };
			LVITEM lvi = { 0 };
			lvi.mask = LVIF_GROUPID | LVIF_TEXT;
			lvi.iSubItem = 0;
			lvi.iItem = (int)lvcd->nmcd.dwItemSpec;
			lvi.pszText = buffer;
			lvi.cchTextMax = _countof(buffer);
			ListView_GetItem(hwnd, &lvi);
			if (lvi.iGroupId != 1) return FALSE;

			for (const auto oldDefaultPlugin : GetRainmeter().GetOldDefaultPlugins())
			{
				if (_wcsicmp(buffer, oldDefaultPlugin) == 0)
				{
					lvcd->clrText = disabled;
					SetWindowLongPtr(m_Window, DWLP_MSGRESULT, CDRF_NEWFONT);
					return TRUE;
				}
			}
		}
		break;
	}

	return FALSE;
}
