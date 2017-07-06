/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __DIALOGMANAGE_H__
#define __DIALOGMANAGE_H__

#include "../Common/Dialog.h"
#include "resource.h"

class DialogManage : public Dialog
{
public:
	DialogManage();
	virtual ~DialogManage();

	DialogManage(const DialogManage& other) = delete;
	DialogManage& operator=(DialogManage other) = delete;

	static Dialog* GetDialog() { return c_Dialog; }

	static void Open(const WCHAR* tabName, const WCHAR* param1, const WCHAR* param2);
	static void Open(const WCHAR* name);
	static void Open(int tab = 0);
	static void OpenSkin(Skin* skin);

	static void UpdateSelectedSkinOptions(Skin* skin);

	static void UpdateSkins(Skin* skin, bool deleted = false);
	static void UpdateLayouts();

	static void CloseDialog() { if (c_Dialog) c_Dialog->HandleMessage(WM_CLOSE, 0, 0); }

protected:
	virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog(WPARAM wParam, LPARAM lParam);
	INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);

private:
	// Skins tab
	class TabSkins : public Tab
	{
	public:
		enum Id
		{
			Id_ActiveSkinsButton = 100,
			Id_NewSkinButton,
			Id_SkinsTreeView,
			Id_CreateSkinPackageButton,
			Id_FileLabel,
			Id_ConfigLabel,
			Id_AuthorLabel,
			Id_VersionLabel,
			Id_LicenseLabel,
			Id_DescriptionLabel,
			Id_AddMetadataLink,
			Id_XPositionEdit,
			Id_YPositionEdit,
			Id_ZPositionDropDownList,
			Id_LoadOrderEdit,
			Id_OnHoverDropDownList,
			Id_TransparencyDropDownList,
			Id_DisplayMonitorButton,
			Id_DraggableCheckBox,
			Id_ClickThroughCheckBox,
			Id_KeepOnScreenCheckBox,
			Id_SavePositionCheckBox,
			Id_SnapToEdgesCheckBox,
			Id_FavoriteCheckBox,

			Id_LoadButton    = IDM_MANAGESKINSMENU_LOAD,
			Id_RefreshButton = IDM_MANAGESKINSMENU_REFRESH,
			Id_EditButton    = IDM_MANAGESKINSMENU_EDIT
		};

		TabSkins();
		~TabSkins();

		void Create(HWND owner);
		virtual void Initialize();

		void UpdateSelected(Skin* skin);
		void Update(Skin* skin, bool deleted);

		static void SelectTreeItem(HWND tree, HTREEITEM item, LPCWSTR name);

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);

	private:
		void SetControls();
		void DisableControls(bool clear = false);
		void ReadSkin();

		static LRESULT CALLBACK NewSkinButtonSubclass(HWND hwnd, UINT msg, WPARAM wParam,
			LPARAM lParam, UINT_PTR uId, DWORD_PTR data);

		static std::wstring GetTreeSelectionPath(HWND tree);
		static int PopulateTree(HWND tree, TVINSERTSTRUCT& tvi, int index = 0);

		std::wstring m_SkinFileName;
		std::wstring m_SkinFolderPath;
		Skin* m_SkinWindow;
		bool m_HandleCommands;
		bool m_IgnoreUpdate;

		static HBRUSH s_NewSkinBkBrush;
		static COLORREF s_NewSkinBkColor;
	};

	// Layouts tab
	class TabLayouts : public Tab
	{
	public:
		enum Id
		{
			Id_List = 100,
			Id_LoadButton,
			Id_DeleteButton,
			Id_EditButton,
			Id_SaveButton,
			Id_SaveEmptyThemeCheckBox,
			Id_ExcludeUnusedSkinsCheckBox,
			Id_IncludeWallpaperCheckBox,
			Id_NameLabel
		};

		TabLayouts();

		void Create(HWND owner);
		virtual void Initialize();

		void Update();

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	};
	
	// Settings tab
	class TabSettings : public Tab
	{
	public:
		enum Id
		{
			Id_CheckForUpdatesCheckBox = 100,
			Id_LockSkinsCheckBox,
			Id_ResetStatisticsButton,
			Id_LogToFileCheckBox,
			Id_VerboseLoggingCheckbox,
			Id_ShowLogFileButton,
			Id_DeleteLogFileButton,
			Id_LanguageDropDownList,
			Id_EditorEdit,
			Id_EditorBrowseButton,
			Id_ShowTrayIconCheckBox
		};

		TabSettings();

		void Create(HWND owner);
		virtual void Initialize();

	protected:
		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	};

	enum Id
	{
		Id_CloseButton = IDCLOSE,
		Id_Tab = 100,
		Id_RefreshAllButton,
		Id_EditSettingsButton,
		Id_OpenLogButton,
		Id_HelpButton
	};

	Tab& GetActiveTab();

	TabSkins m_TabSkins;
	TabLayouts m_TabLayouts;
	TabSettings m_TabSettings;

	static WINDOWPLACEMENT c_WindowPlacement;
	static DialogManage* c_Dialog;
};

#endif
