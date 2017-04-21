/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "DialogPackage.h"
#include "../Common/StringUtil.h"
#include "SkinInstaller.h"
#include "DialogInstall.h"
#include "resource.h"
#include "../Version.h"

#define WM_DELAYED_CLOSE WM_APP + 0

extern GlobalData g_Data;
extern OsNameVersion g_OsNameVersions[];

DialogPackage* DialogPackage::c_Dialog = nullptr;

DialogPackage::DialogPackage(HWND wnd) : OldDialog(wnd),
	m_TabInfo(wnd),
	m_TabOptions(wnd),
	m_TabAdvanced(wnd),
	m_LoadLayout(false),
	m_MergeSkins(false),
	m_PackagerThread(),
	m_ZipFile(),
	m_AllowNonAsciiFilenames(false)
{
}

DialogPackage::~DialogPackage()
{
}

void DialogPackage::Create(HINSTANCE hInstance, LPWSTR lpCmdLine)
{
	HANDLE hMutex;
	if (IsRunning(L"Rainmeter Skin Packager", &hMutex))
	{
		HWND hwnd = FindWindow(L"#32770", L"Rainmeter Skin Packager");
		SetForegroundWindow(hwnd);
	}
	else
	{
		DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PACKAGE_DIALOG), nullptr, (DLGPROC)DlgProc, (LPARAM)lpCmdLine);
		ReleaseMutex(hMutex);
	}
}

OldDialog::Tab& DialogPackage::GetActiveTab()
{
	int sel = TabCtrl_GetCurSel(GetDlgItem(m_Window, IDC_PACKAGE_TAB));
	if (sel == -1)
	{
		return m_TabInfo;
	}
	else if (sel == 0)
	{
		return m_TabOptions;
	}
	else // if (sel == 1)
	{
		return m_TabAdvanced;
	}
}

INT_PTR CALLBACK DialogPackage::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!c_Dialog)
	{
		if (uMsg == WM_INITDIALOG)
		{
			c_Dialog = new DialogPackage(hWnd);
			return c_Dialog->OnInitDialog(wParam, lParam);
		}
	}
	else
	{
		switch (uMsg)
		{
		case WM_COMMAND:
			return c_Dialog->OnCommand(wParam, lParam);

		case WM_NOTIFY:
			return c_Dialog->OnNotify(wParam, lParam);

		case WM_CLOSE:
			{
				if (!c_Dialog->m_PackagerThread)
				{
					EndDialog(hWnd, 0);
				}
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

INT_PTR DialogPackage::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	HICON hIcon = (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_SKININSTALLER), IMAGE_ICON, 16, 16, LR_SHARED);
	SendMessage(m_Window, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	SetDialogFont();

	m_TabInfo.Activate();

	return FALSE;
}

INT_PTR DialogPackage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_PACKAGE_NEXT_BUTTON:
		{
			HWND item = GetDlgItem(m_Window, IDC_PACKAGE_TAB);
			TCITEM tci = {0};
			tci.mask = TCIF_TEXT;
			tci.pszText = L"Options";
			TabCtrl_InsertItem(item, 0, &tci);
			tci.pszText = L"Advanced";
			TabCtrl_InsertItem(item, 1, &tci);

			item = GetDlgItem(m_Window, IDC_PACKAGE_NEXT_BUTTON);
			ShowWindow(item, SW_HIDE);

			item = GetDlgItem(m_Window, IDC_PACKAGE_CREATEPACKAGE_BUTTON);
			ShowWindow(item, SW_SHOWNORMAL);
			SendMessage(m_Window, DM_SETDEFID, IDC_PACKAGE_CREATEPACKAGE_BUTTON, 0);

			ShowWindow(m_TabInfo.GetWindow(), SW_HIDE);

			m_TabOptions.Activate();
		}
		break;

	case IDC_PACKAGE_CREATEPACKAGE_BUTTON:
		{
			HWND item = GetDlgItem(m_Window, IDC_PACKAGE_CREATEPACKAGE_BUTTON);
			EnableWindow(item, FALSE);

			item = GetDlgItem(m_Window, IDCANCEL);
			EnableWindow(item, FALSE);

			m_TabOptions.Activate();
			item = GetDlgItem(m_Window, IDC_PACKAGE_TAB);
			TabCtrl_SetCurSel(item, 0);
			EnableWindow(item, FALSE);
			EnableWindow(m_TabOptions.GetWindow(), FALSE);
			EnableWindow(m_TabAdvanced.GetWindow(), FALSE);

			item = GetDlgItem(m_TabOptions.GetWindow(), IDC_INSTALLTAB_CREATING_TEXT);
			ShowWindow(item, SW_SHOWNORMAL);

			item = GetDlgItem(m_TabOptions.GetWindow(), IDC_INSTALLTAB_CREATING_BAR);
			ShowWindow(item, SW_SHOWNORMAL);
			SendMessage(item, PBM_SETMARQUEE, (WPARAM)TRUE, 0);

			m_PackagerThread = (HANDLE)_beginthreadex(nullptr, 0, PackagerThreadProc, this, 0, nullptr);
			if (!m_PackagerThread)
			{
				MessageBox(m_Window, L"Unknown error.", L"Rainmeter Skin Packager", MB_ERROR);
				EndDialog(m_Window, 0);
			}
		}
		break;

	case IDCANCEL:
		if (!m_PackagerThread)
		{
			EndDialog(m_Window, 0);
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

INT_PTR DialogPackage::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR nm = (LPNMHDR)lParam;
	switch (nm->idFrom)
	{
	case IDC_PACKAGE_TAB:
		if (nm->code == TCN_SELCHANGE)
		{
			// Disable all tab windows first
			EnableWindow(m_TabInfo.GetWindow(), FALSE);
			EnableWindow(m_TabOptions.GetWindow(), FALSE);
			EnableWindow(m_TabAdvanced.GetWindow(), FALSE);

			GetActiveTab().Activate();
		}
		break;

	default:
		return 1;
	}

	return 0;
}

void DialogPackage::SetNextButtonState()
{
	BOOL state = !(m_Name.empty() || m_Author.empty() || m_SkinFolder.second.empty());
	EnableWindow(GetDlgItem(m_Window, IDC_PACKAGE_NEXT_BUTTON), state);
}

bool DialogPackage::CreatePackage()
{
	// Create options file
	WCHAR tempFile[MAX_PATH];
	GetTempPath(MAX_PATH, tempFile);
	GetTempFileName(tempFile, L"ini", 0, tempFile);

	WritePrivateProfileString(L"rmskin", L"Name", m_Name.c_str(), tempFile);
	WritePrivateProfileString(L"rmskin", L"Author", m_Author.c_str(), tempFile);
	WritePrivateProfileString(L"rmskin", L"Version", m_Version.c_str(), tempFile);

	if (!c_Dialog->m_Load.empty())
	{
		WritePrivateProfileString(L"rmskin", L"LoadType", c_Dialog->m_LoadLayout ? L"Layout" : L"Skin", tempFile);
		WritePrivateProfileString(L"rmskin", L"Load", c_Dialog->m_Load.c_str(), tempFile);
	}

	if (!c_Dialog->m_VariableFiles.empty())
	{
		WritePrivateProfileString(L"rmskin", L"VariableFiles", m_VariableFiles.c_str(), tempFile);
	}

	if (c_Dialog->m_MergeSkins)
	{
		WritePrivateProfileString(L"rmskin", L"MergeSkins", L"1", tempFile);
	}

	WritePrivateProfileString(L"rmskin", L"MinimumRainmeter", m_MinimumRainmeter.c_str(), tempFile);
	WritePrivateProfileString(L"rmskin", L"MinimumWindows", m_MinimumWindows.c_str(), tempFile);

	// Only Skin Installer in Rainmeter 3.0.1 support UTF-8 filenames.
	m_AllowNonAsciiFilenames = DialogInstall::CompareVersions(m_MinimumRainmeter, L"3.0.1") != -1;

	// Create archive and add options file and header bitmap
	m_ZipFile = zipOpen(StringUtil::Narrow(m_TargetFile.c_str()).c_str(), APPEND_STATUS_CREATE);

	auto cleanup = [&]()->bool
	{
		zipClose(m_ZipFile, nullptr);
		DeleteFile(m_TargetFile.c_str());
		return false;
	};

	if (!m_ZipFile ||
		(!c_Dialog->m_HeaderFile.empty() && !AddFileToPackage(c_Dialog->m_HeaderFile.c_str(), L"RMSKIN.bmp")) ||
		!AddFileToPackage(tempFile, L"RMSKIN.ini"))
	{
		std::wstring error = L"Unable to create package.";
		error += L"\n\nClick OK to close Packager.";
		MessageBox(m_Window, error.c_str(), L"Rainmeter Skin Packager", MB_OK | MB_ICONERROR);
		DeleteFile(tempFile);
		return cleanup();
	}

	// Add skin
	{
		std::wstring zipPrefix = L"Skins\\" + m_SkinFolder.first;
		zipPrefix += L'\\';
		if (!AddFolderToPackage(m_SkinFolder.second, L"", zipPrefix.c_str()))
		{
			return cleanup();
		}
	}

	// Add layouts
	for (auto iter = m_LayoutFolders.cbegin(); iter != m_LayoutFolders.cend(); ++iter)
	{
		std::wstring realPath = (*iter).second + L"Rainmeter.ini";
		std::wstring zipPath = L"Layouts\\" + (*iter).first;
		zipPath += L"\\Rainmeter.ini";
		if (!AddFileToPackage(realPath.c_str(), zipPath.c_str()))
		{
			std::wstring error = L"Error adding layout '";
			error += (*iter).first;
			error += L"'.";
			error += L"\n\nClick OK to close Packager.";
			MessageBox(m_Window, error.c_str(), L"Rainmeter Skin Packager", MB_OK | MB_ICONERROR);
			return cleanup();
		}
	}

	// Add plugins
	for (auto iter = m_PluginFolders.cbegin(); iter != m_PluginFolders.cend(); ++iter)
	{
		// Add 32bit and 64bit versions
		for (int i = 0; i < 2; ++i)
		{
			const std::wstring& realPath = (i == 0) ? (*iter).second.first : (*iter).second.second;
			std::wstring zipPath = ((i == 0) ? L"Plugins\\32bit\\" : L"Plugins\\64bit\\") + (*iter).first;
			if (!AddFileToPackage(realPath.c_str(), zipPath.c_str()))
			{
				std::wstring error = L"Error adding plugin '";
				error += (*iter).first;
				error += L"'.";
				error += L"\n\nClick OK to close Packager.";
				MessageBox(m_Window, error.c_str(), L"Rainmeter Skin Packager", MB_OK | MB_ICONERROR);
				return cleanup();
			}
		}
	}

	// Add footer
	FILE* file;
	if (zipClose(m_ZipFile, nullptr) == ZIP_OK &&
		(file = _wfopen(m_TargetFile.c_str(), L"r+b")) != nullptr)
	{
		fseek(file, 0, SEEK_END);
		DialogInstall::PackageFooter footer = { _ftelli64(file), 0, "RMSKIN" };
		fwrite(&footer, sizeof(footer), 1, file);
		fclose(file);
	}
	else
	{
		std::wstring error = L"Unable to create package.";
		error += L"\n\nClick OK to close Packager.";
		MessageBox(m_Window, error.c_str(), L"Rainmeter Skin Packager", MB_OK | MB_ICONERROR);
		return false;
	}

	return true;
}

unsigned __stdcall DialogPackage::PackagerThreadProc(void* pParam)
{
	DialogPackage* dialog = (DialogPackage*)pParam;

	if (dialog->CreatePackage())
	{
		// Stop the progress bar
		HWND item = GetDlgItem(dialog->m_TabOptions.GetWindow(), IDC_INSTALLTAB_CREATING_BAR);
		SendMessage(item, PBM_SETMARQUEE, (WPARAM)FALSE, 0);

		FlashWindow(dialog->m_Window, TRUE);

		std::wstring message = L"The skin package has been successfully created.";
		message += L"\n\nClick OK to close Packager.";
		MessageBox(c_Dialog->GetWindow(), message.c_str(), L"Rainmeter Skin Packager", MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		DeleteFile(dialog->m_TargetFile.c_str());
	}

	EndDialog(dialog->m_Window, 0);

	return 0;
}

bool DialogPackage::AddFileToPackage(const WCHAR* filePath, const WCHAR* zipPath)
{
	std::string zipPathUTF8 = StringUtil::NarrowUTF8(zipPath);
	for (size_t i = 0, isize = zipPathUTF8.length(); i < isize; ++i)
	{
		if ((zipPathUTF8[i] & 0x80) != 0)
		{
			// UTF-8 lead bit is not zero so the string is non-ASCII.
			if (!m_AllowNonAsciiFilenames)
			{
				return false;
			}
		}

		if (zipPathUTF8[i] == '\\')
		{
			zipPathUTF8[i] = '/';
		}
	}

	const uLong ZIP_UTF8_FLAG = 1 << 11;
	zip_fileinfo fi = {0};
	if (zipOpenNewFileInZip4(
		m_ZipFile, zipPathUTF8.c_str(), &fi,
		nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION,
		0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0, 0, ZIP_UTF8_FLAG) != ZIP_OK)
	{
		return false;
	}

	bool result = true;

	if (filePath)
	{
		HANDLE file = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,  FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
		{
			result = false;
		}
		else
		{
			do
			{
				const DWORD bufferSize = 16 * 1024;
				BYTE buffer[bufferSize];
				DWORD readSize;
				if (!ReadFile(file, buffer, bufferSize, &readSize, nullptr))
				{
					result = false;
				}
				else if (readSize != 0)
				{
					result = zipWriteInFileInZip(m_ZipFile, buffer, (UINT)readSize) == ZIP_OK;
				}
				else
				{
					// EOF
					break;
				}
			}
			while (result);

			CloseHandle(file);
		}
	}
	else
	{
		// Directory entry, so nothing needs to be written.
	}

	return zipCloseFileInZip(m_ZipFile) == ZIP_OK && result;
}

bool DialogPackage::AddFolderToPackage(const std::wstring& path, std::wstring base, const WCHAR* zipPrefix)
{
	std::wstring currentPath = path + base;
	currentPath += L'*';

	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFileEx(
		currentPath.c_str(),
		FindExInfoBasic,
		&fd,
		FindExSearchNameMatch,
		nullptr,
		0);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	currentPath.pop_back();	// Remove *

	bool result = true;
	bool filesAdded = false;
	std::list<std::wstring> folders;
	do
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
		{
			// Ignore hidden files and folders
			continue;
		}

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (!(fd.cFileName[0] == L'.' && (!fd.cFileName[1] || fd.cFileName[1] == L'.' && !fd.cFileName[2])))
			{
				folders.push_back(fd.cFileName);
			}
		}
		else
		{
			std::wstring filePath = currentPath + fd.cFileName;
			std::wstring zipPath = zipPrefix;
			zipPath.append(filePath, path.length(), filePath.length() - path.length());

			result = AddFileToPackage(filePath.c_str(), zipPath.c_str());
			if (!result)
			{
				std::wstring error = L"Error adding file:\n";
				error += filePath;
				error += L"\n\nClick OK to close Packager.";
				MessageBox(m_Window, error.c_str(), L"Rainmeter Skin Packager", MB_OK | MB_ICONERROR);
				break;
			}

			filesAdded = true;
		}
	}
	while (FindNextFile(hFind, &fd));
	FindClose(hFind);

	if (result)
	{
		if (!filesAdded && folders.empty())
		{
			// Add directory entry if folder is empty.
			std::wstring zipPath = zipPrefix;
			zipPath.append(currentPath, path.length(), currentPath.length() - path.length());
			AddFileToPackage(nullptr, zipPath.c_str());
		}

		std::list<std::wstring>::const_iterator iter = folders.begin();
		for ( ; iter != folders.end(); ++iter)
		{
			std::wstring newBase = base + (*iter);
			newBase += L'\\';
			result = AddFolderToPackage(path, newBase, zipPrefix);
			if (!result) break;
		}
	}

	return result;
}

void DialogPackage::ShowHelp()
{
	const WCHAR* url = revision_beta ?
		L"https://docs.rainmeter.net/manual-beta/publishing-skins/" :
		L"https://docs.rainmeter.net/manual/publishing-skins/";
	ShellExecute(m_Window, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

std::wstring DialogPackage::SelectFolder(HWND parent, const std::wstring& existingPath)
{
	LPCWSTR dialog = MAKEINTRESOURCE(IDD_PACKAGESELECTFOLDER_DIALOG);
	std::wstring folder = existingPath;
	if (DialogBoxParam(GetInstanceHandle(), dialog, parent, SelectFolderDlgProc, (LPARAM)&folder) != 1)
	{
		folder.clear();
	}
	return folder;
}

INT_PTR CALLBACK DialogPackage::SelectFolderDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			EnableThemeDialogTexture(hWnd, ETDT_ENABLETAB);
			c_Dialog->SetDialogFont(hWnd);

			std::wstring* existingPath = (std::wstring*)lParam;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);

			*existingPath += L'*';
			WIN32_FIND_DATA fd;
			HANDLE hFind = FindFirstFileEx(existingPath->c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, 0);
			existingPath->pop_back();

			if (hFind != INVALID_HANDLE_VALUE)
			{
				const WCHAR* folder = PathFindFileName(existingPath->c_str());

				HWND item = GetDlgItem(hWnd, IDC_PACKAGESELECTFOLDER_EXISTING_RADIO);
				std::wstring text = L"Add folder from ";
				text.append(folder, wcslen(folder) - 1);
				text += L':';
				SetWindowText(item, text.c_str());
				Button_SetCheck(item, BST_CHECKED);

				item = GetDlgItem(hWnd, IDC_PACKAGESELECTFOLDER_EXISTING_COMBO);

				do
				{
					if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
						!(fd.cFileName[0] == L'.' && (!fd.cFileName[1] || fd.cFileName[1] == L'.' && !fd.cFileName[2])) &&
						wcscmp(fd.cFileName, L"Backup") != 0 &&
						wcscmp(fd.cFileName, L"@Backup") != 0)
					{
						ComboBox_InsertString(item, -1, fd.cFileName);
					}
				}
				while (FindNextFile(hFind, &fd));

				ComboBox_SetCurSel(item, 0);

				FindClose(hFind);
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_PACKAGESELECTFOLDER_EXISTING_RADIO:
			{
				HWND item = GetDlgItem(hWnd, IDC_PACKAGESELECTFOLDER_EXISTING_COMBO);
				EnableWindow(item, TRUE);
				int sel = ComboBox_GetCurSel(item);
				item = GetDlgItem(hWnd, IDC_PACKAGESELECTFOLDER_CUSTOM_EDIT);
				EnableWindow(item, FALSE);
				item = GetDlgItem(hWnd, IDC_PACKAGESELECTFOLDER_CUSTOMBROWSE_BUTTON);
				EnableWindow(item, FALSE);

				item = GetDlgItem(hWnd, IDOK);
				EnableWindow(item, sel != -1);
			}
			break;

		case IDC_PACKAGESELECTFOLDER_CUSTOM_RADIO:
			{
				HWND item = GetDlgItem(hWnd, IDC_PACKAGESELECTFOLDER_EXISTING_COMBO);
				EnableWindow(item, FALSE);
				item = GetDlgItem(hWnd, IDC_PACKAGESELECTFOLDER_CUSTOM_EDIT);
				EnableWindow(item, TRUE);
				item = GetDlgItem(hWnd, IDC_PACKAGESELECTFOLDER_CUSTOMBROWSE_BUTTON);
				EnableWindow(item, TRUE);

				SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_PACKAGESELECTFOLDER_CUSTOM_EDIT, EN_CHANGE), 0);
			}
			break;

		case IDC_PACKAGESELECTFOLDER_CUSTOM_EDIT:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				WCHAR buffer[MAX_PATH];
				int len = Edit_GetText((HWND)lParam, buffer, MAX_PATH);

				// Disable Add button if invalid directory
				DWORD attributes = GetFileAttributes(buffer);
				BOOL state = (attributes != INVALID_FILE_ATTRIBUTES &&
					attributes & FILE_ATTRIBUTE_DIRECTORY);
				EnableWindow(GetDlgItem(hWnd, IDOK), state);
			}
			break;

		case IDC_PACKAGESELECTFOLDER_CUSTOMBROWSE_BUTTON:
			{
				WCHAR buffer[MAX_PATH];
				BROWSEINFO bi = {0};
				bi.hwndOwner = hWnd;
				bi.ulFlags = BIF_USENEWUI | BIF_NONEWFOLDERBUTTON | BIF_RETURNONLYFSDIRS;

				PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&bi);
				if (pidl && SHGetPathFromIDList(pidl, buffer))
				{
					HWND item = GetDlgItem(hWnd, IDC_PACKAGESELECTFOLDER_CUSTOM_EDIT);
					SetWindowText(item, buffer);
					CoTaskMemFree(pidl);
				}
			}
			break;

		case IDOK:
			{
				WCHAR buffer[MAX_PATH];
				HWND item = GetDlgItem(hWnd, IDC_PACKAGESELECTFOLDER_EXISTING_RADIO);
				bool existing = Button_GetCheck(item) == BST_CHECKED;

				item = GetDlgItem(hWnd, existing ? IDC_PACKAGESELECTFOLDER_EXISTING_COMBO : IDC_PACKAGESELECTFOLDER_CUSTOM_EDIT);
				GetWindowText(item, buffer, _countof(buffer));

				std::wstring* result = (std::wstring*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

				if (existing)
				{
					*result += buffer;
				}
				else
				{
					*result = buffer;
				}
				*result += L'\\';

				EndDialog(hWnd, 1);
			}
		}
		break;

	case WM_CLOSE:
		EndDialog(hWnd, 0);
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

std::pair<std::wstring, std::wstring> DialogPackage::SelectPlugin(HWND parent)
{
	LPCWSTR dialog = MAKEINTRESOURCE(IDD_PACKAGESELECTPLUGIN_DIALOG);
	std::pair<std::wstring, std::wstring> plugins;
	if (DialogBoxParam(GetInstanceHandle(), dialog, parent, SelectPluginDlgProc, (LPARAM)&plugins) != 1)
	{
		plugins.first.clear();
		plugins.second.clear();
	}
	return plugins;
}

INT_PTR CALLBACK DialogPackage::SelectPluginDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			EnableThemeDialogTexture(hWnd, ETDT_ENABLETAB);
			c_Dialog->SetDialogFont(hWnd);

			auto plugins = (std::pair<std::wstring, std::wstring>*)lParam;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)plugins);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_PACKAGESELECTPLUGIN_32BITBROWSE_BUTTON:
		case IDC_PACKAGESELECTPLUGIN_64BITBROWSE_BUTTON:
			{
				WCHAR buffer[MAX_PATH];
				buffer[0] = L'\0';

				OPENFILENAME ofn = { sizeof(OPENFILENAME) };
				ofn.Flags = OFN_FILEMUSTEXIST;
				ofn.lpstrFilter = L"Plugins (.dll)\0*.dll";
				ofn.lpstrTitle = L"Select plugin file";
				ofn.lpstrDefExt = L"dll";
				ofn.nFilterIndex = 0;
				ofn.lpstrFile = buffer;
				ofn.nMaxFile = _countof(buffer);
				ofn.hwndOwner = c_Dialog->GetWindow();

				if (!GetOpenFileName(&ofn))
				{
					break;
				}

				bool x32 = LOWORD(wParam) == IDC_PACKAGESELECTPLUGIN_32BITBROWSE_BUTTON;

				LOADED_IMAGE* loadedImage = ImageLoad(StringUtil::Narrow(buffer).c_str(), nullptr);
				if (loadedImage)
				{
					WORD machine = loadedImage->FileHeader->FileHeader.Machine;
					ImageUnload(loadedImage);

					if ((x32 && machine == IMAGE_FILE_MACHINE_I386) || (!x32 && machine == IMAGE_FILE_MACHINE_AMD64))
					{
						// Check if same name as other DLL
						auto plugins = (std::pair<std::wstring, std::wstring>*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
						const WCHAR* otherName = PathFindFileName(x32 ? plugins->second.c_str() : plugins->first.c_str());
						if (*otherName && _wcsicmp(otherName, PathFindFileName(buffer)) != 0)
						{
							MessageBox(hWnd, L"Plugins must have same name.", L"Rainmeter Skin Packager", MB_OK | MB_TOPMOST);
							break;
						}

						PathSetDlgItemPath(hWnd, x32 ? IDC_PACKAGESELECTPLUGIN_32BIT_EDIT : IDC_PACKAGESELECTPLUGIN_64BIT_EDIT, buffer);

						(x32 ? plugins->first : plugins->second) = buffer;

						if (!plugins->first.empty() && !plugins->second.empty())
						{
							// Enable Add button if both plugins have been selected
							EnableWindow(GetDlgItem(hWnd, IDOK), TRUE);
						}
						break;
					}
				}

				MessageBox(hWnd, L"Invalid plugin.", L"Rainmeter Skin Packager", MB_OK | MB_TOPMOST);
			}
			break;

		case IDOK:
			EndDialog(hWnd, 1);
			break;
		}
		break;

	case WM_CLOSE:
		EndDialog(hWnd, 0);
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------------
//
//                                Info tab
//
// -----------------------------------------------------------------------------------------------

DialogPackage::TabInfo::TabInfo(HWND wnd) : Tab(GetInstanceHandle(), wnd, IDD_PACKAGEINFO_TAB, DlgProc)
{
}

void DialogPackage::TabInfo::Initialize()
{
	m_Initialized = true;

	HWND item = GetDlgItem(m_Window, IDC_INSTALLTAB_NAME_TEXT);
	Edit_SetCueBannerText(item, L"...");

	item = GetDlgItem(m_Window, IDC_INSTALLTAB_AUTHOR_TEXT);
	Edit_SetCueBannerText(item, L"...");

	item = GetDlgItem(m_Window, IDC_INSTALLTAB_VERSION_TEXT);
	Edit_SetCueBannerText(item, L"...");

	item = GetDlgItem(m_Window, IDC_PACKAGEINFO_COMPONENTS_LIST);

	DWORD extendedFlags =
		LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
	SetWindowTheme(item, L"explorer", nullptr);
	ListView_EnableGroupView(item, TRUE);
	ListView_SetExtendedListViewStyleEx(item, 0, extendedFlags);

	// Add columns
	LVCOLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.iSubItem = 0;
	lvc.cx = 252;
	lvc.pszText = L"Name";
	ListView_InsertColumn(item, 0, &lvc);

	// Add groups
	LVGROUP lvg;
	lvg.cbSize = sizeof(LVGROUP);
	lvg.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
	lvg.state = LVGS_COLLAPSIBLE;
	lvg.iGroupId = 0;
	lvg.pszHeader = L"Skin";
	ListView_InsertGroup(item, -1, &lvg);
	lvg.iGroupId = 1;
	lvg.pszHeader = L"Layouts";
	ListView_InsertGroup(item, -1, &lvg);
	lvg.iGroupId = 2;
	lvg.pszHeader = L"Plugins";
	ListView_InsertGroup(item, -1, &lvg);
}

INT_PTR CALLBACK DialogPackage::TabInfo::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return c_Dialog->m_TabInfo.OnCommand(wParam, lParam);

	case WM_NOTIFY:
		return c_Dialog->m_TabInfo.OnNotify(wParam, lParam);
	}

	return FALSE;
}

INT_PTR DialogPackage::TabInfo::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_PACKAGEINFO_ADDSKIN_BUTTON:
		{
			c_Dialog->m_SkinFolder.second = SelectFolder(m_Window, g_Data.skinsPath);
			if (!c_Dialog->m_SkinFolder.second.empty())
			{
				c_Dialog->m_SkinFolder.first = PathFindFileName(c_Dialog->m_SkinFolder.second.c_str());
				c_Dialog->m_SkinFolder.first.pop_back();	// Remove slash

				HWND item = GetDlgItem(m_Window, IDC_PACKAGEINFO_COMPONENTS_LIST);
				LVITEM lvi;
				lvi.mask = LVIF_TEXT | LVIF_GROUPID;
				lvi.iItem = 1;
				lvi.iSubItem = 0;
				lvi.iGroupId = 0;
				lvi.pszText = (WCHAR*)c_Dialog->m_SkinFolder.first.c_str();
				ListView_InsertItem(item, &lvi);

				EnableWindow((HWND)lParam, FALSE);
				c_Dialog->SetNextButtonState();
			}
		}
		break;

	case IDC_PACKAGEINFO_ADDTHEME_BUTTON:
		{
			std::wstring folder = SelectFolder(m_Window, g_Data.settingsPath + L"Layouts\\");
			if (!folder.empty())
			{
				std::wstring name = PathFindFileName(folder.c_str());
				name.pop_back();	// Remove slash

				if (c_Dialog->m_LayoutFolders.insert(std::make_pair(name, folder)).second)
				{
					HWND item = GetDlgItem(m_Window, IDC_PACKAGEINFO_COMPONENTS_LIST);
					LVITEM lvi;
					lvi.mask = LVIF_TEXT | LVIF_GROUPID;
					lvi.iItem = (int)c_Dialog->m_LayoutFolders.size() + 1;
					lvi.iSubItem = 0;
					lvi.iGroupId = 1;
					lvi.pszText = (WCHAR*)name.c_str();
					ListView_InsertItem(item, &lvi);
				}
			}
		}
		break;

	case IDC_PACKAGEINFO_ADDPLUGIN_BUTTON:
		{
			std::pair<std::wstring, std::wstring> plugins = SelectPlugin(m_Window);
			std::wstring name = PathFindFileName(plugins.first.c_str());
			if (!name.empty() && c_Dialog->m_PluginFolders.insert(std::make_pair(name, plugins)).second)
			{
				HWND item = GetDlgItem(m_Window, IDC_PACKAGEINFO_COMPONENTS_LIST);
				LVITEM lvi;
				lvi.mask = LVIF_TEXT | LVIF_GROUPID;
				lvi.iItem = (int)c_Dialog->m_PluginFolders.size() + 1;
				lvi.iSubItem = 0;
				lvi.iGroupId = 2;
				lvi.pszText = (WCHAR*)name.c_str();
				ListView_InsertItem(item, &lvi);
			}
		}
		break;

	case IDC_PACKAGEINFO_REMOVE_BUTTON:
		{
			HWND item = GetDlgItem(m_Window, IDC_PACKAGEINFO_COMPONENTS_LIST);
			int sel = ListView_GetNextItem(item, -1, LVNI_FOCUSED | LVNI_SELECTED);
			if (sel != -1)
			{
				WCHAR buffer[MAX_PATH];

				// Remove unchecked items from the component sets
				LVITEM lvi;
				lvi.mask = LVIF_GROUPID | LVIF_TEXT;
				lvi.iSubItem = 0;
				lvi.iItem = sel;
				lvi.pszText = buffer;
				lvi.cchTextMax = _countof(buffer);
				ListView_GetItem(item, &lvi);

				ListView_DeleteItem(item, sel);

				const std::wstring name = buffer;
				switch (lvi.iGroupId)
				{
				case 0:
					{
						item = GetDlgItem(m_Window, IDC_PACKAGEINFO_ADDSKIN_BUTTON);
						EnableWindow(item, TRUE);
						c_Dialog->m_SkinFolder.first.clear();
						c_Dialog->m_SkinFolder.second.clear();
						c_Dialog->SetNextButtonState();
					}
					break;

				case 1:
					c_Dialog->m_LayoutFolders.erase(c_Dialog->m_LayoutFolders.find(name));
					break;

				case 2:
					c_Dialog->m_PluginFolders.erase(c_Dialog->m_PluginFolders.find(name));
					break;
				}
			}
		}
		break;

	case IDC_PACKAGEINFO_NAME_EDIT:
	case IDC_PACKAGEINFO_AUTHOR_EDIT:
	case IDC_PACKAGEINFO_VERSION_EDIT:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			WCHAR buffer[64];
			int len = GetWindowText((HWND)lParam, buffer, _countof(buffer));
			if (LOWORD(wParam) == IDC_PACKAGEINFO_NAME_EDIT)
			{
				c_Dialog->m_Name.assign(buffer, len);
			}
			else if (LOWORD(wParam) == IDC_PACKAGEINFO_AUTHOR_EDIT)
			{
				c_Dialog->m_Author.assign(buffer, len);
			}
			else // if (LOWORD(wParam) == IDC_PACKAGEINFO_VERSION_EDIT)
			{
				c_Dialog->m_Version.assign(buffer, len);
			}
			c_Dialog->SetNextButtonState();
		}
		break;


	default:
		return FALSE;
	}

	return TRUE;
}

INT_PTR DialogPackage::TabInfo::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR nm = (LPNMHDR)lParam;
	switch (nm->code)
	{
	case LVN_ITEMCHANGED:
		{
			NMLISTVIEW* nmlv = (NMLISTVIEW*)lParam;
			if (nm->idFrom == IDC_PACKAGEINFO_COMPONENTS_LIST)
			{
				BOOL selected = (nmlv->uNewState & LVIS_SELECTED);

				HWND item = GetDlgItem(m_Window, IDC_PACKAGEINFO_REMOVE_BUTTON);
				EnableWindow(item, selected);
			}
		}
		break;

	case NM_CLICK:
		{
			if (nm->idFrom == IDC_PACKAGEINFO_WHATIS_LINK)
			{
				c_Dialog->ShowHelp();
			}
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------------
//
//                                Options tab
//
// -----------------------------------------------------------------------------------------------

DialogPackage::TabOptions::TabOptions(HWND wnd) : Tab(GetInstanceHandle(), wnd, IDD_PACKAGEOPTIONS_TAB, DlgProc)
{
}

void DialogPackage::TabOptions::Initialize()
{
	m_Initialized = true;

	WCHAR buffer[MAX_PATH];
	SHGetFolderPath(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, SHGFP_TYPE_CURRENT, buffer);

	c_Dialog->m_TargetFile = buffer;
	c_Dialog->m_TargetFile += L'\\';
	int pos = (int)c_Dialog->m_TargetFile.length() + 1;
	c_Dialog->m_TargetFile += c_Dialog->m_Name;
	c_Dialog->m_TargetFile += L'_';
	c_Dialog->m_TargetFile += c_Dialog->m_Version;

	// Escape reserved chars
	for (int i = pos, isize = (int)c_Dialog->m_TargetFile.length(); i < isize; ++i)
	{
		if (wcschr(L"\\/:*?\"<>|", c_Dialog->m_TargetFile[i]))
		{
			c_Dialog->m_TargetFile[i] = L'_';
		}
	}

	c_Dialog->m_TargetFile += L".rmskin";

	HWND item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_FILE_EDIT);
	SetWindowText(item,c_Dialog->m_TargetFile.c_str());

	item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_LOADTHEME_RADIO);
	if (c_Dialog->m_LayoutFolders.empty())
	{
		EnableWindow(item, FALSE);

		item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_DONOTHING_RADIO);
		Button_SetCheck(item, BST_CHECKED);
	}
	else
	{
		c_Dialog->m_LoadLayout = true;
		c_Dialog->m_Load = (*c_Dialog->m_LayoutFolders.cbegin()).first;

		Button_SetCheck(item, BST_CHECKED);

		item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_LOADTHEME_COMBO);
		ShowWindow(item, SW_SHOWNORMAL);

		for (auto iter = c_Dialog->m_LayoutFolders.cbegin(); iter != c_Dialog->m_LayoutFolders.cend(); ++iter)
		{
			ComboBox_AddString(item, (*iter).first.c_str());
		}
		ComboBox_SetCurSel(item, 0);
	}

	item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_LOADSKIN_EDIT);
	Edit_SetCueBannerText(item, L"Select skin");

	item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_RAINMETERVERSION_EDIT);
	_snwprintf_s(buffer, _TRUNCATE, L"%s.%i", APPVERSION, revision_number);
	SetWindowText(item, buffer);
	c_Dialog->m_MinimumRainmeter = buffer;

	item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_WINDOWSVERSION_COMBO);
	ComboBox_AddString(item, L"XP");
	ComboBox_AddString(item, L"Vista");
	ComboBox_AddString(item, L"7");
	ComboBox_AddString(item, L"8");	
	ComboBox_AddString(item, L"10");	
	ComboBox_SetCurSel(item, 2);
	c_Dialog->m_MinimumWindows = g_OsNameVersions[0].version;
}

INT_PTR CALLBACK DialogPackage::TabOptions::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return c_Dialog->m_TabOptions.OnCommand(wParam, lParam);
	}

	return FALSE;
}

INT_PTR DialogPackage::TabOptions::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_PACKAGEOPTIONS_FILEBROWSE_BUTTON:
		{
			WCHAR buffer[MAX_PATH];
			HWND item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_FILE_EDIT);
			GetWindowText(item, buffer, _countof(buffer));

			OPENFILENAME ofn = { sizeof(OPENFILENAME) };
			ofn.lpstrFilter = L"Rainmeter skin package (.rmskin)\0*.rmskin";
			ofn.lpstrTitle = L"Select Rainmeter skin package";
			ofn.lpstrDefExt = L"dll";
			ofn.lpstrFile = buffer;
			ofn.nMaxFile = _countof(buffer);
			ofn.hwndOwner = c_Dialog->GetWindow();

			if (GetOpenFileName(&ofn))
			{
				c_Dialog->m_TargetFile = buffer;
				SetWindowText(item, buffer);
			}
		}
		break;

	case IDC_PACKAGEOPTIONS_DONOTHING_RADIO:
		{
			HWND item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_LOADSKIN_EDIT);
			ShowWindow(item, SW_HIDE);
			item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_LOADSKINBROWSE_BUTTON);
			ShowWindow(item, SW_HIDE);
			item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_LOADTHEME_COMBO);
			ShowWindow(item, SW_HIDE);

			c_Dialog->m_Load.clear();
		}
		break;

	case IDC_PACKAGEOPTIONS_LOADSKIN_RADIO:
		{
			HWND item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_LOADSKIN_EDIT);
			ShowWindow(item, SW_SHOWNORMAL);

			WCHAR buffer[MAX_PATH];
			GetWindowText(item, buffer, _countof(buffer));
			c_Dialog->m_Load = buffer;
			c_Dialog->m_LoadLayout = false;

			item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_LOADSKINBROWSE_BUTTON);
			ShowWindow(item, SW_SHOWNORMAL);
			item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_LOADTHEME_COMBO);
			ShowWindow(item, SW_HIDE);
		}
		break;

	case IDC_PACKAGEOPTIONS_LOADTHEME_RADIO:
		{
			HWND item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_LOADSKIN_EDIT);
			ShowWindow(item, SW_HIDE);
			item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_LOADSKINBROWSE_BUTTON);
			ShowWindow(item, SW_HIDE);
			item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_LOADTHEME_COMBO);
			ShowWindow(item, SW_SHOWNORMAL);

			WCHAR buffer[MAX_PATH];
			GetWindowText(item, buffer, _countof(buffer));
			c_Dialog->m_Load = buffer;
			c_Dialog->m_LoadLayout = true;
		}
		break;

	case IDC_PACKAGEOPTIONS_LOADSKINBROWSE_BUTTON:
		{
			WCHAR buffer[MAX_PATH];
			HWND item = GetDlgItem(m_Window, IDC_PACKAGEOPTIONS_LOADSKIN_EDIT);
			GetWindowText(item, buffer, _countof(buffer));

			OPENFILENAME ofn = { sizeof(OPENFILENAME) };
			ofn.Flags = OFN_FILEMUSTEXIST;
			ofn.FlagsEx = OFN_EX_NOPLACESBAR;
			ofn.lpstrFilter = L"Rainmeter skin file (.ini)\0*.ini";
			ofn.lpstrTitle = L"Select Rainmeter skin file";
			ofn.lpstrDefExt = L"ini";
			ofn.lpstrFile = buffer;
			ofn.nMaxFile = _countof(buffer);
			ofn.lpstrInitialDir = c_Dialog->m_SkinFolder.second.c_str();
			ofn.hwndOwner = c_Dialog->GetWindow();

			if (GetOpenFileName(&ofn))
			{
				// Make sure user didn't browse to some random folder
				if (_wcsnicmp(ofn.lpstrInitialDir, buffer, c_Dialog->m_SkinFolder.second.length()) == 0)
				{
					// Skip everything before actual skin folder
					const WCHAR* folderPath = buffer + c_Dialog->m_SkinFolder.second.length() - c_Dialog->m_SkinFolder.first.length() - 1;
					SetWindowText(item, folderPath);
					c_Dialog->m_Load = folderPath;
				}
			}
		}
		break;

	case IDC_PACKAGEOPTIONS_RAINMETERVERSION_EDIT:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			WCHAR buffer[32];
			GetWindowText((HWND)lParam, buffer, _countof(buffer));

			// Get caret position
			DWORD sel = Edit_GetSel((HWND)lParam);

			// Only allow numbers and period
			WCHAR* version = buffer;
			while (*version)
			{
				if (iswdigit(*version) || *version == L'.')
				{
					++version;
				}
				else
				{
					*version = L'\0';
					SetWindowText((HWND)lParam, buffer);

					// Reset caret position
					Edit_SetSel((HWND)lParam, LOWORD(sel), HIWORD(sel));
					break;
				}
			}

			c_Dialog->m_MinimumRainmeter = buffer;
		}
		break;

	case IDC_PACKAGEOPTIONS_WINDOWSVERSION_COMBO:
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			int sel = ComboBox_GetCurSel((HWND)lParam);
			c_Dialog->m_MinimumWindows = g_OsNameVersions[sel].version;
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

// -----------------------------------------------------------------------------------------------
//
//                                Advanced tab
//
// -----------------------------------------------------------------------------------------------

DialogPackage::TabAdvanced::TabAdvanced(HWND wnd) : Tab(GetInstanceHandle(), wnd, IDD_PACKAGEADVANCED_TAB, DlgProc)
{
}

void DialogPackage::TabAdvanced::Initialize()
{
	m_Initialized = true;
}

INT_PTR CALLBACK DialogPackage::TabAdvanced::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		return c_Dialog->m_TabAdvanced.OnCommand(wParam, lParam);

	case WM_NOTIFY:
		return c_Dialog->m_TabAdvanced.OnNotify(wParam, lParam);
	}

	return FALSE;
}

INT_PTR DialogPackage::TabAdvanced::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_PACKAGEADVANCED_HEADERROWSE_BUTTON:
		{
			WCHAR buffer[MAX_PATH];
			HWND item = GetDlgItem(m_Window, IDC_PACKAGEADVANCED_HEADER_EDIT);
			GetWindowText(item, buffer, _countof(buffer));

			OPENFILENAME ofn = { sizeof(OPENFILENAME) };
			ofn.Flags = OFN_FILEMUSTEXIST;
			ofn.lpstrFilter = L"Bitmap file (.bmp)\0*.bmp";
			ofn.lpstrTitle = L"Select header image";
			ofn.lpstrDefExt = L"bmp";
			ofn.lpstrFile = buffer;
			ofn.nMaxFile = _countof(buffer);
			ofn.hwndOwner = c_Dialog->GetWindow();

			if (GetOpenFileName(&ofn))
			{
				// Validate bitmap and make sure size is 400x60
				std::wstring error;
				HBITMAP bitmap = (HBITMAP)LoadImage(nullptr, buffer, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
				if (bitmap)
				{
					BITMAP bm = { 0 };
					GetObject(bitmap, sizeof(bm), &bm);
					if (bm.bmWidth == 400 && bm.bmHeight == 60)
					{
						c_Dialog->m_HeaderFile = buffer;
						SetWindowText(item, buffer);
						break;
					}
					else
					{
						error = L"Error: Invalid size\n\"";
						error += buffer;
						error += L"\" must be exactly 400x60.";
					}
				}
				else
				{
					error = L"Error: Invalid .bmp file\n\"";
					error += buffer;
					error += L"\"";
				}

				MessageBox(m_Window, error.c_str(), L"Rainmeter Skin Packager", MB_OK | MB_ICONERROR);
			}
		}
		break;

	case IDC_PACKAGEADVANCED_VARIABLEFILES_EDIT:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			int length = GetWindowTextLength((HWND)lParam);
			c_Dialog->m_VariableFiles.resize(length);
			GetWindowText((HWND)lParam, &c_Dialog->m_VariableFiles[0], length + 1);
		}
		break;

	case IDC_PACKAGEADVANCED_MERGESKINS_CHECK:
		c_Dialog->m_MergeSkins = !c_Dialog->m_MergeSkins;
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

INT_PTR DialogPackage::TabAdvanced::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR nm = (LPNMHDR)lParam;
	switch (nm->code)
	{
	case NM_CLICK:
		{
			if (nm->idFrom == IDC_PACKAGEADVANCED_HELP_LINK)
			{
				c_Dialog->ShowHelp();
			}
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}
