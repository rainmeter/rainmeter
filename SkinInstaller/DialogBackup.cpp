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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "StdAfx.h"
#include "Application.h"
#include "DialogBackup.h"
#include "resource.h"
#include "../Version.h"

#define WM_DELAYED_CLOSE WM_APP + 0

extern GLOBALDATA g_Data;

CDialogBackup* CDialogBackup::c_Dialog = NULL;

/*
** CDialogBackup
**
** Constructor.
**
*/
CDialogBackup::CDialogBackup(HWND wnd) : CDialog(wnd),
	m_TabBackup(wnd),
	m_ThreadHandle(),
	m_ZipFile(),
	m_WriteBuffer()
{
}

/*
** ~CDialogBackup
**
** Destructor.
**
*/
CDialogBackup::~CDialogBackup()
{
}

/*
** Create
**
** Creates the dialog.
**
*/
void CDialogBackup::Create(HINSTANCE hInstance, LPWSTR lpCmdLine)
{
	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_BACKUP_DIALOG), NULL, (DLGPROC)DlgProc, (LPARAM)lpCmdLine);
}

/*
** DlgProc
**
** Dialog procedure for the About dialog.
**
*/
INT_PTR CALLBACK CDialogBackup::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!c_Dialog)
	{
		if (uMsg == WM_INITDIALOG)
		{
			c_Dialog = new CDialogBackup(hWnd);
			return c_Dialog->OnInitDialog(wParam, lParam);
		}
	}
	else
	{
		switch (uMsg)
		{
		case WM_COMMAND:
			return c_Dialog->OnCommand(wParam, lParam);

		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return TRUE;

		case WM_DESTROY:
			delete c_Dialog;
			c_Dialog = NULL;
			return FALSE;
		}
	}

	return FALSE;
}

INT_PTR CDialogBackup::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	HICON hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_INSTALLER), IMAGE_ICON, 16, 16, LR_SHARED);
	SendMessage(m_Window, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	if (GetOSPlatform() >= OSPLATFORM_VISTA)
	{
		// F-Secure et al. detect SetDialogFont() as malware..
		//SetDialogFont();
	}

	HWND item = GetDlgItem(m_Window, IDC_BACKUP_TAB);
	TCITEM tci = {0};
	tci.mask = TCIF_TEXT;
	tci.pszText = L"Backup";
	TabCtrl_InsertItem(item, 0, &tci);

	m_TabBackup.Activate();

	return TRUE;
}

INT_PTR CDialogBackup::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_BACKUP_BACKUP_BUTTON:
		StartBackup();
		break;

	case IDCLOSE:
		if (m_ThreadHandle)
		{
			if (IDYES != MessageBox(NULL, L"The backup is still in progress. Are you sure you want to cancel?", NULL, MB_YESNO | MB_TOPMOST | MB_ICONHAND))
			{
				break;
			}
		}
		EndDialog(m_Window, 0);
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

void CDialogBackup::StartBackup()
{
	if (!CloseRainmeterIfActive())
	{
		MessageBox(m_Window, L"Unable to close Rainmeter.", L"Backup Rainmeter", MB_ERROR);
	}
	else
	{
		HWND item = GetDlgItem(m_Window, IDC_BACKUP_BACKUP_BUTTON);
		EnableWindow(item, FALSE);

		item = GetDlgItem(m_TabBackup.GetWindow(), IDC_BACKUP_FILE_TEXT);
		ShowWindow(item, SW_HIDE);

		item = GetDlgItem(m_TabBackup.GetWindow(), IDC_BACKUP_BROWSE_BUTTON);
		ShowWindow(item, SW_HIDE);

		item = GetDlgItem(m_TabBackup.GetWindow(), IDC_BACKUP_INPROGRESS_TEXT);
		ShowWindow(item, SW_SHOWNORMAL);

		item = GetDlgItem(m_TabBackup.GetWindow(), IDC_BACKUP_PROGRESS);
		ShowWindow(item, SW_SHOWNORMAL);
		SendMessage(item, PBM_SETMARQUEE, (WPARAM)TRUE, 0);

		m_ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, BackupThreadProc, this, 0, NULL);
		if (!m_ThreadHandle)
		{
			MessageBox(m_Window, L"Unable to start backup.", L"Backup Rainmeter", MB_ERROR);
			EndDialog(m_Window, 0);
		}
	}
}

bool CDialogBackup::CreateBackup()
{
	m_WriteBuffer = new char[c_WriteBufferSize];
	
	std::wstring addonsPath = g_Data.programPath + L"Addons";
	std::wstring fontsPath = g_Data.programPath + L"Fonts";
	std::wstring pluginsPath = g_Data.programPath + L"Plugins";
	std::wstring themesPath = g_Data.settingsPath + L"Themes";
	std::wstring skinsPath = g_Data.skinsPath;

	// Remove trailing slash if needed
	if (skinsPath.back() == L'\\') skinsPath.resize(skinsPath.length() - 1);

	m_ZipFile = zipOpen(ConvertToAscii(m_TargetFile.c_str()).c_str(), 0);
	if (!m_ZipFile)
	{
		MessageBox(NULL, L"Unable to access backup .rmskin file.", NULL, MB_OK | MB_TOPMOST);
		return false;
	}

	if ((_waccess(addonsPath.c_str(), 0) == 0 && !AddFolderToBackup(addonsPath, L"", "Addons", true)) ||
		(_waccess(fontsPath.c_str(), 0) == 0 && !AddFolderToBackup(themesPath, L"", "Fonts", true)) ||
		(_waccess(skinsPath.c_str(), 0) == 0 && !AddFolderToBackup(skinsPath, L"", "Skins", true)) ||
		(_waccess(themesPath.c_str(), 0) == 0 && !AddFolderToBackup(themesPath, L"", "Themes", true)) ||
#ifdef _WIN64
		(_waccess(pluginsPath.c_str(), 0) == 0 && !AddFolderToBackup(pluginsPath, L"", "Plugins\\64bit", false)))
#else
		(_waccess(pluginsPath.c_str(), 0) == 0 && !AddFolderToBackup(pluginsPath, L"", "Plugins\\32bit", false)))
#endif
	{
		// Error message already displayed in AddFolderToBackup()
		return false;
	}

	// Create and add the config file
	WCHAR tempFile[MAX_PATH];
	GetTempPath(MAX_PATH, tempFile);
	GetTempFileName(tempFile, L"ini", 0, tempFile);

	std::wstring tmpSz = m_TargetFile;
	std::wstring::size_type pos = m_TargetFile.find_last_of(L'\\');
	if (pos != std::wstring::npos)
	{
		tmpSz.erase(0, ++pos);
	}

	WritePrivateProfileString(L"Rainstaller", L"Name", tmpSz.c_str(), tempFile);
	WritePrivateProfileString(L"Rainstaller", L"RainmeterFonts", L"1", tempFile);
	WritePrivateProfileString(L"Rainstaller", L"MinRainmeterVer", L"2.2", tempFile);
	AddFileToBackup(ConvertToAscii(tempFile).c_str(), "Rainstaller.cfg");
	DeleteFile(tempFile);

	if (zipClose(m_ZipFile, NULL) != ZIP_OK)
	{
		MessageBox(NULL, L"Unable to access backup .rmskin file.", NULL, MB_OK | MB_TOPMOST);
		return false;
	}

	delete m_WriteBuffer;
	return true;
}

unsigned __stdcall CDialogBackup::BackupThreadProc(void* pParam)
{
	CDialogBackup* dialog = (CDialogBackup*)pParam;

	if (dialog->CreateBackup())
	{
		// Stop the progress bar
		HWND item = GetDlgItem(dialog->m_Window, IDC_BACKUP_PROGRESS);
		SendMessage(item, PBM_SETMARQUEE, (WPARAM)FALSE, 0);

		FlashWindow(dialog->m_Window, TRUE);
		MessageBox(dialog->m_Window, L"The backup is complete.", L"Backup Rainmeter", MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		DeleteFile(dialog->m_TargetFile.c_str());
	}

	EndDialog(dialog->m_Window, 0);

	return 0;
}

uLong filetime(const char *f,  tm_zip *tmzip, uLong *dt)
{
	int ret = 0;
	{
		FILETIME ftLocal;
		HANDLE hFind;
		WIN32_FIND_DATAA ff32;

		hFind = FindFirstFileA(f, &ff32);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			FileTimeToLocalFileTime(&(ff32.ftLastWriteTime),&ftLocal);
			FileTimeToDosDateTime(&ftLocal,((LPWORD)dt)+1,((LPWORD)dt)+0);
			FindClose(hFind);
			ret = 1;
		}
	}
	return ret;
}

bool CDialogBackup::AddFileToBackup(const char* realPath, const char* zipPath)
{
	zip_fileinfo zi = {0};
	filetime(realPath, &zi.tmz_date, &zi.dosDate);

	int err = zipOpenNewFileInZip(m_ZipFile, zipPath, &zi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
	if (err != ZIP_OK) return false;

	FILE* fin = fopen(realPath, "rb");
	if (fin)
	{
		size_t readSize;
		do
		{
			//err = ZIP_OK;
			readSize = fread(m_WriteBuffer, 1, c_WriteBufferSize, fin);
			if (readSize < c_WriteBufferSize && feof(fin) == 0)
			{
				err = ZIP_ERRNO;
			}
			else if (readSize > 0)
			{
				err = zipWriteInFileInZip(m_ZipFile, m_WriteBuffer, (UINT)readSize);
				if (err < 0)
				{
					err = ZIP_ERRNO;
				}
			}
		}
		while ((err == ZIP_OK) && (readSize > 0));

		fclose(fin);
	}
	else
	{
		err = ZIP_ERRNO;
	}

	if (zipCloseFileInZip(m_ZipFile) != ZIP_OK) return false;

	return err == ZIP_OK;
}

bool CDialogBackup::AddFolderToBackup(const std::wstring& path, std::wstring base, char* zipPrefix, bool recursive)
{
	std::wstring filter = path + base;
	filter += L"\\";
	std::string asciiBase = ConvertToAscii(filter.c_str());
	filter += L"*";

	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFileEx(
		filter.c_str(),
		FindExInfoStandard,
		&fd,
		FindExSearchNameMatch,
		NULL,
		0);

	if (hFind == INVALID_HANDLE_VALUE) return false;

	bool ret = true;

	std::list<std::wstring> folders;
	do
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (wcscmp(L".", fd.cFileName) != 0 &&
				wcscmp(L"..", fd.cFileName) != 0 &&
				wcscmp(L"Backup", fd.cFileName) != 0)
			{
				folders.push_back(fd.cFileName);
			}
		}
		else
		{
			std::string asciiFile = asciiBase + ConvertToAscii(fd.cFileName);
			std::string zipFile = zipPrefix;
			zipFile += &asciiFile[path.length()];

			ret = AddFileToBackup(asciiFile.c_str(), zipFile.c_str());
			if (!ret)
			{
				std::wstring error = L"Error including the file:\n\n";
				error += path;
				error += base;
				error += L"\\";
				error += fd.cFileName;
				MessageBox(m_Window, error.c_str(), L"Backup Rainmeter", MB_OK | MB_TOPMOST);
				break;
			}
		}
	}
	while (FindNextFile(hFind, &fd));
	FindClose(hFind);

	if (recursive && ret)
	{
		base += L"\\";
		std::list<std::wstring>::const_iterator iter = folders.begin();
		for ( ; iter != folders.end(); ++iter)
		{
			ret = AddFolderToBackup(path, base + (*iter), zipPrefix, recursive);
			if (!ret) break;
		}
	}

	return ret;
}


// -----------------------------------------------------------------------------------------------
//
//                                Backup tab
//
// -----------------------------------------------------------------------------------------------

/*
** CTabBackup
**
** Constructor.
**
*/
CDialogBackup::CTabBackup::CTabBackup(HWND wnd) : CTab(GetModuleHandle(NULL), wnd, IDD_BACKUP_TABDIALOG, DlgProc)
{
}

/*
** Initialize
**
** Called when tab is displayed.
**
*/
void CDialogBackup::CTabBackup::Initialize()
{
	m_Initialized = true;

	WCHAR buffer[MAX_PATH];
	HRESULT hr = SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, buffer);
	if (SUCCEEDED(hr))
	{
		c_Dialog->m_TargetFile = buffer;
		SYSTEMTIME lt;
		GetLocalTime(&lt);
		_snwprintf_s(buffer, _TRUNCATE, L"\\Backup-%02d.%02d.%02d-%02d.%02d.rmskin", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute);
		c_Dialog->m_TargetFile += buffer;

		HWND item = GetDlgItem(m_Window, IDC_BACKUP_FILE_TEXT);
		SetWindowText(item, c_Dialog->m_TargetFile.c_str());
	}
}

/*
** DlgProc
**
** Dialog procedure for the Version tab.
**
*/
INT_PTR CALLBACK CDialogBackup::CTabBackup::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return c_Dialog->m_TabBackup.OnCommand(wParam, lParam);
	}

	return FALSE;
}

INT_PTR CDialogBackup::CTabBackup::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_BACKUP_BROWSE_BUTTON:
		{
			WCHAR buffer[MAX_PATH];
			BROWSEINFO bi = {0};
			bi.hwndOwner = c_Dialog->GetWindow();
			bi.ulFlags = BIF_USENEWUI;

			PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&bi);
			if (pidl && SHGetPathFromIDList(pidl, buffer))
			{
				c_Dialog->m_TargetFile = buffer;
				SYSTEMTIME lt;
				GetLocalTime(&lt);
				_snwprintf_s(buffer, _TRUNCATE, L"\\Backup-%02d.%02d.%02d-%02d.%02d.rmskin", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute);
				c_Dialog->m_TargetFile += buffer;

				HWND item = GetDlgItem(m_Window, IDC_BACKUP_FILE_TEXT);
				SetWindowText(item, c_Dialog->m_TargetFile.c_str());
			}
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}
