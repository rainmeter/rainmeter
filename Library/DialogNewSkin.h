/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __DIALOGNEWSKIN_H__
#define __DIALOGNEWSKIN_H__

#include "../Common/Dialog.h"
#include "Logger.h"
#include "Skin.h"

class DialogNewSkin : public Dialog
{
public:
	DialogNewSkin();
	virtual ~DialogNewSkin();

	DialogNewSkin(const DialogNewSkin& other) = delete;
	DialogNewSkin& operator=(DialogNewSkin other) = delete;

	static Dialog* GetDialog() { return c_Dialog; }

	static void Open(const WCHAR* tabName, const WCHAR* parent);
	static void Open(const WCHAR* name);
	static void Open(int tab = 0);
	static void ShowNewSkinDialog();
	
	static void CloseDialog() { if (c_Dialog) c_Dialog->HandleMessage(WM_CLOSE, 0, 0); }

protected:
	virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog(WPARAM wParam, LPARAM lParam);
	INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);

private:
	// NewSkin tab
	class TabNew : public Tab
	{
	public:
		enum Id
		{
			Id_ParentPathLabel = 100,
			Id_ItemsTreeView,
			Id_AddFolderButton,
			Id_AddResourcesButton,
			Id_AddSkinButton,
			Id_TemplateDropDownList
		};

		TabNew();

		void Create(HWND owner);
		virtual void Initialize();

		std::wstring& GetParentFolder() { return m_ParentFolder; }
		void SetParentFolder(const WCHAR* folder);

		std::wstring& GetSelectedTemplate() { return m_SelectedTemplate; }

		static void SelectTreeItem(HWND tree, HTREEITEM item, LPCWSTR name);

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);

	private:
		// Helper struct for sorting treeview items
		struct SortInfo
		{
			bool type;
			std::wstring name;
			SortInfo(bool t, std::wstring n) : type(t), name(n) {}
		};

		void UpdateParentPathLabel();
		void UpdateParentPathTT(bool update);
		void AddTreeItem(bool isFolder);
		bool DoesNodeExist(HTREEITEM item, LPCWSTR text, bool isFolder);

		static UINT GetChildSkinCount(HWND tree, HTREEITEM item);
		static int CALLBACK TreeViewSortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
		static std::wstring GetTreeSelectionPath(HWND tree, bool allItems);
		static int PopulateTree(HWND tree, TVINSERTSTRUCT& tvi, int index, bool isParentFolder);
		static LRESULT CALLBACK TreeEditSubclass(HWND hwnd, UINT msg, WPARAM wParam,
			LPARAM lParam, UINT_PTR uId, DWORD_PTR data);

		std::wstring m_ParentFolder;
		std::wstring m_SelectedTemplate;
		bool m_IsRoot;
		bool m_CanAddResourcesFolder;
		bool m_InRenameMode;
		HWND m_TreeEdit;
		HWND m_ParentPathTT;

		static std::vector<SortInfo> s_SortInfo;
	};

	// Template tab
	class TabTemplate : public Tab
	{
	public:
		enum Id
		{
			Id_NewEdit = 100,
			Id_SaveButton,
			Id_TemplateListBox,
			Id_EditButton,
			Id_DeleteButton
		};

		TabTemplate();

		void Create(HWND owner);
		virtual void Initialize();

		static void CreateTemplateMenu(HMENU menu, std::wstring selected);
		static void CreateTemplate(std::wstring& file);

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);

	private:
		static void PopulateTemplates();
	};

	enum Id
	{
		Id_CloseButton = IDCLOSE,
		Id_Tab = 100
	};

	struct CloseType
	{
		bool rootFolder;
		UINT skins;  // Not inc files
	};

	Tab& GetActiveTab();

	static const std::wstring& GetTemplateFolder();
	static void LoadTemplates();
	static void ValidateSelectedTemplate();

	TabNew m_TabNew;
	TabTemplate m_TabTemplate;

	static CloseType c_CloseAction;
	static std::vector<std::wstring> c_Templates;
	static WINDOWPLACEMENT c_WindowPlacement;
	static DialogNewSkin* c_Dialog;
};

#endif
