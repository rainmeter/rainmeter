/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef SKININSTALLER_DIALOGPACKAGE_H_
#define SKININSTALLER_DIALOGPACKAGE_H_

#include <string>
#include "zip.h"
#include "../Common/Dialog.h"

class DialogPackage : public Dialog
{
public:
	static void Create(HINSTANCE hInstance, LPWSTR lpCmdLine);

	virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnInitDialog(WPARAM wParam, LPARAM lParam);
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);

	static DialogPackage* c_Dialog;

private:
	enum Id
	{
		Id_Tab = 1000,
		Id_NextButton,
		Id_CreatePackageButton
	};

	class TabInfo : public Tab
	{
	public:
		enum Id
		{
			Id_NameEdit = 1000,
			Id_AuthorEdit,
			Id_VersionEdit,
			Id_ComponentsList,
			Id_AddSkinButton,
			Id_AddLayoutButton,
			Id_AddPluginButton,
			Id_RemoveButton,
			Id_WhatIsLink,
			Id_DescriptionLabel = 1100,
			Id_InformationGroup,
			Id_NameLabel,
			Id_AuthorLabel,
			Id_VersionLabel,
			Id_ComponentsGroup
		};

		void Create(HWND owner) override;

		virtual void Initialize();

		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
	};

	class TabOptions : public Tab
	{
	public:
		enum Id
		{
			Id_FileEdit = 1000,
			Id_FileBrowseButton,
			Id_DoNothingRadio,
			Id_LoadSkinRadio,
			Id_LoadSkinEdit,
			Id_LoadSkinBrowseButton,
			Id_LoadLayoutRadio,
			Id_LoadLayoutCombo,
			Id_RainmeterVersionEdit,
			Id_WindowsVersionCombo,
			Id_CreatingText,
			Id_CreatingBar,
			Id_SaveLabel = 1100,
			Id_AfterInstallGroup,
			Id_RequirementsGroup,
			Id_RainmeterVersionLabel,
			Id_WindowsVersionLabel
		};

		void Create(HWND owner) override;

		virtual void Initialize();

		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	};

	class TabAdvanced : public Tab
	{
	public:
		enum Id
		{
			Id_HeaderEdit = 1000,
			Id_HeaderBrowseButton,
			Id_VariableFilesEdit,
			Id_MergeSkinsCheck,
			Id_HelpLink,
			Id_HeaderLabel = 1100,
			Id_VariablesLabel
		};

		void Create(HWND owner) override;

		virtual void Initialize();

		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
	};

	DialogPackage();
	virtual ~DialogPackage();

	void SetNextButtonState();

	bool CreatePackage();
	static unsigned __stdcall PackagerThreadProc(void* pParam);

	bool AddFileToPackage(const WCHAR* realPath, const WCHAR* zipPath);
	bool AddFolderToPackage(const std::wstring& path, std::wstring base, const WCHAR* zipPrefix);

	void ShowHelp();

	class SelectFolderDialog;
	class SelectPluginDialog;

	static std::wstring SelectFolder(HWND parent, const std::wstring& existingPath);

	static std::pair<std::wstring, std::wstring> SelectPlugin(HWND parent);

	TabInfo m_TabInfo;
	TabOptions m_TabOptions;
	TabAdvanced m_TabAdvanced;

	std::wstring m_BackupTime;

	// Info tab
	std::wstring m_Name;
	std::wstring m_Author;
	std::wstring m_Version;
	std::pair<std::wstring, std::wstring> m_SkinFolder;
	std::map<std::wstring, std::wstring> m_LayoutFolders;
	std::map<std::wstring, std::pair<std::wstring, std::wstring>> m_PluginFolders;

	// Options tab
	std::wstring m_TargetFile;
	std::wstring m_MinimumRainmeter;
	std::wstring m_MinimumWindows;
	bool m_LoadLayout;
	std::wstring m_Load;

	// Advanced tab
	std::wstring m_HeaderFile;
	std::wstring m_VariableFiles;
	bool m_MergeSkins;

	HANDLE m_PackagerThread;
	zipFile m_ZipFile;
	bool m_AllowNonAsciiFilenames;
};

#endif
