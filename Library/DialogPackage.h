// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <string>
#include <vector>
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
		Id_BackButton,
		Id_CreatePackageButton,
		Id_CancelButton,
		Id_LoadPreviousButton
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

		void Update();
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
			Id_CreatingText,
			Id_CreatingBar,
			Id_SaveLabel = 1100,
			Id_AfterInstallGroup,
			Id_RequirementsGroup,
			Id_RainmeterVersionLabel
		};

		void Create(HWND owner) override;

		virtual void Initialize();

		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);

		void Update();
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

		void Update();
	};

	DialogPackage();
	virtual ~DialogPackage();

	struct Profile
	{
		std::wstring name;
		std::wstring skinFolder;
		std::wstring timestamp;
	};

	void SetNextButtonState();
	void SetLoadPreviousButtonState();
	void UpdateTabs();

	static std::vector<Profile> GetProfiles();

	void ShowLoadPreviousMenu(HWND button);
	void SetSkinFolder(const std::wstring& skinFolder);
	void LoadProfile(const std::wstring& skinFolder);
	void SaveProfile();
	void DeleteProfile(const std::wstring& section);

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
	std::wstring m_OutputFolder;
	std::wstring m_MinimumRainmeter;
	bool m_LoadLayout;
	std::wstring m_Load;

	// Advanced tab
	std::wstring m_HeaderFile;
	std::wstring m_VariableFiles;
	bool m_MergeSkins;

	bool m_ProfileLoaded;
	bool m_OptionsCreated;
	DWORD m_TabItemSize;
	HANDLE m_PackagerThread;
	zipFile m_ZipFile;
	bool m_AllowNonAsciiFilenames;
};
