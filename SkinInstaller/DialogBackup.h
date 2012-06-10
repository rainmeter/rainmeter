/*
  Copyright (C) 2011 Birunthan Mohanathas

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

#ifndef SKININSTALLER_DIALOGBACKUP_H_
#define SKININSTALLER_DIALOGBACKUP_H_

#include <string>
#include "zip.h"
#include "../Library/Dialog.h"

class CDialogBackup : public CDialog
{
public:
	static void Create(HINSTANCE hInstance, LPWSTR lpCmdLine);

	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR OnInitDialog(WPARAM wParam, LPARAM lParam);
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);

	static CDialogBackup* c_Dialog;

protected:
	virtual CTab& GetActiveTab();

private:
	class CTabBackup : public CTab
	{
	public:
		CTabBackup(HWND window);

		virtual void Initialize();

		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnCommand(WPARAM wParam, LPARAM lParam);
	};

	CDialogBackup(HWND wnd);
	virtual ~CDialogBackup();
	
	void StartBackup();
	bool CreateBackup();
	static unsigned __stdcall BackupThreadProc(void* pParam);

	bool AddFileToBackup(const char* realPath, const char* zipPath);
	bool AddFolderToBackup(const std::wstring& path, std::wstring base, char* zipPrefix, bool recursive);

	CTabBackup m_TabBackup;

	std::wstring m_TargetFile;
	std::wstring m_BackupTime;

	HANDLE m_ThreadHandle;

	zipFile m_ZipFile;
	void* m_WriteBuffer;
	static const int c_WriteBufferSize = 16384;
};

#endif
