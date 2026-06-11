/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef SKININSTALLER_DIALOGINSTALL_H_
#define SKININSTALLER_DIALOGINSTALL_H_

#include <string>
#include "unzip.h"
#include "../Common/Dialog.h"

class DialogInstall : public Dialog
{
public:
	static void Create(HINSTANCE hInstance, LPWSTR lpCmdLine);

	virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnInitDialog(WPARAM wParam, LPARAM lParam);
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);

	static DialogInstall* c_Dialog;

private:
	friend class DialogPackage;

	enum Id
	{
		Id_Tab = 1000,
		Id_HeaderBitmap,
		Id_AdvancedButton,
		Id_InstallButton
	};

	class TabInstall : public Tab
	{
	public:
		enum Id
		{
			Id_NameText = 1000,
			Id_AuthorText,
			Id_VersionText,
			Id_ComponentsList,
			Id_ThemeCheckBox,
			Id_InProgressText,
			Id_Progress,
			Id_NameLabel = 1100,
			Id_AuthorLabel,
			Id_VersionLabel,
			Id_ComponentsLabel
		};

		void Create(HWND owner) override;

		virtual void Initialize();

		virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		static INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
	};

	enum Timer
	{
		Thread = 1
	};

	enum PackageFormat
	{
		None,
		New,
		Old
	};

	enum PackageFlag
	{
		Backup    = 0x0001
	};

	struct PackageFooter
	{
		__int64 size;
		BYTE flags;
		char key[7];
	};

	DialogInstall(const WCHAR* file);
	virtual ~DialogInstall();

	bool ReadPackage();
	bool ReadOptions(const WCHAR* file);
	bool InstallPackage();
	void BeginInstall();
	static UINT __stdcall InstallThread(void* pParam);
	bool ExtractCurrentFile(const std::wstring& fileName);
	int IsPluginNewer(const std::wstring& item, const std::wstring& itemPath);

	void LaunchRainmeter();
	void KeepVariables();
	void ArchivePlugin(const std::wstring& folder, const std::wstring& name);

	static void CleanLayoutFile(const WCHAR* file);

	static bool IsIgnoredSkin(const WCHAR* name);
	static bool IsIgnoredLayout(const WCHAR* name);
	static bool IsIgnoredAddon(const WCHAR* name);
	static bool IsIgnoredPlugin(const WCHAR* name);

	static int CompareVersions(const std::wstring& strA, const std::wstring& strB);
	static std::vector<std::wstring> Tokenize(const std::wstring& str, const std::wstring& delimiters);
	static bool CreateDirectoryRecursive(const std::wstring& path);
	static std::wstring GetFileVersionString(const WCHAR* fileName);
	static std::wstring GetWindowsVersionString();

	TabInstall m_TabInstall;

	HBITMAP m_HeaderBitmap;

	HANDLE m_InstallThread;

	std::wstring m_ErrorMessage;
	std::wstring m_Name;
	std::wstring m_Author;
	std::wstring m_Version;

	unzFile m_PackageUnzFile;
	std::wstring m_PackageFileName;
	std::wstring m_PackageRoot;
	PackageFormat m_PackageFormat;
	std::set<std::wstring> m_PackageSkins;
	std::set<std::wstring> m_PackageLayouts;
	std::set<std::wstring> m_PackageAddons;
	std::set<std::wstring> m_PackageFonts;
	std::set<std::wstring> m_PackagePlugins;

	bool m_BackupPackage;
	bool m_BackupSkins;
	bool m_MergeSkins;
	bool m_SystemFonts;
	bool m_ArchivePlugins;
	std::vector<std::wstring> m_VariablesFiles;
	std::vector<std::wstring> m_LoadSkins;
	std::wstring m_LoadLayout;
};

#endif
