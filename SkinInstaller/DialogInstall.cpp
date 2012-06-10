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

#include "StdAfx.h"
#include "Application.h"
#include "DialogInstall.h"
#include "../Library/pcre-8.10/config.h"
#include "../Library/pcre-8.10/pcre.h"
#include "resource.h"
#include "../Version.h"

#define WM_DELAYED_CLOSE WM_APP + 0

extern GlobalData g_Data;

CDialogInstall* CDialogInstall::c_Dialog = NULL;

inline bool IsWin32Build()
{
#ifdef _WIN32
	return true;
#else
	return false;
#endif
}

/*
** Constructor.
**
*/
CDialogInstall::CDialogInstall(HWND wnd, const WCHAR* file) : CDialog(wnd),
	m_TabInstall(wnd),
	m_InstallThread(),
	m_PackageUnzFile(),
	m_PackageFileName(file),
	m_PackageFormat(PackageFormat::Old),
	m_BackupSkins(true),
	m_MergeSkins(false),
	m_SystemFonts(false)
{
}

/*
** Destructor.
**
*/
CDialogInstall::~CDialogInstall()
{
	if (m_PackageUnzFile)
	{
		unzClose(m_PackageUnzFile);
	}
}

/*
** Creates the dialog.
**
*/
void CDialogInstall::Create(HINSTANCE hInstance, LPWSTR lpCmdLine)
{
	// Prompt to select .rmskin file if needed
	WCHAR buffer[MAX_PATH];
	if (!*lpCmdLine)
	{
		buffer[0] = L'\0';

		OPENFILENAME ofn = {0};
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.lpstrFilter = L"Rainmeter skin file (.rmskin)\0*.rmskin;*.zip";
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = buffer;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrTitle = L"Select Rainmeter skin file";
		ofn.lpstrDefExt = L"rmskin";
		ofn.Flags = OFN_FILEMUSTEXIST;

		if (!GetOpenFileName(&ofn))
		{
			return;
		}

		lpCmdLine = buffer;
	}

	HANDLE hMutex;
	if (IsRunning(L"RainmeterSkinInstaller", &hMutex))
	{
		HWND hwnd = FindWindow(L"#32770", L"Rainmeter Skin Installer");
		SetForegroundWindow(hwnd);
	}
	else
	{
		DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_INSTALL_DIALOG), NULL, (DLGPROC)DlgProc, (LPARAM)lpCmdLine);
		ReleaseMutex(hMutex);
	}
}

CDialog::CTab& CDialogInstall::GetActiveTab()
{
	return m_TabInstall;
}

INT_PTR CALLBACK CDialogInstall::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!c_Dialog)
	{
		if (uMsg == WM_INITDIALOG)
		{
			c_Dialog = new CDialogInstall(hWnd, (const WCHAR*)lParam);
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

INT_PTR CDialogInstall::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	HICON hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SKININSTALLER), IMAGE_ICON, 16, 16, LR_SHARED);
	SendMessage(m_Window, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	if (GetOSPlatform() >= OSPLATFORM_VISTA)
	{
		SetDialogFont();

		BUTTON_SPLITINFO bsi;
		bsi.mask = BCSIF_SIZE;
		bsi.size.cx = 20;
		bsi.size.cy = 14;

		HWND item = GetDlgItem(m_Window, IDC_INSTALL_ADVANCED_BUTTON);
		Button_SetStyle(item, BS_SPLITBUTTON, TRUE);
		Button_SetSplitInfo(item, &bsi);
	}

	if (ReadPackage())
	{
		m_TabInstall.Activate();
	}
	else
	{
		if (m_ErrorMessage.empty())
		{
			m_ErrorMessage = L"Invalid package:\n";
			m_ErrorMessage += m_PackageFileName;
		}

		MessageBox(NULL, m_ErrorMessage.c_str(), L"Rainmeter Skin Installer", MB_ERROR);
		EndDialog(m_Window, 0);
	}


	return TRUE;
}

INT_PTR CDialogInstall::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_INSTALL_ADVANCED_BUTTON:
		{
			RECT r;
			GetWindowRect((HWND)lParam, &r);
			HMENU menu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_INSTALL_MENU));
			HMENU subMenu = GetSubMenu(menu, 0);

			if (m_PackageSkins.empty() || m_MergeSkins)
			{
				EnableMenuItem(subMenu, IDM_INSTALL_BACKUPSKINS, MF_BYCOMMAND | MF_GRAYED);
			}
			else
			{
				CheckMenuItem(subMenu, IDM_INSTALL_BACKUPSKINS, (m_BackupSkins ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
			}

			if (m_PackageFonts.empty())
			{
				EnableMenuItem(subMenu, IDM_INSTALL_SYSTEMFONTS, MF_BYCOMMAND | MF_GRAYED);
			}
			else
			{
				CheckMenuItem(subMenu, IDM_INSTALL_SYSTEMFONTS, (m_SystemFonts ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
			}

			const WCHAR* formatName = m_PackageFormat == PackageFormat::New ? L"New format" : L"Old format";
			ModifyMenu(subMenu, IDM_INSTALL_FORMAT, MF_STRING | MF_GRAYED | MF_BYCOMMAND, IDM_INSTALL_FORMAT, formatName);

			TrackPopupMenu(
				subMenu,
				TPM_RIGHTBUTTON | TPM_LEFTALIGN,
				r.left,
				--r.bottom,
				0,
				m_Window,
				NULL);

			DestroyMenu(menu);
		}
		break;

	case IDC_INSTALL_INSTALL_BUTTON:
		BeginInstall();
		break;

	case IDCLOSE:
		if (!m_InstallThread)
		{
			EndDialog(m_Window, 0);
		}
		break;

	case IDM_INSTALL_BACKUPSKINS:
		m_BackupSkins = !m_BackupSkins;
		break;

	case IDM_INSTALL_SYSTEMFONTS:
		m_SystemFonts = !m_SystemFonts;
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

INT_PTR CDialogInstall::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR nm = (LPNMHDR)lParam;
	switch (nm->code)
	{
	case BCN_DROPDOWN:
		{
			NMHDR* hdr = &((NMBCDROPDOWN*)lParam)->hdr;

			// Unpush the drop-down button part and simulate click
			Button_SetDropDownState(hdr->hwndFrom, FALSE);
			SendMessage(hdr->hwndFrom, BM_CLICK, 0, 0);
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

bool CDialogInstall::ExtractCurrentFile(const std::wstring& fileName)
{
	// Some archives don't explicity list directories, so create them recursively
	if (!CreateDirectoryRecursive(fileName))
	{
		return false;
	}

	if (fileName.back() == L'\\')
	{
		// Nothing left to do
		return true;
	}

	if (unzOpenCurrentFile(m_PackageUnzFile) != UNZ_OK)
	{
		return false;
	}

	HANDLE hFile = CreateFile(fileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	int read;
	do
	{
		BYTE buffer[16384];
		DWORD written;
		read = unzReadCurrentFile(m_PackageUnzFile, buffer, 16384);
		if (read < 0 || !WriteFile(hFile, (LPCVOID)buffer, read, &written, NULL) || read != written)
		{
			read = UNZ_ERRNO;
			break;
		}
	}
	while (read != UNZ_EOF);

	CloseHandle(hFile);

	return unzCloseCurrentFile(m_PackageUnzFile) == UNZ_OK && read == UNZ_EOF;
}

bool CDialogInstall::ReadPackage()
{
	const WCHAR* fileName = m_PackageFileName.c_str();
	const WCHAR* fileExtension = PathFindExtension(fileName);

	if (_wcsicmp(fileExtension, L".rmskin") == 0)
	{
		// Check if the footer is present (for new .rmskin format)
		PackageFooter footer = {0};

		FILE* file = _wfopen(fileName, L"rb");
		__int64 fileSize = 0;
		if (file)
		{
			fseek(file, -(long)sizeof(footer), SEEK_END);
			fileSize = _ftelli64(file);
			fread(&footer, sizeof(footer), 1, file);
			fclose(file);
		}

		if (strcmp(footer.key, "RMSKIN") == 0)
		{
			m_PackageFormat = PackageFormat::New;
			if (footer.size != fileSize)
			{
				return false;
			}

			if (footer.flags)
			{
				m_BackupSkins = !(footer.flags & PackageFlag::Backup);
			}
		}
	}
	else if (_wcsicmp(fileExtension, L".zip") != 0)
	{
		return false;
	}

	m_PackageUnzFile = unzOpen(ConvertToAscii(fileName).c_str());
	if (!m_PackageUnzFile)
	{
		return false;
	}

	// Temporary file to extract the options file and header bitmap
	WCHAR tempFile[MAX_PATH];
	GetTempPath(MAX_PATH, tempFile);
	GetTempFileName(tempFile, L"dat", 0, tempFile);

	WCHAR buffer[MAX_PATH];

	// Helper to sets buffer with current file name
	auto getFileInfo = [&]()->bool
	{
		char cBuffer[MAX_PATH];
		unz_file_info ufi;
		if (unzGetCurrentFileInfo(m_PackageUnzFile, &ufi, cBuffer, MAX_PATH, NULL, 0, NULL, 0) == UNZ_OK)
		{
			MultiByteToWideChar(CP_ACP, 0, cBuffer, strlen(cBuffer) + 1, buffer, MAX_PATH);
			while (WCHAR* pos = wcschr(buffer, L'/')) *pos = L'\\';
			return true;
		}

		return false;
	};

	// Loop through the contents of the archive until the settings file is found
	WCHAR* path;
	bool optionsFound = false;
	do
	{
		if (!getFileInfo())
		{
			return false;
		}

		path = wcsrchr(buffer, L'\\');
		if (!path)
		{
			path = buffer;
		}
		else
		{
			if (m_PackageFormat == PackageFormat::New)
			{
				// New package files must be in root of archive
				continue;
			}

			++path;	// Skip slash
		}

		if (_wcsicmp(path, m_PackageFormat == PackageFormat::New ? L"RMSKIN.bmp" : L"Rainstaller.bmp") == 0)
		{
			if (!ExtractCurrentFile(tempFile))
			{
				break;
			}

			HBITMAP header = (HBITMAP)LoadImage(NULL, tempFile, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
			SendMessage(GetDlgItem(m_Window, IDC_INSTALL_HEADER_BITMAP), STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)header);
		}
		else if (_wcsicmp(path, m_PackageFormat == PackageFormat::New ? L"RMSKIN.ini" : L"Rainstaller.cfg") == 0)
		{
			optionsFound = ExtractCurrentFile(tempFile) && ReadOptions(tempFile);
			break;
		}
	}
	while (unzGoToNextFile(m_PackageUnzFile) == UNZ_OK);

	DeleteFile(tempFile);

	if (!optionsFound)
	{
		return false;
	}

	// Loop through the archive a second time and find included components
	unzGoToFirstFile(m_PackageUnzFile);

	m_PackageRoot.assign(buffer, path - buffer);
	const WCHAR* root = m_PackageRoot.c_str();
	do
	{
		if (!getFileInfo())
		{
			return false;
		}

		if (wcsncmp(buffer, root, m_PackageRoot.length()) != 0)
		{
			// Ignore everything that isn't in the root directory
			continue;
		}

		WCHAR* component = buffer + m_PackageRoot.length();
		path = wcschr(component, L'\\');
		if (path)
		{
			*path = L'\0';
			++path;
		}
		else
		{
			continue;
		}

		const WCHAR* pos = wcschr(path, L'\\');
		const WCHAR* extension = PathFindExtension(pos ? pos : path);
		if (pos)
		{
			// Component with subfolders
			const std::wstring item(path, pos - path);
			const WCHAR* itemSz = item.c_str();

			if (_wcsicmp(component, L"Skins") == 0 &&
				!IsIgnoredSkin(itemSz))
			{
				m_PackageSkins.insert(item);
			}
			else if (_wcsicmp(component, L"Themes") == 0 &&
				_wcsicmp(extension, L".thm") == 0 &&
				!IsIgnoredTheme(itemSz))
			{
				m_PackageThemes.insert(item);
			}
			else if (_wcsicmp(component, L"Addons") == 0 &&
				m_PackageFormat == PackageFormat::Old &&
				!IsIgnoredAddon(itemSz))
			{
				m_PackageAddons.insert(item);
			}
			else if (_wcsicmp(component, L"Plugins") == 0 &&
				_wcsicmp(itemSz, IsWin32Build() ? L"32bit" : L"64bit") == 0 &&
				_wcsicmp(extension, L".dll") == 0 &&
				!wcschr(pos + 1, L'\\'))
			{
				const std::wstring plugin(pos + 1);
				if (!IsIgnoredPlugin(plugin.c_str()))
				{
					m_PackagePlugins.insert(plugin);
				}
			}
		}
		else
		{
			// Component with subfiles
			const std::wstring item = path;
			const WCHAR* itemSz = item.c_str();

			if (_wcsicmp(component, L"Fonts") == 0 &&
				m_PackageFormat == PackageFormat::Old &&
				_wcsicmp(extension, L".ttf") == 0)
			{
				m_PackageFonts.insert(item);
			}
		}
	}
	while (unzGoToNextFile(m_PackageUnzFile) == UNZ_OK);

	if (m_PackageSkins.empty())
	{
		// Fonts can be installed only with skins
		m_PackageFonts.clear();
	}

	return !(m_PackageSkins.empty() && m_PackageThemes.empty() &&
		m_PackageAddons.empty() && m_PackageFonts.empty() && m_PackagePlugins.empty());
}

bool CDialogInstall::ReadOptions(const WCHAR* file)
{
	WCHAR buffer[MAX_LINE_LENGTH];

	const bool newFormat = m_PackageFormat == PackageFormat::New;
	const WCHAR* section = newFormat ? L"rmskin" : L"Rainstaller";

	const HWND window = m_TabInstall.GetWindow();

	if (GetPrivateProfileString(section, L"Name", L"", buffer, 64, file) == 0)
	{
		return false;
	}
	Static_SetText(GetDlgItem(window, IDC_INSTALLTAB_NAME_TEXT), buffer);

	if (!newFormat)
	{
		// Determine if skins need to backed up based on name
		int s;
		int scanned = swscanf(buffer, L"Backup-%d.%d.%d-%d.%d.rmskin", &s, &s, &s, &s, &s);
		m_BackupSkins = scanned != 5;
	}

	GetPrivateProfileString(section, L"Author", L"", buffer, 64, file);
	Static_SetText(GetDlgItem(window, IDC_INSTALLTAB_AUTHOR_TEXT), buffer);

	GetPrivateProfileString(section, L"Version", L"", buffer, 64, file);
	Static_SetText(GetDlgItem(window, IDC_INSTALLTAB_VERSION_TEXT), buffer);

	m_MergeSkins = GetPrivateProfileInt(section, newFormat ? L"MergeSkins" : L"Merge", 0, file) != 0;
	
	GetPrivateProfileString(section, newFormat ? L"VariableFiles" : L"KeepVar", L"", buffer, MAX_LINE_LENGTH, file);
	m_VariablesFiles = Tokenize(buffer, L"|");

	if (GetPrivateProfileString(section, newFormat ? L"MinimumRainmeter" : L"MinRainmeterVer", L"", buffer, MAX_LINE_LENGTH, file) > 0)
	{
		std::wstring rainmeterDll = g_Data.programPath + L"Rainmeter.dll";
		std::wstring rainmeterVersion = GetFileVersionString(rainmeterDll.c_str());
		if (CompareVersions(buffer, rainmeterVersion) == 1)
		{
			m_ErrorMessage = L"Rainmeter ";
			m_ErrorMessage += buffer;
			m_ErrorMessage += L" or higher is required to install this package.\n\n"
				L"Get the latest version from rainmeter.net and try again.";
			return false;
		}
	}

	if (GetPrivateProfileString(section, newFormat ? L"LoadType" : L"LaunchType", L"", buffer, MAX_LINE_LENGTH, file) > 0)
	{
		bool loadSkin = _wcsicmp(buffer, newFormat ? L"Skin" : L"Load") == 0;

		GetPrivateProfileString(section, newFormat ? L"Load" : L"LaunchCommand", L"", buffer, MAX_LINE_LENGTH, file);
		if (loadSkin)
		{
			if (newFormat)
			{
				m_LoadSkins.push_back(buffer);
			}
			else
			{
				m_LoadSkins = Tokenize(buffer, L"|");
			}
		}
		else
		{
			m_LoadTheme = buffer;
		}
	}

	if (newFormat)
	{
		if (GetPrivateProfileString(section, L"MinimumDotNET", L"", buffer, MAX_LINE_LENGTH, file) > 0 &&
			CompareVersions(buffer, GetDotNetVersionString()) == 1)
		{
			m_ErrorMessage = L".NET framework ";
			m_ErrorMessage += buffer;
			m_ErrorMessage += L" or higher is required to install this package.";
			return false;
		}

		if (GetPrivateProfileString(section, L"MinimumWindows", L"", buffer, MAX_LINE_LENGTH, file) > 0 &&
			CompareVersions(buffer, GetWindowsVersionString()) == 1)
		{
			m_ErrorMessage = L"Your version of Windows is not supported by this package.\n\n"
				L"Contact the package author for more information.";
			return false;
		}
	}

	return true;
}

bool CDialogInstall::InstallPackage()
{
	if (IsPackageBlacklisted())
	{
		m_ErrorMessage = L"This package has been identified as malware by the Rainmeter Team and will not be installed.";
		return false;
	}

	if (!m_MergeSkins && m_BackupSkins)
	{
		// Move skins into backup folder
		for (auto iter = m_PackageSkins.cbegin(); iter != m_PackageSkins.cend(); ++iter)
		{
			std::wstring from = g_Data.skinsPath + *iter;
			std::wstring to = g_Data.skinsPath + L"Backup\\";
			CreateDirectory(to.c_str(), NULL);
			to += *iter;
			to += L'\0';	// For SHFileOperation

			// Delete current backup
			SHFILEOPSTRUCT fo =
			{
				NULL,
				FO_DELETE,
				to.c_str(),
				NULL,
				FOF_NO_UI | FOF_NOCONFIRMATION | FOF_ALLOWUNDO
			};
			SHFileOperation(&fo);

			if (!CopyFiles(from, to, true))
			{
				m_ErrorMessage = L"Unable to move to:\n";
				m_ErrorMessage += to;
				return false;
			}
		}
	}

	WCHAR buffer[MAX_PATH];

	// Helper to sets buffer with current file name
	auto getFileInfo = [&]()->bool
	{
		char cBuffer[MAX_PATH];
		unz_file_info ufi;
		if (unzGetCurrentFileInfo(m_PackageUnzFile, &ufi, cBuffer, MAX_PATH, NULL, 0, NULL, 0) == UNZ_OK)
		{
			MultiByteToWideChar(CP_ACP, 0, cBuffer, strlen(cBuffer) + 1, buffer, MAX_PATH);
			while (WCHAR* pos = wcschr(buffer, L'/')) *pos = L'\\';
			return true;
		}

		return false;
	};

	unzGoToFirstFile(m_PackageUnzFile);
	const WCHAR* root = m_PackageRoot.c_str();
	do
	{
		if (!getFileInfo())
		{
			m_ErrorMessage = L"Error retrieving file info.";
			return false;
		}

		if (wcsncmp(buffer, root, m_PackageRoot.length()) != 0)
		{
			// Ignore everything that isn't in the root directory
			continue;
		}

		WCHAR* component = buffer + m_PackageRoot.length();
		WCHAR* path = wcschr(component, L'\\');
		if (path)
		{
			*path = L'\0';
			++path;
		}
		else
		{
			continue;
		}

		bool error = false;
		std::wstring targetPath;

		WCHAR* pos = wcschr(path, L'\\');
		WCHAR* extension = PathFindExtension(pos ? pos : path);
		if (pos)
		{
			const std::wstring item(path, pos - path);

			if (_wcsicmp(component, L"Skins") == 0 &&
				m_PackageSkins.find(item) != m_PackageSkins.end())
			{
				targetPath = g_Data.skinsPath;
			}
			else if (_wcsicmp(component, L"Themes") == 0 &&
				_wcsicmp(extension, L".thm") == 0 &&
				m_PackageThemes.find(item) != m_PackageThemes.end())
			{
				targetPath = g_Data.settingsPath;
				targetPath += L"Themes\\";
			}
			else if (_wcsicmp(component, L"Addons") == 0 &&
				m_PackageFormat == PackageFormat::Old &&
				m_PackageAddons.find(item) != m_PackageAddons.end())
			{
				targetPath = g_Data.settingsPath;
				targetPath += L"Addons\\";
			}
			else if (_wcsicmp(component, L"Plugins") == 0 &&
				_wcsnicmp(path, IsWin32Build() ? L"32bit" : L"64bit", pos - path) == 0 &&
				_wcsicmp(extension, L".dll") == 0 &&
				!wcschr(pos + 1, L'\\'))
			{
				const std::wstring plugin(pos + 1);
				if (m_PackagePlugins.find(plugin) != m_PackagePlugins.end())
				{
					path = pos + 1;
					targetPath = g_Data.settingsPath;
					targetPath += L"Plugins\\";
				}
			}

			if (!targetPath.empty())
			{
				targetPath += path;
				error = !ExtractCurrentFile(targetPath);
			}
		}
		else
		{
			if (_wcsicmp(component, L"Fonts") == 0 &&
				m_PackageFormat == PackageFormat::Old &&
				_wcsicmp(extension, L".ttf") == 0)
			{
				for (auto iter = m_PackageSkins.cbegin(); iter != m_PackageSkins.cend(); ++iter)
				{
					targetPath = g_Data.skinsPath;
					targetPath += *iter;
					targetPath += L"\\@Resources\\Fonts\\";
					targetPath += path;
					error = !ExtractCurrentFile(targetPath);
					if (error)
					{
						break;
					}
				}
			}
		}

		if (error)
		{
			m_ErrorMessage = L"Unable to create file:\n";
			m_ErrorMessage += targetPath;
			m_ErrorMessage += L"\n\nSkin Installer will now quit.";
			return false;
		}
	}
	while (unzGoToNextFile(m_PackageUnzFile) == UNZ_OK);

	return true;
}

void CDialogInstall::BeginInstall()
{
	HWND item = GetDlgItem(m_Window, IDC_INSTALL_ADVANCED_BUTTON);
	EnableWindow(item, FALSE);

	item = GetDlgItem(m_Window, IDC_INSTALL_INSTALL_BUTTON);
	EnableWindow(item, FALSE);

	item = GetDlgItem(m_Window, IDCLOSE);
	EnableWindow(item, FALSE);

	item = GetDlgItem(m_TabInstall.GetWindow(), IDC_INSTALLTAB_THEME_CHECKBOX);
	if (Button_GetCheck(item) == BST_UNCHECKED)
	{
		m_LoadTheme.clear();
		m_LoadSkins.clear();
	}
	EnableWindow(item, FALSE);

	item = GetDlgItem(m_TabInstall.GetWindow(), IDC_INSTALLTAB_COMPONENTS_LIST);
	{
		// Remove unchecked items from the component sets
		LVITEM lvi;
		lvi.mask = LVIF_GROUPID | LVIF_PARAM;
		lvi.iSubItem = 0;
		lvi.iItem = 0;

		int itemCount = ListView_GetItemCount(item);
		for (; lvi.iItem < itemCount; ++lvi.iItem)
		{
			ListView_GetItem(item, &lvi);

			std::set<std::wstring>* component = NULL;
			switch (lvi.iGroupId)
			{
			case 0: component = &m_PackageSkins;   break;
			case 1: component = &m_PackageThemes;  break;
			case 2: component = &m_PackageAddons;  break;
			case 3: component = &m_PackagePlugins; break;
			}

			BOOL checked = ListView_GetCheckState(item, lvi.iItem);
			if (component && !checked)
			{
				component->erase(*(std::wstring*)lvi.lParam);
			}
		}
	}
	EnableWindow(item, FALSE);

	m_InstallThread = (HANDLE)_beginthreadex(NULL, 0, InstallThread, this, 0, NULL);
	if (!m_InstallThread)
	{
		MessageBox(m_Window, L"Unable to start install.", L"Rainmeter Skin Installer", MB_ERROR);
		EndDialog(m_Window, 0);
	}
}

UINT __stdcall CDialogInstall::InstallThread(void* pParam)
{
	CDialogInstall* dialog = (CDialogInstall*)pParam;

	if (!CloseRainmeterIfActive())
	{
		MessageBox(dialog->m_Window, L"Unable to close Rainmeter.", L"Rainmeter Skin Installer", MB_ERROR);
	}
	else
	{
		HWND progressText = GetDlgItem(dialog->m_TabInstall.GetWindow(), IDC_INSTALLTAB_INPROGRESS_TEXT);
		ShowWindow(progressText, SW_SHOWNORMAL);

		HWND progressBar = GetDlgItem(dialog->m_TabInstall.GetWindow(), IDC_INSTALLTAB_PROGRESS);
		ShowWindow(progressBar, SW_SHOWNORMAL);
		SendMessage(progressBar, PBM_SETMARQUEE, (WPARAM)TRUE, 0);

		if (!dialog->InstallPackage())
		{
			SetWindowText(progressText, L"Installation stopped");
			SendMessage(progressBar, PBM_SETMARQUEE, (WPARAM)FALSE, 0);

			if (dialog->m_ErrorMessage.empty())
			{
				dialog->m_ErrorMessage = L"Unknown error.";
			}
			dialog->m_ErrorMessage += L"\n\nClick OK to close Skin Installer.";

			MessageBox(dialog->m_Window, dialog->m_ErrorMessage.c_str(), L"Rainmeter Skin Installer", MB_ERROR);

			dialog->m_LoadSkins.clear();
			dialog->m_LoadTheme.clear();
		}

		dialog->LaunchRainmeter();
	}

	EndDialog(dialog->GetWindow(), 0);
	return 0;
}

bool CDialogInstall::IsPackageBlacklisted()
{
	std::string fileName = ConvertToAscii(PathFindFileName(m_PackageFileName.c_str()));
	const char* fileNameSz = fileName.c_str();

	const char* regex = "(?siU)^.*_by_([0-9A-Za-z\\-]+)-d[0-9a-z]{6}\\.rmskin$";
	const char* error;
	int errorOffset;
	pcre* re = pcre_compile(regex, PCRE_UTF8, &error, &errorOffset, NULL);

	int offsets[6];
	bool match = pcre_exec(re, NULL, fileNameSz, fileName.length(), 0, 0, offsets, _countof(offsets)) == 2;
	pcre_free(re);
	if (!match)
	{
		return false;
	}

	bool isBlacklisted = false;

	// Author is the 2nd substring
	std::string author(fileNameSz + offsets[2], offsets[3] - offsets[2]);
	std::wstring url = L"http://blacklist.rainmeter.googlecode.com/git/user/" + ConvertToWide(author.c_str());

	HINTERNET internet = InternetOpen(L"Mozilla/5.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (internet)
	{
		DWORD openFlags = INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_NO_UI;
		HINTERNET internetUrl = InternetOpenUrl(internet, url.c_str(), NULL, 0, openFlags, 0);
		if (internetUrl)
		{
			DWORD queryFlags = HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER;
			DWORD statusCode;
			DWORD size = sizeof(DWORD);
			BOOL query = HttpQueryInfo(internetUrl, queryFlags, &statusCode, &size, NULL);
			InternetCloseHandle(internetUrl);

			isBlacklisted = query && statusCode == HTTP_STATUS_OK;
		}

		InternetCloseHandle(internet);
	}

	return isBlacklisted;
}

void CDialogInstall::KeepVariables()
{
	WCHAR keyname[32767];	// Max size returned by GetPrivateProfileSection
	WCHAR buffer[4];
	std::wstring currKey, currValue;

	for (int i = 0, isize = m_VariablesFiles.size(); i < isize; ++i)
	{
		std::wstring fromPath = g_Data.skinsPath + L"Backup\\";
		fromPath += m_VariablesFiles[i];
		std::wstring toPath = g_Data.skinsPath + m_VariablesFiles[i];

		unsigned int count = GetPrivateProfileSection(L"Variables", keyname, 32767, fromPath.c_str());

		if ((_waccess(fromPath.c_str(), 0) == 0) && (_waccess(toPath.c_str(), 0) == 0)
			&& (count > 0))
		{
			for (unsigned int j = 0; j < count; ++j)
			{
				if (keyname[j] == L'=')
				{
					if (GetPrivateProfileString(L"Variables", currKey.c_str(), NULL, buffer, 4, toPath.c_str()) > 0)
					{
						while (keyname[++j] != L'\0') currValue += keyname[j];
						WritePrivateProfileString(L"Variables", currKey.c_str(), currValue.c_str(), toPath.c_str());
						currValue.clear();
					}
					else
					{
						while (keyname[j] != L'\0') ++j;
					}
					currKey.clear();
				}
				else
				{
					currKey += keyname[j];
				}
			}
		}
	}
}

void CDialogInstall::LoadTheme(const std::wstring& name, bool setWallpaper)
{
	// Take a copy of current Rainmeter.ini before doing anything
	std::wstring backupFile = g_Data.settingsPath;
	backupFile += L"Themes\\Backup\\";
	CreateDirectory(backupFile.c_str(), NULL);
	backupFile += L"Rainmeter.thm";
	CopyFiles(g_Data.iniFile, backupFile, false);

	if (name.empty())
	{
		return;
	}

	std::wstring themeFile = g_Data.settingsPath;
	themeFile += L"Themes\\";
	themeFile += name;
	std::wstring wallpaperFile = themeFile + L"\\RainThemes.bmp";
	themeFile += L"\\Rainmeter.thm";
	if (_waccess(themeFile.c_str(), 0) != -1)
	{
		CopyFiles(themeFile, g_Data.iniFile, false);

		const WCHAR* iniFileSz = g_Data.iniFile.c_str();
		const WCHAR* backupFileSz = backupFile.c_str();

		auto preserveOption = [&](LPCTSTR section, LPCTSTR key)
		{
			WCHAR buffer[MAX_LINE_LENGTH];
			if (GetPrivateProfileString(section, key, L"", buffer, MAX_LINE_LENGTH, iniFileSz) == 0 &&
				GetPrivateProfileString(section, key, L"", buffer, MAX_LINE_LENGTH, backupFileSz) > 0)
			{
				WritePrivateProfileString(section, key, buffer, iniFileSz);
			}
		};

		preserveOption(L"Rainmeter", L"SkinPath");
		preserveOption(L"Rainmeter", L"ConfigEditor");
		preserveOption(L"Rainmeter", L"LogViewer");
		preserveOption(L"Rainmeter", L"Logging");
		preserveOption(L"Rainmeter", L"DisableVersionCheck");
		preserveOption(L"Rainmeter", L"Language");
		preserveOption(L"Rainmeter", L"TrayExecuteM");
		preserveOption(L"Rainmeter", L"TrayExecuteR");
		preserveOption(L"Rainmeter", L"TrayExecuteDM");
		preserveOption(L"Rainmeter", L"TrayExecuteDR");

		// Set wallpaper if it exists
		const WCHAR* wallpaperFileSz = wallpaperFile.c_str();
		if (_waccess(wallpaperFileSz, 0) != -1)
		{
			SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (void*)wallpaperFileSz, SPIF_UPDATEINIFILE);
		}
	}
}

void CDialogInstall::LaunchRainmeter()
{
	// Backup Rainmeter.ini and load theme (if specified)
	LoadTheme(m_LoadTheme, false);

	// Execute Rainmeter and wait up to a minute for it process all messages
	std::wstring rainmeterExe = g_Data.programPath + L"Rainmeter.exe";

	SHELLEXECUTEINFO sei = {0};
	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.fMask = SEE_MASK_WAITFORINPUTIDLE | SEE_MASK_UNICODE;
	sei.hwnd = NULL;
	sei.lpVerb = NULL;
	sei.lpFile = rainmeterExe.c_str();
	sei.lpDirectory = g_Data.programPath.c_str();
	sei.lpParameters = NULL;
	sei.nShow = SW_SHOWNORMAL;
	ShellExecuteEx(&sei);

	if (!m_LoadSkins.empty())
	{
		std::wstring::size_type pos;
		std::wstring bang;

		for (int i = 0, isize = (int)m_LoadSkins.size(); i < isize; ++i)
		{
			const std::wstring& skinName = m_LoadSkins[i];
			pos = skinName.find_last_of(L"\\");
			if (pos != std::wstring::npos)
			{
				// Append with [!ActivateConfig "Config" "File.ini"]
				bang += L"[!ActivateConfig \"";
				bang.append(skinName, 0, pos);
				bang += L"\" \"";;
				bang.append(skinName, pos + 1, skinName.length() - pos + 1);
				bang += L"\"]";
			}
		}

		if (!bang.empty())
		{
			sei.fMask = SEE_MASK_UNICODE;
			sei.lpParameters = (LPCTSTR)bang.c_str();
			ShellExecuteEx(&sei);
		}
	}
}

bool CDialogInstall::IsIgnoredSkin(const WCHAR* name)
{
	return _wcsicmp(name, L"Backup") == 0;
}

bool CDialogInstall::IsIgnoredTheme(const WCHAR* name)
{
	return _wcsicmp(name, L"Backup") == 0;
}

bool CDialogInstall::IsIgnoredAddon(const WCHAR* name)
{
	return _wcsicmp(name, L"Backup") == 0 ||
		_wcsicmp(name, L"Rainstaller") == 0 ||
		_wcsicmp(name, L"RainBackup") == 0;
}

bool CDialogInstall::IsIgnoredPlugin(const WCHAR* name)
{
	return _wcsicmp(name, L"AdvancedCPU.dll") == 0 ||
		_wcsicmp(name, L"CoreTemp.dll") == 0 ||
		_wcsicmp(name, L"FolderInfo.dll") == 0 ||
		_wcsicmp(name, L"InputText.dll") == 0 ||
		_wcsicmp(name, L"iTunesPlugin.dll") == 0 ||
		_wcsicmp(name, L"MediaKey.dll") == 0 ||
		_wcsicmp(name, L"NowPlaying.dll") == 0 ||
		_wcsicmp(name, L"PerfMon.dll") == 0 ||
		_wcsicmp(name, L"PingPlugin.dll") == 0 ||
		_wcsicmp(name, L"PowerPlugin.dll") == 0 ||
		_wcsicmp(name, L"Process.dll") == 0 ||
		_wcsicmp(name, L"QuotePlugin.dll") == 0 ||
		_wcsicmp(name, L"RecycleManager.dll") == 0 ||
		_wcsicmp(name, L"ResMon.dll") == 0 ||
		_wcsicmp(name, L"SpeedFanPlugin.dll") == 0 ||
		_wcsicmp(name, L"SysInfo.dll") == 0 ||
		_wcsicmp(name, L"VirtualDesktops.dll") == 0 ||
		_wcsicmp(name, L"WebParser.dll") == 0 ||
		_wcsicmp(name, L"WifiStatus.dll") == 0 ||
		_wcsicmp(name, L"Win7AudioPlugin.dll") == 0 ||
		_wcsicmp(name, L"WindowMessagePlugin.dll") == 0;
}

/*
** Splits the string from the delimiters and trims whitespace.
*/
std::vector<std::wstring> CDialogInstall::Tokenize(const std::wstring& str, const std::wstring& delimiters)
{
	// Modified from http://www.digitalpeer.com/id/simple
	std::vector<std::wstring> tokens;
	std::wstring::size_type lastPos = str.find_first_not_of(delimiters, 0);	// Skip delimiters at beginning
	std::wstring::size_type pos = str.find_first_of(delimiters, lastPos);	// Find first "non-delimiter"

	while (std::wstring::npos != pos || std::wstring::npos != lastPos)
	{
		std::wstring tmpStr = str.substr(lastPos, pos - lastPos);
		std::wstring::size_type tmpPos = tmpStr.find_first_not_of(L" \t");
		if (tmpPos != std::wstring::npos)
		{
			tmpStr.erase(0, tmpPos);
			tmpPos = tmpStr.find_last_not_of(L" \t");
			if (tmpPos != std::wstring::npos)
			{
				tmpStr.resize(tmpPos + 1);
			}
			tokens.push_back(tmpStr);
		}
		else
		{
			tokens.push_back(L"");	// Add empty string
		}
		lastPos = str.find_first_not_of(delimiters, pos);	// Skip delimiters. Note the "not_of"
		pos = str.find_first_of(delimiters, lastPos);		// Find next "non-delimiter"
	}

	return tokens;
}

/*
** Compares two version strings. Returns 0 if equal, 1 if A > B and -1 if A < B.
*/
int CDialogInstall::CompareVersions(const std::wstring& strA, const std::wstring& strB)
{
	if (strA.empty() && strB.empty()) return 0;
	if (strA.empty()) return -1;
	if (strB.empty()) return 1;

	std::vector<std::wstring> arrayA = Tokenize(strA, L".");
	std::vector<std::wstring> arrayB = Tokenize(strB, L".");

	size_t len = max(arrayA.size(), arrayB.size());
	for (size_t i = 0; i < len; ++i)
	{
		int a = 0;
		int b = 0;

		if (i < arrayA.size())
		{
			a = _wtoi(arrayA[i].c_str());
		}
		if (i < arrayB.size())
		{
			b = _wtoi(arrayB[i].c_str());
		}

		if (a > b) return 1;
		if (a < b) return -1;
	}
	return 0;
}

bool CDialogInstall::CreateDirectoryRecursive(const std::wstring& path)
{
	// Dirty...
	std::wstring& directory = (std::wstring&)path;
	const WCHAR* directorySz = directory.c_str();

	bool failed = true;
	std::wstring::size_type pos = std::wstring::npos;
	while ((pos = failed ? directory.find_last_of(L'\\', pos) : directory.find_first_of(L'\\', pos)) != std::wstring::npos)
	{
		// Temporarily terminate string
		directory[pos] = L'\0';

		failed = CreateDirectory(directorySz, NULL) == 0 && GetLastError() == ERROR_PATH_NOT_FOUND;

		// Restore slash
		directory[pos] = L'\\';

		pos += failed ? -1 : 1;
	}

	return !failed;
}

std::wstring CDialogInstall::GetFileVersionString(const WCHAR* fileName)
{
	DWORD bufSize = GetFileVersionInfoSize(fileName, 0);
	void* versionInfo = new WCHAR[bufSize];
	void* fileVersion = 0;
	UINT valueSize;
	std::wstring result;

	if (GetFileVersionInfo(fileName, 0, bufSize, versionInfo))
	{
		struct LANGANDCODEPAGE
		{
			WORD wLanguage;
			WORD wCodePage;
		} *languageInfo;

		VerQueryValue(versionInfo, L"\\VarFileInfo\\Translation", (LPVOID*)&languageInfo, &valueSize);
		WCHAR blockName[64];
		_snwprintf_s(blockName, _TRUNCATE, L"\\StringFileInfo\\%04x%04x\\FileVersion", languageInfo[0].wLanguage, languageInfo[0].wCodePage);

		VerQueryValue(versionInfo, blockName, &fileVersion, &valueSize);
		if (valueSize)
		{
			result = (WCHAR*)fileVersion;
		}
	}

	delete [] (WCHAR*)versionInfo;
	return result;
}

std::wstring CDialogInstall::GetDotNetVersionString()
{
	WCHAR buffer[255];
	HKEY hKey;
	LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\NET Framework Setup\\NDP", 0L, KEY_READ, &hKey);
	std::wstring currVer(L"v0"), prevVer;
	int i = 0;

	while (lRet == ERROR_SUCCESS)
	{
		lRet = RegEnumKey(hKey, i, buffer, 255);
		if (buffer[0] == L'v')
		{
			currVer = buffer;
		}
		++i;
	}

	RegCloseKey(hKey);
	currVer.erase(0, 1); // Get rid of the 'v'
	return currVer;
}

std::wstring CDialogInstall::GetWindowsVersionString()
{
	WCHAR buffer[16];
	OSVERSIONINFOEX osvi = {sizeof(OSVERSIONINFOEX)};
	GetVersionEx((OSVERSIONINFO*)&osvi);
	_snwprintf_s(buffer, _TRUNCATE, L"%d.%d.%d", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);

	return buffer;
}

// -----------------------------------------------------------------------------------------------
//
//                                Backup tab
//
// -----------------------------------------------------------------------------------------------

/*
** Constructor.
**
*/
CDialogInstall::CTabInstall::CTabInstall(HWND wnd) : CTab(GetModuleHandle(NULL), wnd, IDD_INSTALL_TAB, DlgProc)
{
}

void CDialogInstall::CTabInstall::Initialize()
{
	HWND item = GetDlgItem(m_Window, IDC_INSTALLTAB_COMPONENTS_LIST);

	ListView_SetExtendedListViewStyleEx(item, 0, LVS_EX_CHECKBOXES | LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT);
	ListView_EnableGroupView(item, TRUE);

	// Add columns
	LVCOLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.iSubItem = 0;
	lvc.cx = 180;
	lvc.pszText = L"Name";
	ListView_InsertColumn(item, 0, &lvc);
	lvc.iSubItem = 1;
	lvc.cx = 150;
	lvc.pszText = L"Action";
	ListView_InsertColumn(item, 1, &lvc);

	// Add groups and items
	LVGROUP lvg;
	lvg.cbSize = sizeof(LVGROUP);
	lvg.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
	lvg.state = (GetOSPlatform() >= OSPLATFORM_VISTA) ? LVGS_COLLAPSIBLE : LVGS_NORMAL;

	LVITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_GROUPID | LVIF_PARAM;
	lvi.iSubItem = 0;

	auto addComponent = [&](const WCHAR* name, const std::set<std::wstring>& items, const std::wstring& path, int groupId)
	{
		lvg.iGroupId = groupId;
		lvg.pszHeader = (WCHAR*)name;
		ListView_InsertGroup(item, groupId, &lvg);

		lvi.iGroupId = groupId;
		lvi.iItem = 0;
		for (auto iter = items.cbegin(); iter != items.cend(); ++iter)
		{
			lvi.pszText = (WCHAR*)(*iter).c_str();
			lvi.lParam = (LPARAM)&(*iter);
			ListView_InsertItem(item, &lvi);
			ListView_SetCheckState(item, lvi.iItem, TRUE);

			std::wstring itemPath = path + *iter;
			WCHAR* text = L"Add";
			if (_waccess(itemPath.c_str(), 0) != -1)
			{
				text = (groupId == 0 && c_Dialog->m_BackupSkins) ? L"Backup and replace" : L"Replace";
			}
			ListView_SetItemText(item, lvi.iItem, 1, text);

			++lvi.iItem;
		}
	};

	addComponent(L"Skins", c_Dialog->m_PackageSkins, g_Data.skinsPath, 0);
	addComponent(L"Themes", c_Dialog->m_PackageThemes, g_Data.settingsPath + L"Themes\\", 1);
	addComponent(L"Addons", c_Dialog->m_PackageAddons, g_Data.settingsPath + L"Addons\\", 2);
	addComponent(L"Plugins", c_Dialog->m_PackagePlugins, g_Data.settingsPath + L"Plugins\\", 3);

	item = GetDlgItem(m_Window, IDC_INSTALLTAB_THEME_CHECKBOX);
	if (!c_Dialog->m_LoadTheme.empty())
	{
		Button_SetCheck(item, BST_CHECKED);
	}
	else if (!c_Dialog->m_LoadSkins.empty())
	{
		SetWindowText(item, L"Load included skins");
		Button_SetCheck(item, BST_CHECKED);
	}
	else
	{
		ShowWindow(item, SW_HIDE);
	}

	m_Initialized = true;
}

INT_PTR CALLBACK CDialogInstall::CTabInstall::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}
