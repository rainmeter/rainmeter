/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "DialogInstall.h"
#include "../Common/StringUtil.h"
#include "SkinInstaller.h"
#include "resource.h"
#include "System.h"
#include "../Version.h"

#define WM_DELAYED_CLOSE WM_APP + 0

extern GlobalData g_Data;

DialogInstall* DialogInstall::c_Dialog = nullptr;

inline bool IsWin32Build()
{
#ifdef _WIN64
	return false;
#else
	return true;
#endif
}

/*
** Constructor.
**
*/
DialogInstall::DialogInstall(HWND wnd, const WCHAR* file) : OldDialog(wnd),
	m_TabInstall(wnd),
	m_HeaderBitmap(),
	m_InstallThread(),
	m_PackageUnzFile(),
	m_PackageFileName(file),
	m_PackageFormat(PackageFormat::Old),
	m_BackupPackage(false),
	m_BackupSkins(true),
	m_MergeSkins(false),
	m_SystemFonts(false)
{
}

/*
** Destructor.
**
*/
DialogInstall::~DialogInstall()
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
void DialogInstall::Create(HINSTANCE hInstance, LPWSTR lpCmdLine)
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
	if (IsRunning(L"Rainmeter Skin Installer", &hMutex))
	{
		HWND hwnd = FindWindow(L"#32770", L"Rainmeter Skin Installer");
		SetForegroundWindow(hwnd);
	}
	else
	{
		DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_INSTALL_DIALOG), nullptr, (DLGPROC)DlgProc, (LPARAM)lpCmdLine);
		ReleaseMutex(hMutex);
	}
}

OldDialog::Tab& DialogInstall::GetActiveTab()
{
	return m_TabInstall;
}

INT_PTR CALLBACK DialogInstall::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!c_Dialog)
	{
		if (uMsg == WM_INITDIALOG)
		{
			c_Dialog = new DialogInstall(hWnd, (const WCHAR*)lParam);
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
			if (!c_Dialog->m_InstallThread)
			{
				EndDialog(hWnd, 0);
			}
			return TRUE;

		case WM_DESTROY:
			delete c_Dialog;
			c_Dialog = nullptr;
			return FALSE;
		}
	}

	return FALSE;
}

INT_PTR DialogInstall::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	HICON hIcon = (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_SKININSTALLER), IMAGE_ICON, 16, 16, LR_SHARED);
	SendMessage(m_Window, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	SetDialogFont();

	HWND item = GetDlgItem(m_Window, IDC_INSTALL_ADVANCED_BUTTON);
	OldDialog::SetMenuButton(item);

	if (ReadPackage())
	{
		item = GetDlgItem(m_Window, IDC_INSTALL_HEADER_BITMAP);
		if (m_HeaderBitmap)
		{
			SendMessage(item, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)m_HeaderBitmap);
		}
		else
		{
			RECT r;
			GetClientRect(item, &r);
			ShowWindow(item, SW_HIDE);
			int yDiff = r.bottom;

			// Move all controls on the main dialog up to "fill" header area.
			int controlIds[] = { IDC_INSTALL_TAB, IDC_INSTALL_ADVANCED_BUTTON, IDC_INSTALL_INSTALL_BUTTON, IDCANCEL, 0 };
			for (int i = 0; i < _countof(controlIds); ++i)
			{
				HWND control = controlIds[i] ? GetDlgItem(m_Window, controlIds[i]) : m_TabInstall.GetWindow();
				GetWindowRect(control, &r);
				MapWindowPoints(nullptr, m_Window, (POINT*)&r, sizeof(RECT) / sizeof(POINT));
				MoveWindow(control, r.left, r.top - yDiff, r.right - r.left, r.bottom - r.top, TRUE);
			}

			// Remove blank area at the bottom of the dialog and center it.
			GetWindowRect(m_Window, &r);
			MoveWindow(m_Window, r.left, r.top + (yDiff / 2), r.right - r.left, r.bottom - r.top - yDiff, TRUE);
		}

		m_TabInstall.Activate();
	}
	else
	{
		if (m_ErrorMessage.empty())
		{
			m_ErrorMessage = L"Invalid package:\n";
			m_ErrorMessage += PathFindFileName(m_PackageFileName.c_str());
			m_ErrorMessage += L"\n\nThe Skin Packager tool must be used to create valid .rmskin packages.";
		}

		MessageBox(nullptr, m_ErrorMessage.c_str(), L"Rainmeter Skin Installer", MB_ERROR);
		EndDialog(m_Window, 0);
	}


	return TRUE;
}

INT_PTR DialogInstall::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_INSTALL_ADVANCED_BUTTON:
		{
			RECT r;
			GetWindowRect((HWND)lParam, &r);
			HMENU menu = LoadMenu(GetInstanceHandle(), MAKEINTRESOURCE(IDR_INSTALL_MENU));
			HMENU subMenu = GetSubMenu(menu, 0);

			if (m_PackageSkins.empty() || m_MergeSkins || m_BackupPackage)
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
				nullptr);

			DestroyMenu(menu);
		}
		break;

	case IDC_INSTALL_INSTALL_BUTTON:
		BeginInstall();
		break;

	case IDCANCEL:
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

INT_PTR DialogInstall::OnNotify(WPARAM wParam, LPARAM lParam)
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

bool DialogInstall::ExtractCurrentFile(const std::wstring& fileName)
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

	HANDLE hFile = CreateFile(fileName.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
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
		if (read < 0 || !WriteFile(hFile, (LPCVOID)buffer, read, &written, nullptr) || read != written)
		{
			read = UNZ_ERRNO;
			break;
		}
	}
	while (read != UNZ_EOF);

	CloseHandle(hFile);

	return unzCloseCurrentFile(m_PackageUnzFile) == UNZ_OK && read == UNZ_EOF;
}

bool DialogInstall::ReadPackage()
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
				m_BackupPackage = !(footer.flags & PackageFlag::Backup);
			}
		}
	}
	else if (_wcsicmp(fileExtension, L".zip") != 0)
	{
		return false;
	}

	m_PackageUnzFile = unzOpen(StringUtil::Narrow(fileName).c_str());
	if (!m_PackageUnzFile)
	{
		return false;
	}

	WCHAR buffer[MAX_PATH];

	// Get temporary file to extract the options file and header bitmap
	GetTempPath(MAX_PATH, buffer);
	GetTempFileName(buffer, L"dat", 0, buffer);
	std::wstring tempFile = buffer;
	const WCHAR* tempFileSz = tempFile.c_str();

	// Helper to sets buffer with current file name
	auto getFileInfo = [&]()->bool
	{
		char cBuffer[MAX_PATH * 3];
		unz_file_info ufi;
		if (unzGetCurrentFileInfo(
				m_PackageUnzFile, &ufi, cBuffer, _countof(cBuffer), nullptr, 0, nullptr, 0) == UNZ_OK)
		{
			const uLong ZIP_UTF8_FLAG = 1 << 11;
			const DWORD codePage = (ufi.flag & ZIP_UTF8_FLAG) ? CP_UTF8 : CP_ACP;
			MultiByteToWideChar(codePage, 0, cBuffer, (int)strlen(cBuffer) + 1, buffer, MAX_PATH);
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

		if (_wcsicmp(path, m_PackageFormat == PackageFormat::New ? L"RMSKIN.ini" : L"Rainstaller.cfg") == 0)
		{
			if (ExtractCurrentFile(tempFile))
			{
				optionsFound = ReadOptions(tempFileSz);
				DeleteFile(tempFileSz);
			}

			break;
		}
	}
	while (unzGoToNextFile(m_PackageUnzFile) == UNZ_OK);

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
			if (_wcsicmp(component, m_PackageFormat == PackageFormat::New ? L"RMSKIN.bmp" : L"Rainstaller.bmp") == 0)
			{
				if (!ExtractCurrentFile(tempFile))
				{
					return false;
				}

				m_HeaderBitmap = (HBITMAP)LoadImage(nullptr, tempFileSz, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
				DeleteFile(tempFileSz);
			}

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
			else if (_wcsicmp(component, m_PackageFormat == PackageFormat::New ? L"Layouts" : L"Themes") == 0 &&
				_wcsicmp(extension, m_PackageFormat == PackageFormat::New ? L".ini" : L".thm") == 0 &&
				!IsIgnoredLayout(itemSz))
			{
				m_PackageLayouts.insert(item);
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

	return !(m_PackageSkins.empty() && m_PackageLayouts.empty() &&
		m_PackageAddons.empty() && m_PackageFonts.empty() && m_PackagePlugins.empty());
}

bool DialogInstall::ReadOptions(const WCHAR* file)
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
		m_BackupPackage = scanned == 5;
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
			m_LoadLayout = buffer;
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

bool DialogInstall::InstallPackage()
{
	if ((!m_MergeSkins && m_BackupSkins) || m_BackupPackage)
	{
		// Move skins into backup folder
		for (auto iter = m_PackageSkins.cbegin(); iter != m_PackageSkins.cend(); ++iter)
		{
			std::wstring from = g_Data.skinsPath + *iter;
			if (_waccess(from.c_str(), 0) == -1)
			{
				continue;
			}

			SHFILEOPSTRUCT fo =
			{
				nullptr,
				FO_DELETE,
				nullptr,
				nullptr,
				FOF_NO_UI | FOF_NOCONFIRMATION | FOF_ALLOWUNDO
			};

			if (m_BackupPackage)
			{
				// Remove current skin
				from += L'\0';
				fo.pFrom = from.c_str();
				SHFileOperation(&fo);
			}
			else
			{
				std::wstring to = g_Data.skinsPath + L"@Backup\\";
				CreateDirectory(to.c_str(), nullptr);

				// Delete current backup
				to += *iter;
				to += L'\0';
				fo.pFrom = to.c_str();
				SHFileOperation(&fo);

				if (!System::CopyFiles(from, to, true))
				{
					m_ErrorMessage = L"Unable to move to:\n";
					m_ErrorMessage += to;
					return false;
				}
			}
		}
	}

	WCHAR buffer[MAX_PATH];

	// Helper to sets buffer with current file name
	auto getFileInfo = [&]()->bool
	{
		char cBuffer[MAX_PATH * 3];
		unz_file_info ufi;
		if (unzGetCurrentFileInfo(
				m_PackageUnzFile, &ufi, cBuffer, _countof(cBuffer), nullptr, 0, nullptr, 0) == UNZ_OK)
		{
			const uLong ZIP_UTF8_FLAG = 1 << 11;
			const DWORD codePage = (ufi.flag & ZIP_UTF8_FLAG) ? CP_UTF8 : CP_ACP;
			MultiByteToWideChar(codePage, 0, cBuffer, (int)strlen(cBuffer) + 1, buffer, MAX_PATH);
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
			else if (_wcsicmp(component, m_PackageFormat == PackageFormat::New ? L"Layouts" : L"Themes") == 0 &&
				_wcsicmp(extension, m_PackageFormat == PackageFormat::New ? L".ini" : L".thm") == 0 &&
				m_PackageLayouts.find(item) != m_PackageLayouts.end())
			{
				if (m_PackageFormat == PackageFormat::Old)
				{
					wcscpy_s(extension, 5, L".ini");
				}

				targetPath = g_Data.settingsPath;
				targetPath += L"Layouts\\";
				targetPath += path;
				error = !ExtractCurrentFile(targetPath);
				if (!error)
				{
					CleanLayoutFile(targetPath.c_str());
				}
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

	if (!m_MergeSkins && m_BackupSkins)
	{
		KeepVariables();
	}

	return true;
}

void DialogInstall::BeginInstall()
{
	HWND item = GetDlgItem(m_Window, IDC_INSTALL_ADVANCED_BUTTON);
	EnableWindow(item, FALSE);

	item = GetDlgItem(m_Window, IDC_INSTALL_INSTALL_BUTTON);
	EnableWindow(item, FALSE);

	item = GetDlgItem(m_Window, IDCANCEL);
	EnableWindow(item, FALSE);

	item = GetDlgItem(m_TabInstall.GetWindow(), IDC_INSTALLTAB_THEME_CHECKBOX);
	if (Button_GetCheck(item) == BST_UNCHECKED)
	{
		m_LoadLayout.clear();
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

			std::set<std::wstring>* component = nullptr;
			switch (lvi.iGroupId)
			{
			case 0: component = &m_PackageSkins;   break;
			case 1: component = &m_PackageLayouts; break;
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

	m_InstallThread = (HANDLE)_beginthreadex(nullptr, 0, InstallThread, this, 0, nullptr);
	if (!m_InstallThread)
	{
		MessageBox(m_Window, L"Unable to start install.", L"Rainmeter Skin Installer", MB_ERROR);
		EndDialog(m_Window, 0);
	}
}

UINT __stdcall DialogInstall::InstallThread(void* pParam)
{
	DialogInstall* dialog = (DialogInstall*)pParam;

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
			ShowWindow(progressText, SW_HIDE);
			ShowWindow(progressBar, SW_HIDE);

			if (dialog->m_ErrorMessage.empty())
			{
				dialog->m_ErrorMessage = L"Unknown error.";
			}
			dialog->m_ErrorMessage += L"\n\nClick OK to close Skin Installer.";

			MessageBox(dialog->m_Window, dialog->m_ErrorMessage.c_str(), L"Rainmeter Skin Installer", MB_ERROR);

			dialog->m_LoadSkins.clear();
			dialog->m_LoadLayout.clear();
		}

		dialog->LaunchRainmeter();
	}

	EndDialog(dialog->GetWindow(), 0);
	return 0;
}

void DialogInstall::KeepVariables()
{
	WCHAR keyname[32767];	// Max size returned by GetPrivateProfileSection
	WCHAR buffer[4];
	std::wstring currKey, currValue;

	for (size_t i = 0, isize = m_VariablesFiles.size(); i < isize; ++i)
	{
		std::wstring fromPath = g_Data.skinsPath + L"@Backup\\";
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
					if (GetPrivateProfileString(L"Variables", currKey.c_str(), nullptr, buffer, 4, toPath.c_str()) > 0)
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

void DialogInstall::LaunchRainmeter()
{
	// Execute Rainmeter and wait up to a minute for it process all messages
	std::wstring rainmeterExe = g_Data.programPath + L"Rainmeter.exe";
	std::wstring args;
	if (!m_LoadLayout.empty())
	{
		args += L"!LoadLayout \"";
		args += m_LoadLayout;
		args += L'"';
	}

	SHELLEXECUTEINFO sei = {0};
	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.fMask = SEE_MASK_WAITFORINPUTIDLE | SEE_MASK_UNICODE;
	sei.lpFile = rainmeterExe.c_str();
	sei.lpParameters = args.c_str();
	sei.lpDirectory = g_Data.programPath.c_str();
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

void DialogInstall::CleanLayoutFile(const WCHAR* file)
{
	// Clear the [Rainmeter] section.
	WritePrivateProfileSection(L"Rainmeter", L"", file);
}

// Helper for the IsIgnore... functions.
bool IsIgnoredName(const WCHAR* name, const WCHAR* names[], int namesCount)
{
	for (int i = 0; i < namesCount; ++i)
	{
		if (_wcsicmp(name, names[i]) == 0)
		{
			return true;
		}
	}

	return false;
}

bool DialogInstall::IsIgnoredSkin(const WCHAR* name)
{
	static const WCHAR* s_Skins[] =
	{
		L"Backup",
		L"@Backup"
	};

	return IsIgnoredName(name, s_Skins, _countof(s_Skins));
}

bool DialogInstall::IsIgnoredLayout(const WCHAR* name)
{
	static const WCHAR* s_Layouts[] =
	{
		L"Backup",
		L"@Backup"
	};

	return IsIgnoredName(name, s_Layouts, _countof(s_Layouts));
}

bool DialogInstall::IsIgnoredAddon(const WCHAR* name)
{
	static const WCHAR* s_Addons[] =
	{
		L"Backup",
		L"Rainstaller",
		L"RainBackup"
	};

	return IsIgnoredName(name, s_Addons, _countof(s_Addons));
}

bool DialogInstall::IsIgnoredPlugin(const WCHAR* name)
{
	static const WCHAR* s_Plugins[] =
	{
		L"ActionTimer.dll",
		L"AdvancedCPU.dll",
		L"AudioLevel.dll",
		L"CoreTemp.dll",
		L"FileView.dll",
		L"FolderInfo.dll",
		L"InputText.dll",
		L"iTunesPlugin.dll",
		L"MediaKey.dll",
		L"NowPlaying.dll",
		L"PerfMon.dll",
		L"PingPlugin.dll",
		L"PowerPlugin.dll",
		L"Process.dll",
		L"QuotePlugin.dll",
		L"RecycleManager.dll",
		L"ResMon.dll",
		L"RunCommand.dll",
		L"SpeedFanPlugin.dll",
		L"SysInfo.dll",
		L"WebParser.dll",
		L"WifiStatus.dll",
		L"Win7AudioPlugin.dll",
		L"WindowMessagePlugin.dll"
	};

	return IsIgnoredName(name, s_Plugins, _countof(s_Plugins));
}

/*
** Splits the string from the delimiters and trims whitespace.
*/
std::vector<std::wstring> DialogInstall::Tokenize(const std::wstring& str, const std::wstring& delimiters)
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
int DialogInstall::CompareVersions(const std::wstring& strA, const std::wstring& strB)
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

bool DialogInstall::CreateDirectoryRecursive(const std::wstring& path)
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

		failed = CreateDirectory(directorySz, nullptr) == 0 && GetLastError() == ERROR_PATH_NOT_FOUND;

		// Restore slash
		directory[pos] = L'\\';

		pos += failed ? -1 : 1;
	}

	return !failed;
}

std::wstring DialogInstall::GetFileVersionString(const WCHAR* fileName)
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

std::wstring DialogInstall::GetDotNetVersionString()
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

std::wstring DialogInstall::GetWindowsVersionString()
{
	WCHAR buffer[16];
	OSVERSIONINFOEX osvi = {sizeof(OSVERSIONINFOEX)};
	GetVersionEx((OSVERSIONINFO*)&osvi);
	_snwprintf_s(buffer, _TRUNCATE, L"%d.%d.%d", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);

	return buffer;
}

int DialogInstall::IsPluginNewer(const std::wstring& item, const std::wstring& itemPath)
{
	std::wstring itemVersion;
	WCHAR buffer[MAX_PATH];

	// Get temporary file to extract the plugin file
	GetTempPath(MAX_PATH, buffer);
	GetTempFileName(buffer, L"dat", 0, buffer);
	std::wstring tempFile = buffer;
	const WCHAR* tempFileSz = tempFile.c_str();

	// Helper to sets buffer with current file name
	auto getFileInfo = [&]()->bool
	{
		char cBuffer[MAX_PATH * 3];
		unz_file_info ufi;
		if (unzGetCurrentFileInfo(
			m_PackageUnzFile, &ufi, cBuffer, _countof(cBuffer), nullptr, 0, nullptr, 0) == UNZ_OK)
		{
			const uLong ZIP_UTF8_FLAG = 1 << 11;
			const DWORD codePage = (ufi.flag & ZIP_UTF8_FLAG) ? CP_UTF8 : CP_ACP;
			MultiByteToWideChar(codePage, 0, cBuffer, (int)strlen(cBuffer) + 1, buffer, MAX_PATH);
			while (WCHAR* pos = wcschr(buffer, L'/')) *pos = L'\\';
			return true;
		}

		return false;
	};

	unzGoToFirstFile(m_PackageUnzFile);

	// Loop through the contents of the archive until the plugin file is found
	WCHAR* path;
	do
	{
		if (!getFileInfo())
		{
			return 0;
		}

		path = wcsrchr(buffer, L'\\');
		if (!path)
		{
			path = buffer;
		}
		else
		{
			++path;	// Skip slash
		}

		if (_wcsicmp(path, item.c_str()) == 0)
		{
			if (ExtractCurrentFile(tempFile))
			{
				itemVersion = GetFileVersionString(tempFileSz);
				DeleteFile(tempFileSz);
			}

			break;
		}
	} while (unzGoToNextFile(m_PackageUnzFile) == UNZ_OK);

	return CompareVersions(GetFileVersionString(itemPath.c_str()), itemVersion);
}

// -----------------------------------------------------------------------------------------------
//
//                                Install tab
//
// -----------------------------------------------------------------------------------------------

/*
** Constructor.
**
*/
DialogInstall::TabInstall::TabInstall(HWND wnd) : Tab(GetInstanceHandle(), wnd, IDD_INSTALL_TAB, DlgProc)
{
}

void DialogInstall::TabInstall::Initialize()
{
	HWND item = GetDlgItem(m_Window, IDC_INSTALLTAB_COMPONENTS_LIST);

	DWORD extendedFlags =
		LVS_EX_CHECKBOXES |
		LVS_EX_LABELTIP |
		LVS_EX_FULLROWSELECT |
		LVS_EX_DOUBLEBUFFER;
	SetWindowTheme(item, L"explorer", nullptr);
	ListView_EnableGroupView(item, TRUE);
	ListView_SetExtendedListViewStyleEx(item, 0, extendedFlags);

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
	lvg.state = LVGS_COLLAPSIBLE;

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
			bool disablePlugin = false;
			if (_waccess(itemPath.c_str(), 0) != -1)
			{
				if (groupId == 3)
				{
					int isNewer = c_Dialog->IsPluginNewer(*iter, itemPath);
					if (isNewer >= 0)
					{
						disablePlugin = true;
						text = isNewer > 0 ? L"Newer version installed" : L"Versions are the same";
					}
					else
					{
						text = L"Replace";
					}
				}
				else
				{
					bool backup = groupId == 0 && c_Dialog->m_BackupSkins && !c_Dialog->m_BackupPackage;
					text = backup ? L"Backup and replace" : L"Replace";
				}
			}
			ListView_SetItemText(item, lvi.iItem, 1, text);

			if (disablePlugin)
			{
				ListView_SetCheckState(item, lvi.iItem, FALSE);
			}

			++lvi.iItem;
		}
	};

	addComponent(L"Skins", c_Dialog->m_PackageSkins, g_Data.skinsPath, 0);
	addComponent(L"Layouts", c_Dialog->m_PackageLayouts, g_Data.settingsPath + L"Layouts\\", 1);
	addComponent(L"Addons", c_Dialog->m_PackageAddons, g_Data.settingsPath + L"Addons\\", 2);
	addComponent(L"Plugins", c_Dialog->m_PackagePlugins, g_Data.settingsPath + L"Plugins\\", 3);

	item = GetDlgItem(m_Window, IDC_INSTALLTAB_THEME_CHECKBOX);
	if (!c_Dialog->m_LoadLayout.empty())
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

INT_PTR CALLBACK DialogInstall::TabInstall::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
		return OnNotify(wParam, lParam);
	}

	return FALSE;
}

INT_PTR DialogInstall::TabInstall::OnNotify(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_INSTALLTAB_COMPONENTS_LIST:
		{
			LPNMLISTVIEW pNMLV = (LPNMLISTVIEW)lParam;
			if (pNMLV->uChanged == LVIF_STATE && (pNMLV->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(2))
			{
				HWND hwnd = pNMLV->hdr.hwndFrom;

				// Get needed information from item
				WCHAR text[80];
				ListView_GetItemText(hwnd, pNMLV->iItem, 1, text, 80);
				BOOL checked = ListView_GetCheckState(hwnd, pNMLV->iItem);

				// Make sure we only display a message box if the plugin is older than the installed version
				if (!checked && wcscmp(L"Newer version installed", text) == 0)
				{
					const WCHAR* message = L"There is already a newer version of this plugin installed " \
						L"on your computer. Installing an older plugin is not recommended.\n\n" \
						L"Proceed with caution.";
					MessageBox(hwnd, message, L"Rainmeter Skin Installer", MB_OK | MB_ICONEXCLAMATION);
				}

				return TRUE;
			}
		}
	}

	return FALSE;
}
