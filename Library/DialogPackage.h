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
#include "Dialog.h"

class DialogPackage : public OldDialog
{
public:
	static void Create(HINSTANCE hInstance, LPWSTR lpCmdLine);

	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog(WPARAM wParam, LPARAM lParam);
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);

	static DialogPackage* c_Dialog;

protected:
	virtual Tab& GetActiveTab();

private:
	class TabInfo : public Tab
	{
	public:
		TabInfo(HWND window);

		virtual void Initialize();

		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
	};

	class TabOptions : public Tab
	{
	public:
		TabOptions(HWND window);

		virtual void Initialize();

		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	};

	class TabAdvanced : public Tab
	{
	public:
		TabAdvanced(HWND window);

		virtual void Initialize();

		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
	};

	DialogPackage(HWND wnd);
	virtual ~DialogPackage();

	void SetNextButtonState();
	
	bool CreatePackage();
	static unsigned __stdcall PackagerThreadProc(void* pParam);

	bool AddFileToPackage(const WCHAR* realPath, const WCHAR* zipPath);
	bool AddFolderToPackage(const std::wstring& path, std::wstring base, const WCHAR* zipPrefix);

	void ShowHelp();

	static std::wstring SelectFolder(HWND parent, const std::wstring& existingPath);
	static INT_PTR CALLBACK SelectFolderDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static std::pair<std::wstring, std::wstring> SelectPlugin(HWND parent);
	static INT_PTR CALLBACK SelectPluginDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
