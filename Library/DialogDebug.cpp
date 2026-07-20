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
#include "MeasureScript.h"
#include "resource.h"
#include "DialogDebug.h"
#include "../Common/FileUtil.h"
#include "../Common/DirectoryWatcher.h"
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
		if (c_WindowPlacement.length == 0)
		{
			GetRainmeter().ReadDialogWindowPlacement(L"DebugDialogBounds", c_WindowPlacement);
		}

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

void DialogDebug::OpenSkin(Skin* skin)
{
	Open(1);

	if (c_Dialog && c_Dialog->m_TabSkins.IsInitialized())
	{
		c_Dialog->m_TabSkins.SelectSkin(skin);
	}
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
			GetRainmeter().SaveDialogWindowPlacement(L"DebugDialogBounds", c_WindowPlacement);
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

	static const Control s_Controls[] =
	{
		Control::ListView(Id_LogListView, 0,
			0, 22, 570, 315,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | LVS_ICON | LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER, 0,
			Control::ANCHOR_ALL),
		Control::Button(Id_LogMenuButton, IDS_Type,
			335, 0, 75, 14,
			WS_VISIBLE | WS_TABSTOP, 0, Control::ANCHOR_RIGHT | Control::ANCHOR_TOP),
		Control::Button(Id_LogFileMenuButton, 0,
			415, 0, 75, 14,
			WS_VISIBLE | WS_TABSTOP, 0, Control::ANCHOR_RIGHT | Control::ANCHOR_TOP),
		Control::Button(Id_ClearButton, IDS_Clear,
			495, 0, 75, 14,
			WS_VISIBLE | WS_TABSTOP, 0, Control::ANCHOR_RIGHT | Control::ANCHOR_TOP)
	};

	CreateControls(s_Controls, _countof(s_Controls), GetString);
}

/*
** Called when tab is displayed.
**
*/
void DialogDebug::TabLog::Initialize()
{
	Dialog::SetMenuButton(GetControl(Id_LogMenuButton));
	Dialog::SetMenuButton(GetControl(Id_LogFileMenuButton));
	SetWindowText(GetControl(Id_LogFileMenuButton), L"Log file");

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

	// Adjust 4th colum
	HWND item = GetControl(Id_LogListView);
	LVCOLUMN lvc = { 0 };
	lvc.mask = LVCF_WIDTH;
	lvc.cx = w - m_ControlTemplate.ScaleDialogUnits(30) -
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
	LVITEM vitem = { 0 };
	vitem.mask = LVIF_IMAGE | LVIF_TEXT;
	vitem.iItem = 0;
	vitem.iSubItem = 0;
	HWND item = nullptr;

	switch (level)
	{
	case Logger::Level::Error:
		if (!m_Error) return;
		vitem.pszText = (WCHAR*)GetString(IDS_Error);
		vitem.iImage = 0;
		break;

	case Logger::Level::Warning:
		if (!m_Warning) return;
		vitem.pszText = (WCHAR*)GetString(IDS_Warning);
		vitem.iImage = 1;
		break;

	case Logger::Level::Notice:
		if (!m_Notice) return;
		vitem.pszText = (WCHAR*)GetString(IDS_Notice);
		vitem.iImage = 2;
		break;

	case Logger::Level::Debug:
		if (!m_Debug) return;
		vitem.pszText = (WCHAR*)GetString(IDS_Debug);
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
	case Id_LogMenuButton:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			static const MenuTemplate s_LogMenu[] =
			{
				MENU_ITEM(Id_ErrorMenuItem, IDS_Error),
				MENU_ITEM(Id_WarningMenuItem, IDS_Warning),
				MENU_ITEM(Id_NoticeMenuItem, IDS_Notice),
				MENU_ITEM(Id_DebugMenuItem, IDS_Debug),
				MENU_SEPARATOR(),
				MENU_ITEM(Id_DebugModeMenuItem, IDS_DebugMode)
			};

			HMENU menu = MenuTemplate::CreateMenu(s_LogMenu, _countof(s_LogMenu), GetString);
			CheckMenuItem(menu, Id_ErrorMenuItem, MF_BYCOMMAND | (m_Error ? MF_CHECKED : MF_UNCHECKED));
			CheckMenuItem(menu, Id_WarningMenuItem, MF_BYCOMMAND | (m_Warning ? MF_CHECKED : MF_UNCHECKED));
			CheckMenuItem(menu, Id_NoticeMenuItem, MF_BYCOMMAND | (m_Notice ? MF_CHECKED : MF_UNCHECKED));
			CheckMenuItem(menu, Id_DebugMenuItem, MF_BYCOMMAND | (m_Debug ? MF_CHECKED : MF_UNCHECKED));
			CheckMenuItem(menu, Id_DebugModeMenuItem,
				MF_BYCOMMAND | (GetRainmeter().GetDebug() ? MF_CHECKED : MF_UNCHECKED));

			RECT r;
			GetWindowRect((HWND)lParam, &r);
			TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_LEFTALIGN,
				GetRainmeter().IsLanguageRTL() ? r.right : r.left, --r.bottom, 0, m_Window, nullptr);
			DestroyMenu(menu);
			SetFocus(GetControl(Id_LogListView));
		}
		break;

	case Id_ErrorMenuItem:
		m_Error = !m_Error;
		break;

	case Id_WarningMenuItem:
		m_Warning = !m_Warning;
		break;

	case Id_NoticeMenuItem:
		m_Notice = !m_Notice;
		break;

	case Id_DebugMenuItem:
		m_Debug = !m_Debug;
		break;

	case Id_DebugModeMenuItem:
		GetRainmeter().SetDebug(!GetRainmeter().GetDebug());
		break;

	case Id_LogFileMenuButton:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			static const MenuTemplate s_LogFileMenu[] =
			{
				MENU_ITEM(Id_StartLoggingMenuItem, IDS_StartLogging),
				MENU_ITEM(Id_StopLoggingMenuItem, IDS_StopLogging),
				MENU_SEPARATOR(),
				MENU_ITEM(Id_ShowLogFileMenuItem, IDS_ShowLogFile),
				MENU_ITEM(Id_DeleteLogFileMenuItem, IDS_DeleteLogFile)
			};

			HMENU menu = MenuTemplate::CreateMenu(s_LogFileMenu, _countof(s_LogFileMenu), GetString);
			if (_waccess_s(GetLogger().GetLogFilePath().c_str(), 0) != 0)
			{
				EnableMenuItem(menu, Id_ShowLogFileMenuItem, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(menu, Id_DeleteLogFileMenuItem, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(menu, Id_StopLoggingMenuItem, MF_BYCOMMAND | MF_GRAYED);
			}
			else
			{
				EnableMenuItem(menu,
					GetLogger().IsLogToFile() ? Id_StartLoggingMenuItem : Id_StopLoggingMenuItem,
					MF_BYCOMMAND | MF_GRAYED);
			}

			RECT r;
			GetWindowRect((HWND)lParam, &r);
			TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_LEFTALIGN,
				GetRainmeter().IsLanguageRTL() ? r.right : r.left, --r.bottom, 0, m_Window, nullptr);
			DestroyMenu(menu);
			SetFocus(GetControl(Id_LogListView));
		}
		break;

	case Id_ShowLogFileMenuItem:
		GetRainmeter().ShowLogFile();
		break;

	case Id_StartLoggingMenuItem:
		GetLogger().StartLogFile();
		break;

	case Id_StopLoggingMenuItem:
		GetLogger().StopLogFile();
		break;

	case Id_DeleteLogFileMenuItem:
		GetLogger().DeleteLogFile();
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
//                                Skins tab
//
// -----------------------------------------------------------------------------------------------

class DialogDebug::TabSkins::PanelWatch : public Dialog
{
public:
	enum Id
	{
		Id_StringRadio = 1000,
		Id_FormulaRadio,
		Id_ExpressionTypeLabel,
		Id_ExplanationLabel,
		Id_Edit,
		Id_Result,
		Id_ExecuteButton,
		Id_AddButton,
		Id_CancelButton
	};

	PanelWatch(TabSkins& owner) : m_Owner(owner), m_EditIndex((size_t)-1) {}

	void Open(HWND parent)
	{
		m_EditIndex = (size_t)-1;

		ShowDialogWindow(
			L"Add Watch",
			0, 0, 400, 176,
			DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU,
			WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT,
			parent);

		SetWindowText(GetControl(Id_AddButton), L"Add");
		SetWindowText(GetControl(Id_Edit), L"");
		UpdateResult();
	}

	void Edit(HWND parent, size_t index, const std::wstring& text, bool formula)
	{
		m_EditIndex = index;

		ShowDialogWindow(
			L"Edit Watch",
			0, 0, 400, 176,
			DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU,
			WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT,
			parent);

		Button_SetCheck(GetControl(Id_StringRadio), formula ? BST_UNCHECKED : BST_CHECKED);
		Button_SetCheck(GetControl(Id_FormulaRadio), formula ? BST_CHECKED : BST_UNCHECKED);
		SetWindowText(GetControl(Id_AddButton), L"Save");
		SetWindowText(GetControl(Id_Edit), text.c_str());
		UpdateLabel();
		UpdateResult();
	}

	bool AppendExpression(const std::wstring& name, bool measure)
	{
		if (!m_Window || !IsWindowVisible(m_Window)) return false;

		std::wstring text;
		if (measure)
		{
			text = IsFormula() ? name : L"[" + name + L"]";
		}
		else
		{
			text = L"#" + name + L"#";
		}

		HWND item = GetControl(Id_Edit);
		SendMessage(item, EM_REPLACESEL, TRUE, (LPARAM)text.c_str());
		SetFocus(item);
		return true;
	}

	void UpdateResult()
	{
		HWND item = GetControl(Id_Edit);
		if (!item) return;

		const int length = GetWindowTextLength(item);
		std::wstring text(length + 1, L'\0');
		GetWindowText(item, &text[0], length + 1);
		text.resize(length);
		const std::wstring result = Evaluate(text, IsFormula());

		HWND resultItem = GetControl(Id_Result);
		const auto firstVisibleLine = SendMessage(resultItem, EM_GETFIRSTVISIBLELINE, 0, 0);
		SendMessage(resultItem, WM_SETREDRAW, FALSE, 0);
		SetWindowText(resultItem, result.c_str());
		const auto updatedFirstVisibleLine = SendMessage(resultItem, EM_GETFIRSTVISIBLELINE, 0, 0);
		SendMessage(resultItem, EM_LINESCROLL, 0, firstVisibleLine - updatedFirstVisibleLine);
		SendMessage(resultItem, WM_SETREDRAW, TRUE, 0);
		RedrawWindow(resultItem, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);

		EnableWindow(GetControl(Id_ExecuteButton), m_Owner.m_SkinWindow && result.starts_with(L"[!"));
	}

	std::wstring Evaluate(std::wstring text, bool formula)
	{
		Skin* skin = m_Owner.m_SkinWindow;
		if (skin && !text.empty())
		{
			ConfigParser& parser = skin->GetParser();
			parser.ReplaceVariables(text, true);
			parser.ReplaceMeasures(text);

			if (formula)
			{
				double value = 0.0;
				const WCHAR* error = skin->GetMathParser().CheckedParse(text.c_str(), &value);
				if (error)
				{
					text = L"Formula failed to parse: ";
					text += error;
				}
				else
				{
					WCHAR buffer[128] = { 0 };
					int bufferLen = _snwprintf_s(buffer, _TRUNCATE, L"%lf", value);
					Measure::RemoveTrailingZero(buffer, bufferLen);
					text = buffer;
				}
			}
		}

		return text;
	}

protected:
	INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		const INT_PTR baseResult = Dialog::HandleMessage(uMsg, wParam, lParam);
		switch (uMsg)
		{
		case WM_INITDIALOG:
			return OnInitDialog();
		case WM_COMMAND:
			return OnCommand(wParam, lParam);
		case WM_CLOSE:
			ShowWindow(m_Window, SW_HIDE);
			return TRUE;
		}
		return baseResult;
	}

private:
	INT_PTR OnInitDialog()
	{
		static const Control controls[] =
		{
			Control::Label(Id_ExpressionTypeLabel, 0,
				6, 6, 78, 10,
				WS_VISIBLE, 0),
			Control::RadioButton(Id_StringRadio, 0,
				88, 4, 50, 14,
				WS_VISIBLE | WS_TABSTOP | WS_GROUP, 0),
			Control::RadioButton(Id_FormulaRadio, 0,
				142, 4, 58, 14,
				WS_VISIBLE | WS_TABSTOP, 0),
			Control::Label(Id_ExplanationLabel, 0,
				6, 24, 388, 10,
				WS_VISIBLE, 0),
			Control::Edit(Id_Edit, 0,
				6, 40, 388, 50,
				WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_WANTRETURN, 0),
			Control::Edit(Id_Result, 0,
				6, 96, 388, 50,
				WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY, 0),
			Control::Button(Id_ExecuteButton, 0,
				157, 156, 75, 14,
				WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
			Control::Button(Id_AddButton, 0,
				238, 156, 75, 14,
				WS_VISIBLE | WS_TABSTOP, 0),
			Control::Button(Id_CancelButton, 0,
				319, 156, 75, 14,
				WS_VISIBLE | WS_TABSTOP, 0)
		};

		CreateControls(controls, _countof(controls), GetString);
		SetWindowText(GetControl(Id_ExpressionTypeLabel), L"Expression type:");
		SetWindowText(GetControl(Id_StringRadio), L"String");
		Button_SetCheck(GetControl(Id_StringRadio), BST_CHECKED);
		SetWindowText(GetControl(Id_FormulaRadio), L"Formula");
		SetWindowText(GetControl(Id_ExecuteButton), L"Execute");
		SetWindowText(GetControl(Id_AddButton), L"Add");
		SetWindowText(GetControl(Id_CancelButton), L"Cancel");
		UpdateLabel();
		return TRUE;
	}

	void UpdateLabel()
	{
		const WCHAR* text = Button_GetCheck(GetControl(Id_FormulaRadio)) == BST_CHECKED ?
			L"Enter a string to evaluate math formula:" : L"Enter a string to evaluate variables and measures:";
		SetWindowText(GetControl(Id_ExplanationLabel), text);
	}

	bool IsFormula()
	{
		return Button_GetCheck(GetControl(Id_FormulaRadio)) == BST_CHECKED;
	}

	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam)
	{
		switch (LOWORD(wParam))
		{
		case Id_Edit:
			if (HIWORD(wParam) == EN_CHANGE) UpdateResult();
			break;

		case Id_StringRadio:
		case Id_FormulaRadio:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				UpdateLabel();
				UpdateResult();
			}
			break;

		case Id_ExecuteButton:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				HWND result = GetControl(Id_Result);
				const int length = GetWindowTextLength(result);
				if (length > 0)
				{
					std::wstring command(length + 1, L'\0');
					GetWindowText(result, &command[0], length + 1);
					command.resize(length);
					if (command.starts_with(L"[!"))
					{
						GetRainmeter().ExecuteCommand(command.c_str(), m_Owner.m_SkinWindow);
					}
				}
			}
			break;

		case Id_AddButton:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				HWND edit = GetControl(Id_Edit);
				const int length = GetWindowTextLength(edit);
				if (length > 0)
				{
					std::wstring text(length + 1, L'\0');
					GetWindowText(edit, &text[0], length + 1);
					text.resize(length);
					if (m_EditIndex == (size_t)-1)
					{
						m_Owner.AddWatch(text, IsFormula());
					}
					else
					{
						m_Owner.SaveWatch(m_EditIndex, text, IsFormula());
					}
					m_EditIndex = (size_t)-1;
					SetWindowText(edit, L"");
					ShowWindow(m_Window, SW_HIDE);
				}
			}
			break;

		case Id_CancelButton:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				m_EditIndex = (size_t)-1;
				SetWindowText(GetControl(Id_Edit), L"");
				ShowWindow(m_Window, SW_HIDE);
			}
			break;

		default:
			return FALSE;
		}

		return TRUE;
	}

	TabSkins& m_Owner;
	size_t m_EditIndex;
};

DialogDebug::TabSkins::TabSkins() : Tab(),
	m_SkinWindow(),
	m_AutoRefresh(false),
	m_DirectoryWatcher(std::make_unique<DirectoryWatcher>()),
	m_PanelWatch(std::make_unique<PanelWatch>(*this))
{
}

DialogDebug::TabSkins::~TabSkins() = default;

void DialogDebug::TabSkins::Create(HWND owner)
{
	Tab::CreateTabWindow(15, 30, 570, 338, owner);

	static const Control s_Controls[] =
	{
		Control::Button(Id_SelectSkinButton, 0,
			0, 0, 220, 14,
			WS_VISIBLE | WS_TABSTOP, 0,
			Control::ANCHOR_LEFT | Control::ANCHOR_TOP),
		Control::CheckBox(Id_AutoRefreshCheckBox, 0,
			226, 0, 150, 14,
			WS_VISIBLE | WS_TABSTOP, 0,
			Control::ANCHOR_RIGHT | Control::ANCHOR_TOP),
		Control::Button(Id_SkinMenuButton, 0,
			415, 0, 75, 14,
			WS_VISIBLE | WS_TABSTOP, 0,
			Control::ANCHOR_RIGHT | Control::ANCHOR_TOP),
		Control::Button(Id_AddWatchButton, 0,
			495, 0, 75, 14,
			WS_VISIBLE | WS_TABSTOP, 0,
			Control::ANCHOR_RIGHT | Control::ANCHOR_TOP),
		Control::ListView(Id_SkinsListView, 0,
			0, 22, 570, 315,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER, 0,
			Control::ANCHOR_ALL)
	};

	CreateControls(s_Controls, _countof(s_Controls), GetString);
}

void DialogDebug::TabSkins::Initialize()
{
	Dialog::SetMenuButton(GetControl(Id_SelectSkinButton));
	Dialog::SetMenuButton(GetControl(Id_SkinMenuButton));

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
	lvg.iGroupId = 2;
	lvg.pszHeader = (WCHAR*)L"Watch";
	ListView_InsertGroup(item, 2, &lvg);

	ListView_EnableGroupView(item, TRUE);

	LVCOLUMN lvc = { 0 };
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.iSubItem = 0;
	lvc.cx = m_ControlTemplate.ScaleDialogUnits(160);
	lvc.pszText = (WCHAR*)GetString(IDS_Name);
	ListView_InsertColumn(item, 0, &lvc);
	lvc.iSubItem = 1;
	lvc.cx = m_ControlTemplate.ScaleDialogUnits(90);
	lvc.pszText = (WCHAR*)GetString(IDS_Number);
	ListView_InsertColumn(item, 1, &lvc);
	lvc.iSubItem = 2;
	lvc.cx = m_ControlTemplate.ScaleDialogUnits(110);  // Resized later
	lvc.pszText = (WCHAR*)GetString(IDS_String);
	ListView_InsertColumn(item, 2, &lvc);

	// Start 3rd column at max width
	RECT rc;
	GetClientRect(m_Window, &rc);
	Relayout(rc.right, rc.bottom);
	SetWindowText(GetControl(Id_SelectSkinButton), L"Select skin...");
	SetWindowText(GetControl(Id_AutoRefreshCheckBox), L"Auto refresh on edit");
	Button_SetCheck(GetControl(Id_AutoRefreshCheckBox), m_AutoRefresh ? BST_CHECKED : BST_UNCHECKED);
	SetWindowText(GetControl(Id_SkinMenuButton), L"Skin");
	SetWindowText(GetControl(Id_AddWatchButton), L"Add watch...");

	UpdateSkinList();

	m_Initialized = true;
}

void DialogDebug::TabSkins::Relayout(int w, int h)
{
	Tab::Relayout(w, h);

	// Adjust 3rd column
	HWND item = GetControl(Id_SkinsListView);
	LVCOLUMN lvc = { 0 };
	lvc.mask = LVCF_WIDTH;
	lvc.cx = w - m_ControlTemplate.ScaleDialogUnits(24) -
		(ListView_GetColumnWidth(item, 0) +
		 ListView_GetColumnWidth(item, 1));
	ListView_SetColumn(item, 2, &lvc);
}

void DialogDebug::TabSkins::HandleDpiChange()
{
	HWND list = GetControl(Id_SkinsListView);
	ListView_SetColumnWidth(list, 0, m_ControlTemplate.ScaleDialogUnits(160));
	ListView_SetColumnWidth(list, 1, m_ControlTemplate.ScaleDialogUnits(90));

	RECT rect;
	GetClientRect(m_Window, &rect);
	Relayout(rect.right, rect.bottom);
}

void DialogDebug::TabSkins::SelectSkin(Skin* skin)
{
	m_SkinWindow = skin;
	UpdateSkinList();
	UpdateMeasureList(m_SkinWindow);
}

/*
** Updates the list of skins.
**
*/
void DialogDebug::TabSkins::UpdateSkinList()
{
	Button_Enable(GetControl(Id_SelectSkinButton), !GetRainmeter().GetAllSkins().empty());

	bool found = false;
	std::wstring skinName;
	for (const auto& iter : GetRainmeter().GetAllSkins())
	{
		if (!found && m_SkinWindow == iter.second)
		{
			found = true;
			skinName = iter.first;
		}
	}

	if (!found)
	{
		if (GetRainmeter().GetAllSkins().empty())
		{
			m_SkinWindow = nullptr;
			HWND item = GetControl(Id_SkinsListView);
			ListView_DeleteAllItems(item);
		}
		else
		{
			// Default to first skin
			m_SkinWindow = GetRainmeter().GetAllSkins().begin()->second;
			skinName = GetRainmeter().GetAllSkins().begin()->first;
			UpdateMeasureList(m_SkinWindow);
		}
	}

	Button_Enable(GetControl(Id_SkinMenuButton), m_SkinWindow != nullptr);

	HWND button = GetControl(Id_SelectSkinButton);
	SetWindowText(button, skinName.empty() ? L"Select Skin..." : skinName.c_str());
	RedrawWindow(button, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
	UpdateDirectoryWatcher();
}

void DialogDebug::TabSkins::UpdateDirectoryWatcher()
{
	m_DirectoryWatcher->Stop();
	if (!m_AutoRefresh || !m_SkinWindow) return;

	m_AutoRefreshFiles = m_SkinWindow->GetParser().GetIniFiles();
	for (Measure* measure : m_SkinWindow->GetMeasures())
	{
		if (measure->GetTypeID() == TypeID<MeasureScript>())
		{
			const std::wstring& file = static_cast<MeasureScript*>(measure)->GetScriptFile();
			if (!file.empty()) m_AutoRefreshFiles.push_back(file);
		}
	}

	m_DirectoryWatcher->Start(m_SkinWindow->GetRootPath(), true, OnDirectoryChange, this);
}

void DialogDebug::TabSkins::OnDirectoryChange(const WCHAR* path, void* context)
{
	TabSkins* tab = static_cast<TabSkins*>(context);
	const auto& files = tab->m_AutoRefreshFiles;
	if (std::find_if(files.begin(), files.end(), [&](const std::wstring& file)
		{ return _wcsicmp(file.c_str(), path) == 0; }) != files.end())
	{
		PostMessage(tab->m_SkinWindow->GetWindow(), WM_METERWINDOW_DELAYED_REFRESH, 0, 0);
	}
}

void DialogDebug::TabSkins::AddWatch(const std::wstring& text, bool formula)
{
	m_Watches.push_back({ text, formula });
	UpdateMeasureList(m_SkinWindow);
	EnsureWatchVisible(m_Watches.size() - 1);
}

void DialogDebug::TabSkins::EditWatch(size_t index)
{
	if (index < m_Watches.size())
	{
		const Watch& watch = m_Watches[index];
		m_PanelWatch->Edit(GetParent(m_Window), index, watch.text, watch.formula);
	}
}

void DialogDebug::TabSkins::SaveWatch(size_t index, const std::wstring& text, bool formula)
{
	if (index < m_Watches.size())
	{
		m_Watches[index] = { text, formula };
		UpdateMeasureList(m_SkinWindow);
		EnsureWatchVisible(index);
	}
}

void DialogDebug::TabSkins::DeleteWatch(size_t index)
{
	if (index < m_Watches.size())
	{
		m_Watches.erase(m_Watches.begin() + index);
		UpdateMeasureList(m_SkinWindow);
	}
}

size_t DialogDebug::TabSkins::GetSelectedWatch()
{
	HWND item = GetControl(Id_SkinsListView);
	const int selected = ListView_GetNextItem(item, -1, LVNI_FOCUSED | LVNI_SELECTED);
	if (selected != -1)
	{
		LVITEM lvi = { 0 };
		lvi.mask = LVIF_GROUPID | LVIF_PARAM;
		lvi.iItem = selected;
		if (ListView_GetItem(item, &lvi) && lvi.iGroupId == 2)
		{
			for (size_t i = 0; i < m_Watches.size(); ++i)
			{
				if ((const WCHAR*)lvi.lParam == m_Watches[i].text.c_str())
				{
					return i;
				}
			}
		}
	}

	return (size_t)-1;
}

void DialogDebug::TabSkins::EnsureWatchVisible(size_t index)
{
	if (index >= m_Watches.size()) return;

	HWND item = GetControl(Id_SkinsListView);
	const WCHAR* watch = m_Watches[index].text.c_str();
	LVITEM lvi = { 0 };
	lvi.mask = LVIF_PARAM;
	for (lvi.iItem = 0; lvi.iItem < ListView_GetItemCount(item); ++lvi.iItem)
	{
		ListView_GetItem(item, &lvi);
		if ((const WCHAR*)lvi.lParam == watch)
		{
			ListView_SetItemState(item, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
			ListView_SetItemState(item, lvi.iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
			ListView_EnsureVisible(item, lvi.iItem, FALSE);

			RECT rect;
			if (ListView_GetItemRect(item, lvi.iItem, &rect, LVIR_BOUNDS))
			{
				ListView_Scroll(item, 0, rect.bottom - rect.top);
			}
			break;
		}
	}
}

/*
** Updates the list of measures, variables, and watches.
**
*/
void DialogDebug::TabSkins::UpdateMeasureList(Skin* skin)
{
	if (!skin)
	{
		if (!m_SkinWindow)
		{
			return;
		}
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

		WCHAR buffer[256];
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

		ListView_SetItemText(item, lvi.iItem, 1, (WCHAR*)numValue.c_str());
		ListView_SetItemText(item, lvi.iItem, 2, (WCHAR*)strValue.c_str());
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
		ListView_SetItemText(item, lvi.iItem, 2, (WCHAR*)valStr.c_str());
		++lvi.iItem;
	}

	lvi.iGroupId = 2;
	for (const auto& watch : m_Watches)
	{
		lvi.pszText = (WCHAR*)watch.text.c_str();
		lvi.lParam = (LPARAM)watch.text.c_str();

		if (lvi.iItem < count)
		{
			ListView_SetItem(item, &lvi);
		}
		else
		{
			ListView_InsertItem(item, &lvi);
		}

		std::wstring result = m_PanelWatch->Evaluate(watch.text, watch.formula);
		if (result.length() > 259)
		{
			result.erase(256);
			result += L"...";
		}

		ListView_SetItemText(item, lvi.iItem, 1, (WCHAR*)L"");
		ListView_SetItemText(item, lvi.iItem, 2, (WCHAR*)result.c_str());
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

	UINT state = selIndex == -1 ? 0 : LVIS_FOCUSED | LVIS_SELECTED;

	// Re-select previously selected item (or deselect group header)
	ListView_SetItemState(item, selIndex, state, LVIS_FOCUSED | LVIS_SELECTED);

	SendMessage(item, WM_SETREDRAW, TRUE, 0);

	m_PanelWatch->UpdateResult();
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
	case Id_SelectSkinButton:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			HMENU menu = CreatePopupMenu();
			int index = 0;
			for (const auto& iter : GetRainmeter().GetAllSkins())
			{
				InsertMenu(menu, index, MF_BYPOSITION, ID_CONFIG_FIRST + index, iter.first.c_str());
				++index;
			}

			if (index > 0)
			{
				RECT r;
				GetWindowRect((HWND)lParam, &r);
				TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_LEFTALIGN,
					GetRainmeter().IsLanguageRTL() ? r.right : r.left, --r.bottom, 0, m_Window, nullptr);
			}

			DestroyMenu(menu);
		}
		break;

	case Id_AddWatchButton:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			m_PanelWatch->Open(GetParent(m_Window));
			m_PanelWatch->UpdateResult();
		}
		break;

	case Id_SkinMenuButton:
		if (HIWORD(wParam) == BN_CLICKED && m_SkinWindow)
		{
			RECT r;
			GetWindowRect((HWND)lParam, &r);
			GetRainmeter().ShowContextMenu({ r.left, r.bottom }, m_SkinWindow);
		}
		break;

	case Id_AutoRefreshCheckBox:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			m_AutoRefresh = !m_AutoRefresh;
			UpdateDirectoryWatcher();
		}
		break;

	case IDM_SKIN_REFRESH:
		if (m_SkinWindow)
		{
			SendMessage(m_SkinWindow->GetWindow(), WM_COMMAND, IDM_SKIN_REFRESH, 0);
		}
		break;

	case IDM_EDITWATCH:
		{
			const size_t index = GetSelectedWatch();
			if (index != (size_t)-1)
			{
				EditWatch(index);
			}
		}
		break;

	case IDM_DELETEWATCH:
		{
			const size_t index = GetSelectedWatch();
			if (index != (size_t)-1)
			{
				DeleteWatch(index);
			}
		}
		break;

	case IDM_ADD_WATCH:
		{
			HWND hwnd = GetControl(Id_SkinsListView);
			const int sel = ListView_GetNextItem(hwnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
			if (sel != -1)
			{
				WCHAR buffer[512] = { 0 };
				ListView_GetItemText(hwnd, sel, 0, buffer, _countof(buffer));

				LVITEM lvi = { 0 };
				lvi.mask = LVIF_GROUPID;
				lvi.iItem = sel;
				lvi.iGroupId = -1;
				ListView_GetItem(hwnd, &lvi);

				if (lvi.iGroupId == 0)
				{
					AddWatch(L"[" + std::wstring(buffer) + L"]", false);
				}
				else if (lvi.iGroupId == 1)
				{
					AddWatch(L"#" + std::wstring(buffer) + L"#", false);
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
		if (wParam >= ID_CONFIG_FIRST && wParam <= ID_CONFIG_LAST)
		{
			int index = (int)wParam - ID_CONFIG_FIRST;
			for (const auto& iter : GetRainmeter().GetAllSkins())
			{
				if (index-- == 0)
				{
					SelectSkin(iter.second);
					SetFocus(GetControl(Id_SkinsListView));
					return 0;
				}
			}
		}
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
					MENU_ITEM_GRAYED(0, 0),
					MENU_ITEM_GRAYED(0, 0),
					MENU_SEPARATOR(),
					MENU_ITEM(IDM_ADD_WATCH, 0),
					MENU_SEPARATOR(),
					MENU_ITEM(IDM_COPYMEASURENAME, 0),
					MENU_ITEM(IDM_COPYNUMBERVALUE, 0),
					MENU_ITEM(IDM_COPYSTRINGVALUE, 0)
				};

				static const MenuTemplate s_VariableMenu[] =
				{
					MENU_ITEM(IDM_ADD_WATCH, 0),
					MENU_SEPARATOR(),
					MENU_ITEM(IDM_COPY, IDS_CopyToClipboard)
				};

				static const MenuTemplate s_WatchMenu[] =
				{
					MENU_ITEM(IDM_EDITWATCH, IDS_Edit),
					MENU_ITEM(IDM_DELETEWATCH, IDS_Delete)
				};

				bool isMeasure = lvi.iGroupId == 0;
				bool isWatch = lvi.iGroupId == 2;
				const MenuTemplate* menuTemplate = isMeasure ? s_MeasureMenu :
					(isWatch ? s_WatchMenu : s_VariableMenu);
				const UINT menuSize = isMeasure ? _countof(s_MeasureMenu) :
					(isWatch ? _countof(s_WatchMenu) : _countof(s_VariableMenu));
				HMENU menu = MenuTemplate::CreateMenu(
					menuTemplate,
					menuSize,
					GetString);

				if (menu)
				{
					if (!isWatch)
					{
						ModifyMenu(menu, IDM_ADD_WATCH, MF_BYCOMMAND, IDM_ADD_WATCH, L"Add watch");
					}

					if (isMeasure)
					{
						WCHAR buffer[512] = { 0 };
						ListView_GetItemText(hwnd, item->iItem, 0, buffer, _countof(buffer));
						Measure* measure = m_SkinWindow->GetMeasure(buffer);
						if (measure)
						{
							auto getRangeValue = [](double value) -> std::wstring
							{
								WCHAR buffer[256];
								Measure::GetScaledValue(AUTOSCALE_ON, 1, value, buffer, _countof(buffer));
								std::wstring text = buffer;
								if (!text.empty() && text.back() == L' ') text.pop_back();
								return text;
							};

							std::wstring minValue = L"Min value: ";
							minValue += getRangeValue(measure->GetMinValue());
							ModifyMenu(menu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED, 0, minValue.c_str());

							std::wstring maxValue = L"Max value: ";
							maxValue += getRangeValue(measure->GetMaxValue());
							ModifyMenu(menu, 1, MF_BYPOSITION | MF_STRING | MF_GRAYED, 0, maxValue.c_str());
						}

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

	case NM_DBLCLK:
		if (nm->idFrom == Id_SkinsListView)
		{
			const size_t index = GetSelectedWatch();
			if (index != (size_t)-1)
			{
				EditWatch(index);
			}
			else
			{
				const int sel = ListView_GetNextItem(hwnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
				if (sel != -1)
				{
					WCHAR buffer[512] = { 0 };
					ListView_GetItemText(hwnd, sel, 0, buffer, _countof(buffer));

					LVITEM lvi = { 0 };
					lvi.mask = LVIF_GROUPID;
					lvi.iItem = sel;
					lvi.iGroupId = -1;
					ListView_GetItem(hwnd, &lvi);

					if ((lvi.iGroupId == 0 || lvi.iGroupId == 1) &&
						!m_PanelWatch->AppendExpression(buffer, lvi.iGroupId == 0))
					{
						OnCommand(IDM_ADD_WATCH, 0);
					}
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

			WORD imageBitness = 0;
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
			DWORD handle = 0;
			DWORD versionSize = GetFileVersionInfoSize(path, &handle);
			if (versionSize)
			{
				bool found = false;
				void* data = new BYTE[versionSize];
				if (GetFileVersionInfo(path, 0, versionSize, data))
				{
					UINT len = 0;
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
