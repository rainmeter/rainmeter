/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "../Common/MenuTemplate.h"
#include "../Common/PathUtil.h"
#include "Rainmeter.h"
#include "System.h"
#include "Util.h"
#include "resource.h"
#include "DialogNewSkin.h"
#include "DialogManage.h"

DialogNewSkin::CloseType DialogNewSkin::c_CloseAction = { false, 0 };
std::vector<std::wstring> DialogNewSkin::c_Templates;
WINDOWPLACEMENT DialogNewSkin::c_WindowPlacement = { 0 };
DialogNewSkin* DialogNewSkin::c_Dialog = nullptr;

DialogNewSkin::DialogNewSkin() : Dialog()
{
}

DialogNewSkin::~DialogNewSkin()
{
}

/*
** Opens by tab index
**
*/
void DialogNewSkin::Open(int tab)
{
	if (!c_Dialog)
	{
		c_Dialog = new DialogNewSkin();
	}

	c_Dialog->ShowDialogWindow(
		GetString(ID_STR_CREATENEWSKIN),
		0, 0, 300, 250,
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

void DialogNewSkin::Open(const WCHAR* tabName, const WCHAR* parent)
{
	bool isOpen = c_Dialog ? true : false;

	Open(tabName);

	if (!isOpen && c_Dialog)
	{
		// "New" tab
		if (_wcsicmp(tabName, L"New") == 0)
		{
			// |parent| represents the config (ie. "illustro\Clock")
			c_Dialog->m_TabNew.SetParentFolder(parent);
		}
	}
}

void DialogNewSkin::Open(const WCHAR* name)
{
	int tab = 0;

	if (name)
	{
		if (_wcsicmp(name, L"Template") == 0)
		{
			tab = 1;
		}
	}

	Open(tab);
}

void DialogNewSkin::ShowNewSkinDialog()
{
	if (!c_Dialog)
	{
		Open();
	}
}

Dialog::Tab& DialogNewSkin::GetActiveTab()
{
	int sel = TabCtrl_GetCurSel(GetControl(Id_Tab));
	if (sel == 0)
	{
		return m_TabNew;
	}
	else // if (sel == 1)
	{
		return m_TabTemplate;
	}
}

INT_PTR DialogNewSkin::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			if (c_CloseAction.rootFolder && c_CloseAction.skins == 0)
			{
				// If a root folder was created (without any skins being created), attempt to create a skin using
				// the selected template using the name of the root folder as the name of the skin. If the selected
				// template is not longer valid, attempt to create a the skin using the default template.
				const size_t pos = GetRainmeter().GetSkinPath().size();
				std::wstring folder = c_Dialog->m_TabNew.GetParentFolder();
				std::wstring file = folder.substr(pos);
				PathUtil::RemoveTrailingBackslash(file);
				file += L".ini";
				folder += file;

				const std::wstring& selectedTemplate = c_Dialog->m_TabNew.GetSelectedTemplate();
				if (!selectedTemplate.empty())
				{
					// Copy template file
					bool templateExists = true;
					std::wstring templateFile = GetTemplateFolder();
					templateFile += selectedTemplate;
					templateFile += L".template";
					if (_waccess(templateFile.c_str(), 0) == -1)
					{
						// Template file doesn't exist, so ask user if they would like a default template instead.
						templateExists = false;

						const std::wstring text = GetFormattedString(ID_STR_TEMPLATEDOESNOTEXIST, selectedTemplate.c_str());
						if (GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONQUESTION | MB_YESNO) != IDYES)
						{
							// Cancel
							break;
						}

						templateFile = folder;
						c_Dialog->m_TabTemplate.CreateTemplate(templateFile);
						if (templateFile.empty())
						{
							// Could not create the skin using the default template
							const std::wstring text = GetFormattedString(ID_STR_CREATEFILEFAIL, file.c_str());
							GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONERROR | MB_OK);
							break;
						}
					}

					// If the template exists, copy the template file to new location
					if (templateExists)
					{
						if (!System::CopyFiles(templateFile, folder))
						{
							// Could not create the skin using the selected template
							const std::wstring text = GetFormattedString(ID_STR_CREATEFILEFAIL, file.c_str());
							GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONERROR | MB_OK);
							break;
						}
					}
				}
				else
				{
					// Use default template
					c_Dialog->m_TabTemplate.CreateTemplate(folder);
					if (folder.empty())
					{
						// Could not create the skin
						const std::wstring text = GetFormattedString(ID_STR_CREATEFILEFAIL, file.c_str());
						GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONERROR | MB_OK);
						break;
					}
				}

				++c_CloseAction.skins; // increase skin count
			}

			// If any skins were created, refresh Rainmeter and select parent folder in the Manage dialog
			if (c_CloseAction.skins > 0)
			{
				GetRainmeter().RefreshAll();
				const size_t pos = GetRainmeter().GetSkinPath().size();
				const std::wstring folder = c_Dialog->m_TabNew.GetParentFolder().substr(pos);
				DialogManage::Open(L"Skins", folder.c_str(), nullptr);
			}

			// Reset any close action
			c_CloseAction = { false, 0 };

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

INT_PTR DialogNewSkin::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	static const ControlTemplate::Control s_Controls[] =
	{
		CT_BUTTON(Id_CloseButton, ID_STR_CLOSE,
			243, 231, 50, 14,
			WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0),
		CT_TAB(Id_Tab, 0,
			6, 6, 288, 221,
			WS_VISIBLE | WS_TABSTOP | TCS_FIXEDWIDTH, 0)  // Last for correct tab order.
	};

	CreateControls(s_Controls, _countof(s_Controls), m_Font, GetString);

	// Load template filenames
	LoadTemplates();

	HWND item = GetControl(Id_Tab);
	m_TabNew.Create(m_Window);
	m_TabTemplate.Create(m_Window);

	TCITEM tci = {0};
	tci.mask = TCIF_TEXT;
	tci.pszText = GetString(ID_STR_NEWSKIN);
	TabCtrl_InsertItem(item, 0, &tci);
	tci.pszText = GetString(ID_STR_TEMPLATE);
	TabCtrl_InsertItem(item, 1, &tci);

	HICON hIcon = GetIcon(IDI_RAINMETER);
	SendMessage(m_Window, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	item = GetControl(Id_CloseButton);
	SendMessage(m_Window, WM_NEXTDLGCTL, (WPARAM)item, TRUE);

	// Use arrows instead of plus/minus in the tree
	item = m_TabNew.GetControl(TabNew::Id_ItemsTreeView);
	SetWindowTheme(item, L"explorer", nullptr);

	if (c_WindowPlacement.length == 0)
	{
		c_WindowPlacement.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(m_Window, &c_WindowPlacement);
	}

	SetWindowPlacement(m_Window, &c_WindowPlacement);

	return TRUE;
}

INT_PTR DialogNewSkin::OnCommand(WPARAM wParam, LPARAM lParam)
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

INT_PTR DialogNewSkin::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR nm = (LPNMHDR)lParam;
	switch (nm->idFrom)
	{
	case Id_Tab:
		if (nm->code == TCN_SELCHANGE)
		{
			// Disable all tab windows first
			EnableWindow(m_TabNew.GetWindow(), FALSE);
			EnableWindow(m_TabTemplate.GetWindow(), FALSE);

			GetActiveTab().Activate();
		}
		break;

	default:
		return 1;
	}

	return 0;
}

const std::wstring& DialogNewSkin::GetTemplateFolder()
{
	static std::wstring& folder = GetRainmeter().GetSettingsPath() + L"Templates\\";
	return folder;
}

void DialogNewSkin::LoadTemplates()
{
	c_Templates.clear();

	const auto& templateFolder = GetTemplateFolder();

	// If |NewSkin.template| exists in the settings path, move to Templates folder
	const std::wstring oldTemplate = GetRainmeter().GetSettingsPath() + L"NewSkin.template";
	if (_waccess(oldTemplate.c_str(), 0) != -1)
	{
		// Move file to new location
		const std::wstring newTemplate = templateFolder + L"NewSkin.template";
		System::CopyFiles(oldTemplate, newTemplate, true);
	}

	// Add template files
	std::wstring files = templateFolder + L"*.template";
	WIN32_FIND_DATA fd;
	HANDLE hSearch = FindFirstFile(files.c_str(), &fd);
	if (hSearch != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (_wcsicmp(PathFindExtension(fd.cFileName), L".template") == 0)
			{
				PathRemoveExtension(fd.cFileName);
				c_Templates.emplace_back(fd.cFileName);
			}
		}
		while (FindNextFile(hSearch, &fd));

		FindClose(hSearch);
	}
}

void DialogNewSkin::ValidateSelectedTemplate()
{
	std::wstring& selected = c_Dialog->m_TabNew.GetSelectedTemplate();
	bool found = false;
	for (const auto& iter : c_Templates)
	{
		if (_wcsicmp(iter.c_str(), selected.c_str()) == 0)
		{
			// Selected template is valid
			found = true;
			break;
		}
	}

	if (!found)
	{
		selected.clear();
	}

	WritePrivateProfileString(
		L"Dialog_NewSkin",
		L"SelectedTemplate",
		selected.empty() ? NULL : selected.c_str(),
		GetRainmeter().GetDataFile().c_str());
}

// -----------------------------------------------------------------------------------------------
//
//                                New tab
//
// -----------------------------------------------------------------------------------------------

std::vector<DialogNewSkin::TabNew::SortInfo> DialogNewSkin::TabNew::s_SortInfo;

DialogNewSkin::TabNew::TabNew() : Tab(),
	m_IsRoot(true),
	m_CanAddResourcesFolder(false),
	m_InRenameMode(false)
{
}

void DialogNewSkin::TabNew::Create(HWND owner)
{
	Tab::CreateTabWindow(15, 30, 270, 188, owner);

	short buttonWidth = (short)_wtoi(GetString(ID_STR_NUM_BUTTONWIDTH));
	buttonWidth += 10;
	short column1 = (268 - buttonWidth);

	static const ControlTemplate::Control s_Controls[] =
	{
		CT_LABEL(Id_ParentPathLabel, ID_STR_ELLIPSIS,
			0, 0, 268, 14,
			WS_VISIBLE | SS_CENTERIMAGE | SS_PATHELLIPSIS | SS_NOTIFY | WS_BORDER, 0),
		CT_TREEVIEW(Id_ItemsTreeView, 0,
			0, 19, 268 - buttonWidth - 10, 169,
			WS_VISIBLE | WS_TABSTOP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS | WS_VSCROLL, WS_EX_CLIENTEDGE),
		CT_BUTTON(Id_AddFolderButton, ID_STR_ADDFOLDER,
			column1, 19, buttonWidth, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		CT_BUTTON(Id_AddResourcesButton, ID_STR_ADDRESOURCES,
			column1, 38, buttonWidth, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_BUTTON(Id_AddSkinButton, ID_STR_ADDSKIN,
			column1, 57, buttonWidth, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_BUTTON(Id_TemplateDropDownList, ID_STR_TEMPLATEE,
			column1, 76, buttonWidth, 14,
			WS_VISIBLE | WS_TABSTOP, 0)
	};

	CreateControls(s_Controls, _countof(s_Controls), c_Dialog->m_Font, GetString);

	// Create the tooltip for the parent path
	m_ParentPathTT = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		m_Window, NULL,
		GetModuleHandle(NULL), NULL);

	// Add tooltip to window
	UpdateParentPathTT(false);

	// Get selected template for drop down menu
	WCHAR buffer[MAX_PATH];
	GetPrivateProfileString(L"Dialog_NewSkin", L"SelectedTemplate", L"", buffer, MAX_PATH, GetRainmeter().GetDataFile().c_str());
	if (buffer && *buffer)
	{
		m_SelectedTemplate = buffer;
		ValidateSelectedTemplate();
	}
}

void DialogNewSkin::TabNew::Initialize()
{
	// Draw arrow on button
	HWND item = GetControl(Id_TemplateDropDownList);
	Dialog::SetMenuButton(item);

	// Load folder/.ini icons from shell32
	HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_COLOR32, 2, 10);
	HMODULE hDLL = GetModuleHandle(L"shell32");
	HICON hIcon = (HICON)LoadImage(hDLL, MAKEINTRESOURCE(4), IMAGE_ICON, 16, 16, LR_SHARED);
	ImageList_AddIcon(hImageList, hIcon);
	hIcon = (HICON)LoadImage(hDLL, MAKEINTRESOURCE(151), IMAGE_ICON, 16, 16, LR_SHARED);
	ImageList_AddIcon(hImageList, hIcon);

	// Apply icons and populate tree
	item = GetControl(Id_ItemsTreeView);
	TreeView_SetImageList(item, hImageList, TVSIL_NORMAL);

	m_Initialized = true;
}

INT_PTR DialogNewSkin::TabNew::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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

INT_PTR DialogNewSkin::TabNew::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case Id_AddFolderButton:
		AddTreeItem(true);
		break;

	case Id_AddSkinButton:
		AddTreeItem(false);
		break;

	case Id_AddResourcesButton:
		{
			HWND tree = GetControl(Id_ItemsTreeView);
			TVINSERTSTRUCT tvi = { 0 };
			tvi.hInsertAfter = TVI_FIRST;
			tvi.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_PARAM;
			tvi.item.iImage = tvi.item.iSelectedImage = 0;
			tvi.item.pszText = L"@Resources";
			tvi.item.state = tvi.item.stateMask = TVIS_BOLD;
			tvi.hParent = TreeView_GetRoot(tree);

			s_SortInfo.emplace_back(true, L"@Resources");
			tvi.item.lParam = (LPARAM)(size_t)(s_SortInfo.size() - 1);

			TreeView_InsertItem(tree, &tvi);

			TreeView_Expand(tree, tvi.hParent, TVE_EXPAND);
			TreeView_Select(tree, tvi.hParent, TVGN_CARET);

			std::wstring resources = m_ParentFolder;
			resources += L"@Resources\\";
			BOOL exists = CreateDirectory(resources.c_str(), NULL);
			if (!exists && GetLastError() == ERROR_PATH_NOT_FOUND)
			{
				const std::wstring text = GetFormattedString(ID_STR_CREATEFOLDERFAIL, L"@Resources");
				GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONERROR | MB_OK);
				TreeView_DeleteItem(tree, tvi.item.hItem);
				return 0;
			}

			Button_Enable(GetControl(Id_AddResourcesButton), FALSE);
			m_CanAddResourcesFolder = false;
		}
		break;

	case Id_TemplateDropDownList:
		{
			static const MenuTemplate s_Menu[] =
			{
				MENU_ITEM(IDM_DEFAULT_TEMPLATE, ID_STR_USEDEFAULTTEMPLATE)
			};

			HMENU menu = MenuTemplate::CreateMenu(s_Menu, _countof(s_Menu), GetString);
			if (menu)
			{
				TabTemplate::CreateTemplateMenu(menu, m_SelectedTemplate);

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

	case IDM_NEWSKINMENU_EXPAND:
		{
			HWND tree = GetControl(Id_ItemsTreeView);
			HTREEITEM item = TreeView_GetSelection(tree);
			TreeView_Expand(tree, item, TVE_TOGGLE);
		}
		break;

	case IDM_NEWSKINMENU_OPENFOLDER:
		{
			HWND tree = GetControl(Id_ItemsTreeView);
			std::wstring folder = m_ParentFolder + GetTreeSelectionPath(tree, false);
			PathUtil::AppendBackslashIfMissing(folder);
			CommandHandler::RunFile(folder.c_str());
		}
		break;

	case IDM_NEWSKINMENU_EDITFOLDERNAME:
	case IDM_NEWSKINMENU_EDITFILENAME:
		{
			m_InRenameMode = true;

			HWND tree = GetControl(Id_ItemsTreeView);

			// Make tree nodes editable
			LONG_PTR style = GetWindowLongPtr(tree, GWL_STYLE) | TVS_EDITLABELS;
			SetWindowLongPtr(tree, GWL_STYLE, style);

			// Enter 'edit mode' of treeview edit control
			TreeView_EditLabel(tree, TreeView_GetSelection(tree));
		}
		break;

	case IDM_NEWSKINMENU_DELETEFOLDER:
		{
			HWND tree = GetControl(Id_ItemsTreeView);
			std::wstring folder = m_ParentFolder + GetTreeSelectionPath(tree, false);
			PathUtil::AppendBackslashIfMissing(folder);

			std::wstring text = GetFormattedString(ID_STR_FOLDERDELETE, folder.c_str());
			if (GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONQUESTION | MB_YESNO) != IDYES)
			{
				// Cancel
				break;
			}

			if (!System::RemoveFolder(folder))
			{
				text = GetFormattedString(ID_STR_FOLDERDELETEFAIL, folder.c_str());
				GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONERROR | MB_OK);
				return 0;
			}

			HTREEITEM item = TreeView_GetSelection(tree);

			// Subtract any newly created skins in the folder and its subfolders.
			// Note: This only checks .ini skin files, not .inc files. Also any folders
			//       should only contain newly created items, not existing items.
			UINT count = GetChildSkinCount(tree, item);
			if (count > c_CloseAction.skins)
			{
				c_CloseAction.skins = 0;
			}
			else
			{
				c_CloseAction.skins -= count;
			}

			if (item == TreeView_GetRoot(tree))
			{
				// If deleting the 'root' item, all child skins and folders should be deleted,
				// so no need to refresh Rainmeter or create any skins when the dialog is closed.
				c_CloseAction = { false, 0 };

				// Update the parent folder and label
				m_ParentFolder = GetRainmeter().GetSkinPath();
				UpdateParentPathLabel();

				EnableWindow(GetControl(Id_AddResourcesButton), FALSE);
				EnableWindow(GetControl(Id_AddSkinButton), FALSE);
			}

			TreeView_DeleteItem(tree, TreeView_GetSelection(tree));
		}
		break;

	case IDM_NEWSKINMENU_DELETEFILE:
		{
			HWND tree = GetControl(Id_ItemsTreeView);
			std::wstring file = m_ParentFolder + GetTreeSelectionPath(tree, true);

			std::wstring text = GetFormattedString(ID_STR_FILEDELETE, file.c_str());
			if (GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONQUESTION | MB_YESNO) != IDYES)
			{
				// Cancel
				break;
			}

			if (!System::RemoveFile(file))
			{
				text = GetFormattedString(ID_STR_FOLDERDELETEFAIL, file.c_str());
				GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONERROR | MB_OK);
				return 0;
			}

			TreeView_DeleteItem(tree, TreeView_GetSelection(tree));
			--c_CloseAction.skins;
		}
		break;

	case IDM_NEWSKINMENU_EDITSKIN:
		{
			HWND tree = GetControl(Id_ItemsTreeView);
			const std::wstring file = m_ParentFolder + GetTreeSelectionPath(tree, true);
			CommandHandler::RunFile(GetRainmeter().GetSkinEditor().c_str(), file.c_str());
		}
		break;

	default:
		if (wParam == IDM_DEFAULT_TEMPLATE)
		{
			m_SelectedTemplate.clear();
			ValidateSelectedTemplate();
			break;
		}
		else if (wParam >= ID_TEMPLATE_FIRST && wParam <= ID_TEMPLATE_LAST)
		{
			size_t index = (size_t)wParam - ID_TEMPLATE_FIRST;
			if (index < c_Templates.size())
			{
				m_SelectedTemplate = c_Templates[index];
			}
			else
			{
				m_SelectedTemplate.clear();
			}

			ValidateSelectedTemplate();
			break;
		}

		return 1;
	}

	return 0;
}

INT_PTR DialogNewSkin::TabNew::OnNotify(WPARAM wParam, LPARAM lParam)
{
	auto enterEditMode = [](HWND tree, HTREEITEM item) -> void
	{
		// Make tree nodes editable
		LONG_PTR style = GetWindowLongPtr(tree, GWL_STYLE) | TVS_EDITLABELS;
		SetWindowLongPtr(tree, GWL_STYLE, style);

		// Enter 'edit mode' of treeview edit control
		TreeView_EditLabel(tree, item);
	};

	LPNMHDR nm = (LPNMHDR)lParam;
	switch (nm->code)
	{
	case NM_DBLCLK:
		if (nm->idFrom == Id_ItemsTreeView)
		{
			POINT pt = System::GetCursorPosition();

			TVHITTESTINFO ht;
			ht.pt = pt;
			ScreenToClient(nm->hwndFrom, &ht.pt);

			if (TreeView_HitTest(nm->hwndFrom, &ht) && !(ht.flags & TVHT_NOWHERE))
			{
				TreeView_SelectItem(nm->hwndFrom, ht.hItem);

				TVITEM tvi = { 0 };
				tvi.hItem = TreeView_GetSelection(nm->hwndFrom);
				tvi.mask = TVIF_IMAGE;

				if (TreeView_GetItem(nm->hwndFrom, &tvi))
				{
					if (tvi.iImage == 1)
					{
						// Double click on a .ini or .inc file
						OnCommand(MAKEWPARAM(IDM_NEWSKINMENU_EDITSKIN, 0), 0);
					}
				}
			}
		}
		break;

	case NM_RCLICK:
		if (nm->idFrom == Id_ItemsTreeView)
		{
			POINT pt = System::GetCursorPosition();

			TVHITTESTINFO ht;
			ht.pt = pt;
			ScreenToClient(nm->hwndFrom, &ht.pt);

			if (TreeView_HitTest(nm->hwndFrom, &ht) && !(ht.flags & TVHT_NOWHERE))
			{
				TreeView_SelectItem(nm->hwndFrom, ht.hItem);

				WCHAR buffer[MAX_PATH];

				TVITEM tvi = { 0 };
				tvi.hItem = TreeView_GetSelection(nm->hwndFrom);
				tvi.mask = TVIF_STATE | TVIF_IMAGE | TVIF_CHILDREN | TVIF_TEXT;
				tvi.pszText = buffer;
				tvi.cchTextMax = MAX_PATH;

				if (TreeView_GetItem(nm->hwndFrom, &tvi))
				{
					bool isNewItem = (tvi.state & TVIS_BOLD) != 0;
					HMENU menu = nullptr;

					if (tvi.iImage == 0)
					{
						// Folder menu.
						static const MenuTemplate s_Menu[] =
						{
							MENU_ITEM(IDM_NEWSKINMENU_EXPAND, ID_STR_EXPAND),
							MENU_ITEM(IDM_NEWSKINMENU_OPENFOLDER, ID_STR_OPENFOLDER)
						};

						MENUITEMINFO mii = { 0 };
						mii.cbSize = sizeof(MENUITEMINFO);
						mii.fMask = MIIM_STRING;

						menu = MenuTemplate::CreateMenu(s_Menu, _countof(s_Menu), GetString);
						SetMenuDefaultItem(menu, IDM_NEWSKINMENU_EXPAND, MF_BYCOMMAND);

						// Allow for renaming of new items *only* if not in the skin registry.
						// Also, do not allow editing of the @Resources folder.
						if (isNewItem && (_wcsicmp(buffer, L"@Resources") != 0))
						{
							std::wstring folder = m_ParentFolder + GetTreeSelectionPath(nm->hwndFrom, false);

							// Remove trailing slash (not needed for skin registry)
							PathUtil::RemoveTrailingBackslash(folder);

							folder = folder.substr(GetRainmeter().GetSkinPath().size());
							if (!GetRainmeter().m_SkinRegistry.FindFolder(folder))
							{
								std::wstring text = GetString(ID_STR_RENAME);
								AppendMenu(menu, MF_BYPOSITION, IDM_NEWSKINMENU_EDITFOLDERNAME, text.c_str());
								text = GetString(ID_STR_DELETE);
								AppendMenu(menu, MF_BYPOSITION, IDM_NEWSKINMENU_DELETEFOLDER, text.c_str());
							}
						}

						if (tvi.state & TVIS_EXPANDED)
						{
							mii.dwTypeData = GetString(ID_STR_COLLAPSE);
							SetMenuItemInfo(menu, IDM_NEWSKINMENU_EXPAND, MF_BYCOMMAND, &mii);
						}

						// Disable 'Expand' if folder has no children
						if (tvi.cChildren == 0)
						{
							EnableMenuItem(menu, IDM_NEWSKINMENU_EXPAND, MF_BYCOMMAND | MF_GRAYED);
						}
					}
					else
					{
						// Skin menu.
						static const MenuTemplate s_Menu[] =
						{
							MENU_ITEM(IDM_NEWSKINMENU_EDITSKIN, ID_STR_EDITSKIN)
						};

						menu = MenuTemplate::CreateMenu(s_Menu, _countof(s_Menu), GetString);
						SetMenuDefaultItem(menu, IDM_NEWSKINMENU_EDITSKIN, MF_BYCOMMAND);

						// Allow for renaming of new items *only* if not in the skin registry
						if (isNewItem)
						{
							std::wstring folder = m_ParentFolder + GetTreeSelectionPath(nm->hwndFrom, false);
							folder = folder.substr(GetRainmeter().GetSkinPath().size());

							// Remove trailing slash (not needed for skin registry)
							PathUtil::RemoveTrailingBackslash(folder);

							const std::wstring file = buffer;
							const SkinRegistry::Indexes indexes =
								GetRainmeter().m_SkinRegistry.FindIndexes(folder, file);
							if (!indexes.IsValid())
							{
								std::wstring text = GetString(ID_STR_RENAME);
								AppendMenu(menu, MF_BYPOSITION, IDM_NEWSKINMENU_EDITFILENAME, text.c_str());
								text = GetString(ID_STR_DELETE);
								AppendMenu(menu, MF_BYPOSITION, IDM_NEWSKINMENU_DELETEFILE, text.c_str());
							}
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

	case TVN_BEGINLABELEDIT:
		{
			HWND tree = GetControl(Id_ItemsTreeView);
			m_TreeEdit = TreeView_GetEditControl(tree);

			// Install subclass to capture key strokes for edit control
			SetWindowSubclass(m_TreeEdit, &TreeEditSubclass, 1, 0);
			SetFocus(m_TreeEdit);

			// Fake set the text in the edit control, so that |TVN_ENDLABELEDIT| notification sees it
			LPNMTVDISPINFO info = (LPNMTVDISPINFO)lParam;
			SetWindowText(m_TreeEdit, info->item.pszText);

			// Only select the skin name, not the extension
			const size_t len = wcslen(info->item.pszText);
			const WCHAR* ext = PathFindExtension(info->item.pszText);
			if (len > 4 && ((_wcsicmp(ext, L".ini") == 0) || (_wcsicmp(ext, L".inc") == 0)))
			{
				Edit_SetSel(m_TreeEdit, 0, len - 4);
			}
		}
		break;

	case TVN_ENDLABELEDIT:
		{
			RemoveWindowSubclass(m_TreeEdit, &TreeEditSubclass, 1);

			HWND tree = GetControl(Id_ItemsTreeView);
			LPNMTVDISPINFO info = (LPNMTVDISPINFO)lParam;

			// Make tree nodes non-editable
			LONG_PTR style = GetWindowLongPtr(tree, GWL_STYLE) & ~TVS_EDITLABELS;
			SetWindowLongPtr(tree, GWL_STYLE, style);

			// The items text will be |NULL| if the edit control was cancelled.
			// So delete the item from the tree unless the item is being renamed.
			if (!info->item.pszText)
			{
				if (!m_InRenameMode)
				{
					TreeView_DeleteItem(tree, info->item.hItem);
				}

				m_InRenameMode = false;
				return FALSE;
			}

			const UINT count = TreeView_GetCount(tree);

			// Make sure the field is not empty
			std::wstring name = info->item.pszText;
			if (name.empty())
			{
				enterEditMode(tree, info->item.hItem);
				return TRUE;
			}

			// Determine if the item is a folder or skin file (.ini)
			TVITEM tvitem = { 0 };
			tvitem.hItem = info->item.hItem;
			tvitem.mask = TVIF_IMAGE | TVIF_TEXT | TVIF_PARAM;
			TreeView_GetItem(tree, &tvitem);

			const bool isFolder = tvitem.iImage == 0;

			// Find out if item already exists in tree
			if (DoesNodeExist(info->item.hItem, info->item.pszText, isFolder))
			{
				const UINT msg = isFolder ? ID_STR_FOLDEREXISTS : ID_STR_FILEEXISTS;
				const std::wstring text = GetFormattedString(msg, name.c_str());
				GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONWARNING | MB_OK);
				enterEditMode(tree, info->item.hItem);
				return TRUE;
			}

			std::wstring newItem = m_ParentFolder;

			// Add selection to item
			newItem += GetTreeSelectionPath(tree, false);
			PathUtil::AppendBackslashIfMissing(newItem);

			if (isFolder)
			{
				if (m_InRenameMode)
				{
					// |newItem| is now the 'old' path, so replace the last folder
					// in the path with the |name| of the new folder
					const std::wstring oldItem = newItem;
					PathUtil::RemoveTrailingBackslash(newItem);

					const size_t pos = newItem.find_last_of(L"\\");
					if (pos == std::wstring::npos)
					{
						// Invalid path
						m_InRenameMode = false;
						const std::wstring error = std::to_wstring(GetLastError());
						const std::wstring text = GetFormattedString(ID_STR_RENAMEFOLDERFAIL, name.c_str(), error.c_str());
						GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONERROR | MB_OK);
						return FALSE;
					}

					newItem = newItem.substr(0, pos + 1);
					newItem += name;
					PathUtil::AppendBackslashIfMissing(newItem);

					// If items are the same, just return like nothing happened.
					if (wcscmp(oldItem.c_str(), newItem.c_str()) == 0)
					{
						m_InRenameMode = false;
						return TRUE;
					}

					// New folder already exists, re-enter folder name
					if (_waccess(newItem.c_str(), 0) == 0)
					{
						const std::wstring text = GetFormattedString(ID_STR_FOLDEREXISTS, name.c_str());
						GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONWARNING | MB_OK);
						enterEditMode(tree, info->item.hItem);
						return TRUE;
					}

					m_InRenameMode = false;

					BOOL success = MoveFile(oldItem.c_str(), newItem.c_str());
					if (!success)
					{
						const std::wstring error = std::to_wstring(GetLastError());
						const std::wstring text = GetFormattedString(ID_STR_RENAMEFOLDERFAIL, name.c_str(), error.c_str());
						GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONERROR | MB_OK);
						return FALSE;
					}

					// If parent folder was changed, update label
					if (info->item.hItem == TreeView_GetRoot(tree))
					{
						m_ParentFolder = newItem;
						UpdateParentPathLabel();
					}
				}
				else  // Not in rename mode
				{
					newItem += name;
					newItem += L"\\";

					// Create the folder
					if (!CreateDirectory(newItem.c_str(), NULL))
					{
						// If the folder already exists, enter edit mode to rename,
						// otherwise delete the item from the tree.
						DWORD error = GetLastError();
						if (error == ERROR_ALREADY_EXISTS)
						{
							const std::wstring text = GetFormattedString(ID_STR_FOLDEREXISTS, name.c_str());
							GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONWARNING | MB_OK);
							enterEditMode(tree, info->item.hItem);
							return TRUE;
						}
						else
						{
							const std::wstring text = GetFormattedString(ID_STR_CREATEFOLDERFAIL, name.c_str());
							GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONERROR | MB_OK);
							TreeView_DeleteItem(tree, info->item.hItem);
							return FALSE;
						}
					}

					// Change parent if folder is at root level
					if (count == 1)
					{
						m_ParentFolder = newItem;
						UpdateParentPathLabel();
						c_CloseAction.rootFolder = true;
					}
				}
			}
			else // not a folder
			{
				// Only allow .ini or .inc extensions
				const WCHAR* ext = PathFindExtension(name.c_str());
				bool isSkin = _wcsicmp(ext, L".ini") == 0;
				bool isInc = _wcsicmp(ext, L".inc") == 0;
				if (!isSkin && !isInc)
				{
					// Remove extension and add '.ini' as extension
					LPWSTR temp = &name[0];
					PathRemoveExtension(temp);
					name = temp;
					name += L".ini";
					isSkin = true;  // signal dialog that a skin was added (see below)
				}

				if (m_InRenameMode)
				{
					// Get the selected item (with filename)
					newItem = m_ParentFolder + GetTreeSelectionPath(tree, true);
					const std::wstring oldItem = newItem;

					const size_t pos = newItem.find_last_of(L"\\");
					if (pos == std::wstring::npos)
					{
						// Invalid path
						m_InRenameMode = false;
						const std::wstring error = std::to_wstring(GetLastError());
						const std::wstring text = GetFormattedString(ID_STR_RENAMEFILEFAIL, name.c_str(), error.c_str());
						GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONERROR | MB_OK);
						return FALSE;
					}

					// Remove old filename and add new filename
					newItem = newItem.substr(0, pos + 1);
					newItem += name;

					// If items are the same, just return like nothing happened.
					if (wcscmp(oldItem.c_str(), newItem.c_str()) == 0)
					{
						m_InRenameMode = false;
						return TRUE;
					}

					if (_waccess(newItem.c_str(), 0) == 0)
					{
						// File exists, enter edit mode
						const std::wstring text = GetFormattedString(ID_STR_FILEEXISTS, name.c_str());
						GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONWARNING | MB_OK);
						enterEditMode(tree, info->item.hItem);
						return TRUE;
					}

					m_InRenameMode = false;

					BOOL success = MoveFile(oldItem.c_str(), newItem.c_str());
					if (!success)
					{
						// Could not rename file
						const std::wstring error = std::to_wstring(GetLastError());
						const std::wstring text = GetFormattedString(ID_STR_RENAMEFILEFAIL, name.c_str(), error.c_str());
						GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONERROR | MB_OK);
						return FALSE;
					}

					// Do a 'refresh all' if a skin was added
					if (isSkin) ++c_CloseAction.skins;
				}
				else  // Not in rename mode
				{
					newItem += name;

					if (PathFileExists(newItem.c_str()) == TRUE)
					{
						// The skin file already exists
						const std::wstring text = GetFormattedString(ID_STR_FILEEXISTS, name.c_str());
						GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONWARNING | MB_OK);
						enterEditMode(tree, info->item.hItem);
						return TRUE;
					}

					// Create or copy the selected template. If the selected template
					// does not exist, attempt to create a default template in its place.
					if (m_SelectedTemplate.empty())
					{
						// Create default template file
						std::wstring newFile = newItem;
						c_Dialog->m_TabTemplate.CreateTemplate(newFile);
						if (newFile.empty())
						{
							// Could not create default template file
							const std::wstring text = GetFormattedString(ID_STR_CREATEFILEFAIL, name.c_str());
							GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONERROR | MB_OK);
							TreeView_DeleteItem(tree, info->item.hItem);
							return FALSE;
						}
					}
					else
					{
						// Copy template file
						bool templateExists = true;
						std::wstring templateFile = GetTemplateFolder();
						templateFile += m_SelectedTemplate;
						templateFile += L".template";
						if (_waccess(templateFile.c_str(), 0) == -1)
						{
							// Template file doesn't exist, so ask user if they would like a default template instead.
							templateExists = false;

							std::wstring text = GetFormattedString(ID_STR_TEMPLATEDOESNOTEXIST, m_SelectedTemplate.c_str());
							if (GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONQUESTION | MB_YESNO) != IDYES)
							{
								// 'No' button was pushed
								TreeView_DeleteItem(tree, info->item.hItem);
								return FALSE;
							}

							templateFile = newItem;
							c_Dialog->m_TabTemplate.CreateTemplate(templateFile);
							if (templateFile.empty())
							{
								// Could not create skin using the default template
								text = GetFormattedString(ID_STR_CREATEFILEFAIL, name.c_str());
								GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONERROR | MB_OK);
								TreeView_DeleteItem(tree, info->item.hItem);
								return FALSE;
							}
						}

						// If the template exists, copy the template file to new location
						if (templateExists)
						{
							if (!System::CopyFiles(templateFile, newItem))
							{
								// Could not create the skin using the selected template
								const std::wstring text = GetFormattedString(ID_STR_CREATEFILEFAIL, name.c_str());
								GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONERROR | MB_OK);
								TreeView_DeleteItem(tree, info->item.hItem);
								return FALSE;
							}
						}
					}

					// Do a 'refresh all' if a skin was added
					if (isSkin) ++c_CloseAction.skins;
				}
			}

			// Update item text and sorting information
			tvitem.pszText = (WCHAR*)name.c_str();
			s_SortInfo.emplace_back(isFolder, name);
			tvitem.lParam = (LPARAM)(size_t)(s_SortInfo.size() - 1);
			TreeView_SetItem(tree, &tvitem);

			// Sort items and select new item
			TVSORTCB sort = { 0 };
			sort.hParent = TreeView_GetParent(tree, info->item.hItem);
			sort.lpfnCompare = TreeViewSortProc;
			TreeView_SortChildrenCB(tree, &sort, 0);
			TreeView_Select(tree, info->item.hItem, TVGN_CARET);

			if (isFolder && count == 1)
			{
				Button_Enable(GetControl(Id_AddResourcesButton), m_CanAddResourcesFolder ? TRUE : FALSE);
				Button_Enable(GetControl(Id_AddSkinButton), TRUE);
			}
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

void DialogNewSkin::TabNew::SetParentFolder(const WCHAR* parentFolder)
{
	m_ParentFolder = GetRainmeter().GetSkinPath();
	if (parentFolder && *parentFolder)
	{
		m_ParentFolder += parentFolder;
		m_ParentFolder += L"\\";
		m_IsRoot = false;

		// Populate tree
		HWND tree = GetControl(Id_ItemsTreeView);
		TreeView_DeleteAllItems(tree);

		TVINSERTSTRUCT tvi = { 0 };
		tvi.hInsertAfter = TVI_LAST;
		tvi.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
		tvi.item.iImage = tvi.item.iSelectedImage = 0;

		if (!GetRainmeter().m_SkinRegistry.IsEmpty())
		{
			const int index = GetRainmeter().m_SkinRegistry.FindFolderIndex(parentFolder);
			PopulateTree(tree, tvi, index, true);

			// Select, expand, and focus first item in tree
			SelectTreeItem(tree, TreeView_GetRoot(tree), parentFolder);
			HTREEITEM item = TreeView_GetSelection(tree);
			TreeView_Expand(tree, item, TVE_TOGGLE);
			SetFocus(tree);
		}
	}
	else
	{
		m_CanAddResourcesFolder = true;
	}

	UpdateParentPathLabel();

	if (!m_IsRoot)
	{
		HWND item = GetControl(Id_AddSkinButton);
		Button_Enable(item, TRUE);
		item = GetControl(Id_TemplateDropDownList);
		Button_Enable(item, TRUE);
	}
}

void DialogNewSkin::TabNew::SelectTreeItem(HWND tree, HTREEITEM item, LPCWSTR name)
{
	WCHAR buffer[MAX_PATH];
	TVITEM tvi = { 0 };
	tvi.mask = TVIF_TEXT;
	tvi.hItem = item;
	tvi.pszText = buffer;

	const WCHAR* pos = wcschr(name, L'\\');
	if (pos)
	{
		const int folderLen = (int)(pos - name);
		tvi.cchTextMax = folderLen + 1;  // Length of folder name plus 1 for nullptr

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
					++pos;  // Skip the slash
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

void DialogNewSkin::TabNew::UpdateParentPathLabel()
{
	// Only display the skins folder name plus any subfolder that is selected
	const std::wstring sp = GetRainmeter().GetSkinPath();
	const size_t pos = sp.find_last_of(L'\\', sp.size() - 2);

	// Add some padding so the text does not touch the border
	std::wstring text = L" ";
	if (pos != std::wstring::npos) text += m_ParentFolder.substr(pos + 1);
	else text += m_ParentFolder;

	HWND item = GetControl(Id_ParentPathLabel);
	Static_SetText(item, text.c_str());

	// Update tooltip text
	UpdateParentPathTT(true);
}

void DialogNewSkin::TabNew::UpdateParentPathTT(bool update)
{
	if (!m_ParentPathTT) return;

	TOOLINFO toolInfo = { 0 };
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = m_Window;
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	toolInfo.uId = (UINT_PTR)GetControl(Id_ParentPathLabel);
	toolInfo.lpszText = (WCHAR*)m_ParentFolder.c_str();

	SendMessage(m_ParentPathTT, update ? TTM_UPDATETIPTEXT : TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
}

void DialogNewSkin::TabNew::AddTreeItem(bool isFolder)
{
	const std::wstring name = isFolder ? L"NewFolder" : L"NewSkin.ini";
	const std::wstring path = GetRainmeter().GetSkinPath();

	HWND tree = GetControl(Id_ItemsTreeView);
	
	TVITEM tvitem = { 0 };
	tvitem.hItem = TreeView_GetSelection(tree);
	tvitem.mask = TVIF_IMAGE;
	TreeView_GetItem(tree, &tvitem);

	if (tvitem.iImage == 1) // .ini file is selected
	{
		tvitem.hItem = TreeView_GetParent(tree, tvitem.hItem);
	}

	// Add new item to tree
	TVINSERTSTRUCT tvi = { 0 };
	tvi.hInsertAfter = tvitem.hItem;
	tvi.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_PARAM;
	tvi.item.iImage = tvi.item.iSelectedImage = isFolder ? 0 : 1;
	tvi.item.pszText = (WCHAR*)name.c_str();
	tvi.item.state = tvi.item.stateMask = TVIS_BOLD;
	tvi.hParent = tvitem.hItem;

	// Store sorting information
	s_SortInfo.emplace_back(isFolder, name);
	tvi.item.lParam = (LPARAM)(size_t)(s_SortInfo.size() - 1);

	HTREEITEM item = TreeView_InsertItem(tree, &tvi);

	// Sort items
	TVSORTCB sort = { 0 };
	sort.hParent = tvitem.hItem;
	sort.lpfnCompare = TreeViewSortProc;
	TreeView_SortChildrenCB(tree, &sort, 0);

	TreeView_Expand(tree, tvi.hParent, TVE_EXPAND);

	// Make tree nodes editable
	LONG_PTR style = GetWindowLongPtr(tree, GWL_STYLE) | TVS_EDITLABELS;
	SetWindowLongPtr(tree, GWL_STYLE, style);

	// Enter 'edit mode' of treeview edit control
	TreeView_EditLabel(tree, item);
}

bool DialogNewSkin::TabNew::DoesNodeExist(HTREEITEM item, LPCWSTR text, bool isFolder)
{
	HWND tree = GetControl(Id_ItemsTreeView);
	HTREEITEM parent = TreeView_GetParent(tree, item);
	HTREEITEM current = TreeView_GetChild(tree, parent);

	while (current != NULL && current != item)
	{
		WCHAR buffer[MAX_PATH];

		TVITEM tvi = { 0 };
		tvi.hItem = current;
		tvi.mask = TVIF_TEXT | TVIF_IMAGE;
		tvi.cchTextMax = MAX_PATH;
		tvi.pszText = buffer;
		TreeView_GetItem(tree, &tvi);

		// Test if item is of the same 'type' (folder or file), and if the names are the same
		if (((isFolder && tvi.iImage == 0) || (!isFolder && tvi.iImage == 1)) &&
			_wcsicmp(buffer, text) == 0)
		{
			return true;
		}

		current = TreeView_GetNextSibling(tree, current);
	}

	return false;
}

UINT DialogNewSkin::TabNew::GetChildSkinCount(HWND tree, HTREEITEM item)
{
	if (item == nullptr) return 0;

	UINT count = 0;
	WCHAR buffer[MAX_PATH];
	TVITEM tvi = { 0 };
	tvi.hItem = item;
	tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_CHILDREN;
	tvi.pszText = buffer;
	tvi.cchTextMax = MAX_PATH;
	TreeView_GetItem(tree, &tvi);

	if (tvi.cChildren > 0)
	{
		item = TreeView_GetChild(tree, item);
		while (item)
		{
			tvi.hItem = item;
			TreeView_GetItem(tree, &tvi);

			if (tvi.iImage == 0)  // Is a folder
			{
				count += GetChildSkinCount(tree, item);
			}
			else  // Is a file
			{
				// Only add .ini files to count
				const WCHAR* ext = PathFindExtension(buffer);
				if (_wcsicmp(ext, L".ini") == 0)
				{
					++count;
				}
			}
			item = TreeView_GetNextSibling(tree, item);
		}
	}

	return count;
}

int CALLBACK DialogNewSkin::TabNew::TreeViewSortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const size_t index1 = (size_t)lParam1;
	const size_t index2 = (size_t)lParam2;

	const size_t size = s_SortInfo.size();
	if (index1 >= size || index2 >= size) return 0; // out of bounds

	const SortInfo& item1 = s_SortInfo[index1];
	const SortInfo& item2 = s_SortInfo[index2];

	if (item1.type && !item2.type) return -1;  // item1 is a folder, item2 is a file
	if (!item1.type && item2.type) return 1;   // item1 is a file, item2 is a folder

	return wcscmp(item1.name.c_str(), item2.name.c_str());  // compare names
}

std::wstring DialogNewSkin::TabNew::GetTreeSelectionPath(HWND tree, bool allItems)
{
	WCHAR buffer[MAX_PATH];
	std::wstring path;

	// Get current selection name
	TVITEM tvi = { 0 };
	tvi.hItem = TreeView_GetSelection(tree);
	tvi.mask = TVIF_TEXT | TVIF_IMAGE;
	tvi.pszText = buffer;
	tvi.cchTextMax = MAX_PATH;
	TreeView_GetItem(tree, &tvi);

	// Only add files if necessary. Always add folders.
	if (allItems || tvi.iImage == 0)
	{
		path = buffer;
	}

	while ((tvi.hItem = TreeView_GetParent(tree, tvi.hItem)) != nullptr)
	{
		TreeView_GetItem(tree, &tvi);
		path.insert(0, 1, L'\\');
		path.insert(0, buffer);
	}

	// Remove the first folder in the path since it is already included
	// in the parent path or is about to added to it.
	const std::wstring first = path.substr(0, path.find_first_of(L"\\"));
	path.erase(0, first.length() + 1);

	return path;
}

int DialogNewSkin::TabNew::PopulateTree(HWND tree, TVINSERTSTRUCT& tvi, int index, bool isParentFolder)
{
	const int initialLevel = GetRainmeter().m_SkinRegistry.GetFolder(index).level;
	const int max = GetRainmeter().m_SkinRegistry.GetFolderCount();

	do
	{
		const auto& skinFolder = GetRainmeter().m_SkinRegistry.GetFolder(index);
		if (skinFolder.level != initialLevel)
		{
			return index - 1;
		}

		HTREEITEM oldParent = tvi.hParent;

		// Add sorting identifier
		s_SortInfo.emplace_back(true, skinFolder.name);
		tvi.item.lParam = (LPARAM)(size_t)(s_SortInfo.size() - 1);

		// Add folder
		tvi.item.iImage = tvi.item.iSelectedImage = 0;
		tvi.item.pszText = (WCHAR*)skinFolder.name.c_str();
		tvi.hParent = TreeView_InsertItem(tree, &tvi);

		// Add @Resources file if it exists
		if (skinFolder.level == 1)
		{
			std::wstring resources = GetRainmeter().GetSkinPath();
			resources += skinFolder.name;
			resources += L"\\@Resources";
			if (PathIsDirectory(resources.c_str()))
			{
				s_SortInfo.emplace_back(true, L"@Resources");
				tvi.item.lParam = (LPARAM)(size_t)(s_SortInfo.size() - 1);

				tvi.item.pszText = L"@Resources";
				TreeView_InsertItem(tree, &tvi);
			}
			else
			{
				c_Dialog->m_TabNew.m_CanAddResourcesFolder = true;
				Button_Enable(c_Dialog->m_TabNew.GetControl(Id_AddResourcesButton), TRUE);
			}
		}

		// Add subfolders
		if ((index + 1) < max &&
			GetRainmeter().m_SkinRegistry.GetFolder(index + 1).level == initialLevel + 1)
		{
			index = PopulateTree(tree, tvi, index + 1, false);
		}

		// Add files
		tvi.item.iImage = tvi.item.iSelectedImage = 1;
		for (int i = 0, isize = (int)skinFolder.files.size(); i < isize; ++i)
		{
			s_SortInfo.emplace_back(false, skinFolder.files[i].filename);
			tvi.item.lParam = (LPARAM)(size_t)(s_SortInfo.size() - 1);

			tvi.item.pszText = (WCHAR*)skinFolder.files[i].filename.c_str();
			TreeView_InsertItem(tree, &tvi);
		}

		tvi.hParent = oldParent;

		++index;
	}
	while (!isParentFolder);

	return index;
}

LRESULT DialogNewSkin::TabNew::TreeEditSubclass(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uId, DWORD_PTR data)
{
	if (!lParam) return DefSubclassProc(hwnd, msg, wParam, lParam);

	switch (msg)
	{
	case WM_GETDLGCODE:
	{
		LPMSG lpMsg = (LPMSG)lParam;
		switch (lpMsg->message)
		{
		case WM_CHAR:
		case WM_KEYDOWN:
			switch (wParam)
			{
			case VK_ESCAPE:
				TreeView_EndEditLabelNow(hwnd, FALSE);
				break;

			case VK_RETURN:
				TreeView_EndEditLabelNow(hwnd, TRUE);
				break;
			}
		}
		return (DLGC_WANTMESSAGE | DefSubclassProc(hwnd, msg, wParam, lParam));
	}
	break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

// -----------------------------------------------------------------------------------------------
//
//                                Template tab
//
// -----------------------------------------------------------------------------------------------

DialogNewSkin::TabTemplate::TabTemplate() : Tab()
{
}

void DialogNewSkin::TabTemplate::Create(HWND owner)
{
	Tab::CreateTabWindow(15, 30, 270, 188, owner);

	short buttonWidth = (short)_wtoi(GetString(ID_STR_NUM_BUTTONWIDTH));
	short column1 = (268 - buttonWidth - 6);

	static const ControlTemplate::Control s_Controls[] =
	{
		CT_GROUPBOX(-1, ID_STR_SAVENEWTEMPLATE,
			0, 0, 268, 36,
			WS_VISIBLE, 0),
		CT_LABEL(-1, ID_STR_NAMESC,
			6, 16, 55, 9,
			WS_VISIBLE, 0),
		CT_EDIT(Id_NewEdit, 0,
			66, 14, column1 - 66 - 10, 14,
			WS_VISIBLE | WS_TABSTOP, WS_EX_CLIENTEDGE),
		CT_BUTTON(Id_SaveButton, ID_STR_SAVE,
			column1, 14, buttonWidth, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),

		CT_GROUPBOX(-1, ID_STR_SAVEDTEMPLATES,
			0, 43, 268, 143,
			WS_VISIBLE , 0),
		CT_LISTBOX(Id_TemplateListBox, 0,
			6, 59, column1 - 16, 119,
			WS_VISIBLE | WS_TABSTOP | LBS_NOTIFY | LBS_SORT | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_HSCROLL, WS_EX_CLIENTEDGE),
		CT_BUTTON(Id_EditButton, ID_STR_EDIT,
			column1, 59, buttonWidth, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		CT_BUTTON(Id_DeleteButton, ID_STR_DELETE,
			column1, 78, buttonWidth, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0)
	};

	CreateControls(s_Controls, _countof(s_Controls), c_Dialog->m_Font, GetString);

	// Add templates to listbox
	PopulateTemplates();
}

void DialogNewSkin::TabTemplate::Initialize()
{
	m_Initialized = true;
}

INT_PTR DialogNewSkin::TabTemplate::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return OnCommand(wParam, lParam);
	}

	return FALSE;
}

INT_PTR DialogNewSkin::TabTemplate::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case Id_NewEdit:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			WCHAR buffer[32];
			const int len = Edit_GetText((HWND)lParam, buffer, 32);
			EnableWindow(GetControl(Id_SaveButton), len > 0);
		}
		break;

	case Id_SaveButton:
		{
			WCHAR buffer[MAX_PATH];
			HWND item = GetControl(Id_NewEdit);
			Edit_GetText(item, buffer, MAX_PATH);

			const auto& templateFolder = GetTemplateFolder();
			std::wstring templateFile = templateFolder + buffer;
			templateFile += L".template";

			// Check if template already exists
			bool alreadyExists = (_waccess(templateFile.c_str(), 0) != -1);
			if (alreadyExists)
			{
				const std::wstring text = GetFormattedString(ID_STR_TEMPLATEEXISTS, buffer);
				GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONWARNING | MB_OK);
				break;
			}

			// Create template folder if it doesnt exist
			BOOL exists = CreateDirectory(templateFolder.c_str(), NULL);
			if (!exists && GetLastError() == ERROR_PATH_NOT_FOUND)
			{
				const std::wstring text = GetString(ID_STR_TEMPLATEFOLDERFAIL);
				GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONERROR | MB_OK);
				break;
			}

			CreateTemplate(templateFile);

			if (templateFile.empty())  // Could not create template file
			{
				const std::wstring text = GetFormattedString(ID_STR_TEMPLATEFILEFAIL, buffer);
				GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONERROR | MB_OK);
				break;
			}

			// Clear editbox
			Edit_SetText(item, L"");

			// Add string to listbox and select it
			item = GetControl(Id_TemplateListBox);
			const int selected = ListBox_AddString(item, buffer);
			ListBox_SetCurSel(item, selected);

			c_Templates.insert(c_Templates.begin() + selected, buffer);

			// Reset the selected template on the other tab
			ValidateSelectedTemplate();

			// |LBN_SELCHANGE| doesn't get sent from ListBox_SetCurSel, so enable buttons
			item = GetControl(Id_EditButton);
			EnableWindow(item, TRUE);
			item = GetControl(Id_DeleteButton);
			EnableWindow(item, TRUE);
		}
		break;

	case Id_TemplateListBox:
		if (HIWORD(wParam) == LBN_SELCHANGE)
		{
			// Ignore clicks that don't hit items
			if (ListBox_GetCurSel((HWND)lParam) != LB_ERR)
			{
				HWND item = GetControl(Id_EditButton);
				EnableWindow(item, TRUE);
				item = GetControl(Id_DeleteButton);
				EnableWindow(item, TRUE);
			}
		}
		break;

	case Id_EditButton:
		{
			HWND item = GetControl(Id_TemplateListBox);
			WCHAR buffer[MAX_PATH];
			if (ListBox_GetText(item, ListBox_GetCurSel(item), buffer) > 0)
			{
				std::wstring args = L"\"" + GetTemplateFolder();
				args += buffer;
				args += L".template\"";
				CommandHandler::RunFile(GetRainmeter().GetSkinEditor().c_str(), args.c_str());
			}
		}
		break;

	case Id_DeleteButton:
		{
			HWND item = GetControl(Id_TemplateListBox);
			WCHAR buffer[MAX_PATH];
			const int selected = ListBox_GetCurSel(item);
			if (ListBox_GetText(item, selected, buffer) > 0)
			{
				const std::wstring text = GetFormattedString(ID_STR_TEMPLATEDELETE, buffer);
				if (GetRainmeter().ShowMessage(m_Window, text.c_str(), MB_ICONQUESTION | MB_YESNO) != IDYES)
				{
					// Cancel
					break;
				}

				// Remove file, delete from vector, and remove from listbox
				std::wstring file = GetTemplateFolder();
				file += buffer;
				file += L".template";
				System::RemoveFile(file);
				c_Templates.erase(c_Templates.begin() + selected);
				ListBox_DeleteString(item, selected);
				EnableWindow(GetControl(Id_EditButton), FALSE);
				EnableWindow(GetControl(Id_DeleteButton), FALSE);

				ValidateSelectedTemplate();
			}
		}
		break;

	default:
		return 1;
	}

	return 0;
}

void DialogNewSkin::TabTemplate::CreateTemplateMenu(HMENU menu, std::wstring selected)
{
	HWND listbox = c_Dialog->m_TabTemplate.GetControl(Id_TemplateListBox);
	const int count = ListBox_GetCount(listbox);

	int sel = 0;
	for (int i = 0; i < count; ++i)
	{
		WCHAR buffer[MAX_PATH];
		const int len = ListBox_GetText(listbox, i, buffer);
		if (len > 0)
		{
			// Insert a separator
			if (i == 0)
			{
				AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
			}
			
			if (_wcsicmp(selected.c_str(), buffer) == 0)
			{
				sel = i + 2;
			}

			const UINT flags = MF_BYPOSITION | MF_UNCHECKED;
			AppendMenu(menu, flags, ID_TEMPLATE_FIRST + i, buffer);
		}
	}

	CheckMenuItem(menu, sel, MF_BYPOSITION | MF_CHECKED);
}

void DialogNewSkin::TabTemplate::PopulateTemplates()
{
	HWND item = c_Dialog->m_TabTemplate.GetControl(Id_TemplateListBox);
	ListBox_ResetContent(item);

	for (const auto& iter : c_Templates)
	{
		ListBox_AddString(item, iter.c_str());
	}
}

void DialogNewSkin::TabTemplate::CreateTemplate(std::wstring& file)
{
	// Create default template file
	BOOL exists = PathFileExists(file.c_str());
	if (!exists)
	{
		FILE* fp;
		if (_wfopen_s(&fp, file.c_str(), L"w, ccs=UTF-16LE") == 0)
		{
			const WCHAR* str = L"[Rainmeter]\n"
				L"Update=1000\n"
				L"AccurateText=1\n\n"
				L"[Metadata]\n"
				L"Name=\n"
				L"Author=\n"
				L"Information=\n"
				L"Version=\n"
				L"License=Creative Commons Attribution - Non - Commercial - Share Alike 3.0\n\n"
				L"[Variables]\n\n"
				L"[MeterString]\n"
				L"Meter=String\n";

			if (fputws(str, fp) != 0 || fclose(fp) != 0)
			{
				file.clear();  // Error writing/closing file
			}
		}
		else  // Could not create file
		{
			file.clear();
		}
	}
}
