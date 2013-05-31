/*
  Copyright (C) 2012 Birunthan Mohanathas

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef SKININSTALLER_DIALOGINSTALL_H_
#define SKININSTALLER_DIALOGINSTALL_H_

#include <string>
#include "unzip.h"
#include "../Library/Dialog.h"

class DialogInstall : public Dialog
{
public:
	static void Create(HINSTANCE hInstance, LPWSTR lpCmdLine);

	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog(WPARAM wParam, LPARAM lParam);
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);

	static DialogInstall* c_Dialog;

protected:
	virtual Tab& GetActiveTab();

private:
	friend class DialogPackage;

	class TabInstall : public Tab
	{
	public:
		TabInstall(HWND window);

		virtual void Initialize();

		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
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

	DialogInstall(HWND wnd, const WCHAR* file);
	virtual ~DialogInstall();

	bool ReadPackage();
	bool ReadOptions(const WCHAR* file);
	bool InstallPackage();
	void BeginInstall();
	static UINT __stdcall InstallThread(void* pParam);
	bool ExtractCurrentFile(const std::wstring& fileName);

	void LaunchRainmeter();
	void KeepVariables();

	static bool IsIgnoredSkin(const WCHAR* name);
	static bool IsIgnoredLayout(const WCHAR* name);
	static bool IsIgnoredAddon(const WCHAR* name);
	static bool IsIgnoredPlugin(const WCHAR* name);

	static int CompareVersions(const std::wstring& strA, const std::wstring& strB);
	static std::vector<std::wstring> Tokenize(const std::wstring& str, const std::wstring& delimiters);
	static bool CreateDirectoryRecursive(const std::wstring& path);
	static std::wstring GetFileVersionString(const WCHAR* fileName);
	static std::wstring GetDotNetVersionString();
	static std::wstring GetWindowsVersionString();

	TabInstall m_TabInstall;

	HBITMAP m_HeaderBitmap;

	HANDLE m_InstallThread;

	std::wstring m_ErrorMessage;

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
	std::vector<std::wstring> m_VariablesFiles;
	std::vector<std::wstring> m_LoadSkins;
	std::wstring m_LoadLayout;
};

#endif
