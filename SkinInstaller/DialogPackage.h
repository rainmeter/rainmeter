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

#ifndef SKININSTALLER_DIALOGPACKAGE_H_
#define SKININSTALLER_DIALOGPACKAGE_H_

#include <string>
#include "zip.h"
#include "../Library/Dialog.h"

class CDialogPackage : public CDialog
{
public:
	static void Create(HINSTANCE hInstance, LPWSTR lpCmdLine);

	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog(WPARAM wParam, LPARAM lParam);
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);

	static CDialogPackage* c_Dialog;

protected:
	virtual CTab& GetActiveTab();

private:
	class CTabInfo : public CTab
	{
	public:
		CTabInfo(HWND window);

		virtual void Initialize();

		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
	};

	class CTabOptions : public CTab
	{
	public:
		CTabOptions(HWND window);

		virtual void Initialize();

		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	};

	class CTabAdvanced : public CTab
	{
	public:
		CTabAdvanced(HWND window);

		virtual void Initialize();

		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
		INT_PTR OnNotify(WPARAM wParam, LPARAM lParam);
	};

	CDialogPackage(HWND wnd);
	virtual ~CDialogPackage();

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

	CTabInfo m_TabInfo;
	CTabOptions m_TabOptions;
	CTabAdvanced m_TabAdvanced;

	std::wstring m_BackupTime;

	// Info tab
	std::wstring m_Name;
	std::wstring m_Author;
	std::wstring m_Version;
	std::pair<std::wstring, std::wstring> m_SkinFolder;
	std::map<std::wstring, std::wstring> m_ThemeFolders;
	std::map<std::wstring, std::pair<std::wstring, std::wstring>> m_PluginFolders;

	// Options tab
	std::wstring m_TargetFile;
	std::wstring m_MinimumRainmeter;
	std::wstring m_MinimumWindows;
	bool m_LoadTheme;
	std::wstring m_Load;

	// Advanced tab
	std::wstring m_HeaderFile;
	std::wstring m_VariableFiles;
	bool m_MergeSkins;

	HANDLE m_PackagerThread;
	zipFile m_ZipFile;
};

#endif
