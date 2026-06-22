/* Copyright (C) 2012 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "DialogPackage.h"
#include "SkinInstaller.h"
#include "DialogInstall.h"
#include "resource.h"
#include "Util.h"
#include "../Common/FileUtil.h"
#include "../Common/StringUtil.h"
#include "../Version.h"

#include "iowin32.h"

#define WM_DELAYED_CLOSE WM_APP + 0

extern GlobalData g_Data;
extern OsNameVersion g_OsNameVersions[];

DialogPackage* DialogPackage::c_Dialog = nullptr;

DialogPackage::DialogPackage() : Dialog(),
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
	(void)hInstance;
	(void)lpCmdLine;

	HANDLE hMutex;
	if (IsRunning(L"Rainmeter Skin Packager", &hMutex))
	{
		HWND hwnd = FindWindow(L"#32770", L"Rainmeter Skin Packager");
		SetForegroundWindow(hwnd);
	}
	else
	{
		DialogPackage dialog;
		c_Dialog = &dialog;
		dialog.ShowDialogWindow(
			L"Rainmeter Skin Packager",
			0, 0, 300, 283,
			DS_CENTER | WS_POPUP | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU,
			WS_EX_APPWINDOW | WS_EX_CONTROLPARENT,
			nullptr,
			false);
		c_Dialog = nullptr;
		ReleaseMutex(hMutex);
	}
}

INT_PTR DialogPackage::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const INT_PTR baseResult = Dialog::HandleMessage(uMsg, wParam, lParam);

	switch (uMsg)
	{
	case WM_INITDIALOG:
		return OnInitDialog(wParam, lParam);

	case WM_COMMAND:
		return OnCommand(wParam, lParam);

	case WM_CLOSE:
		if (!m_PackagerThread)
		{
			EndDialog(m_Window, 0);
		}
		return TRUE;
	}

	return baseResult;
}

INT_PTR DialogPackage::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	const Control controls[] =
	{
		Control::Button(DialogPackage::Id_NextButton, 0,
			188, 264, 50, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED | BS_DEFPUSHBUTTON, 0),
		Control::Button(DialogPackage::Id_CreatePackageButton, 0,
			158, 264, 80, 14,
			WS_TABSTOP, 0),
		Control::Button(IDCANCEL, 0,
			243, 264, 50, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		Control::Tab(DialogPackage::Id_Tab, 0,
			6, 6, 288, 254,
			WS_VISIBLE | WS_TABSTOP | TCS_FIXEDWIDTH, 0)
	};

	CreateControls(controls, _countof(controls), GetString);
	SetWindowText(GetControl(DialogPackage::Id_NextButton), L"Next");
	SetWindowText(GetControl(DialogPackage::Id_CreatePackageButton), L"Create package");
	SetWindowText(GetControl(IDCANCEL), L"Cancel");
	AddPage(m_TabInfo);

	HICON hIcon = GetIcon(IDI_SKININSTALLER, true);
	SendMessage(m_Window, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);  // Titlebar icon: 16x16
	SendMessage(m_Window, WM_SETICON, ICON_BIG, (LPARAM)hIcon);    // Taskbar icon:  32x32

	m_TabInfo.Activate();

	return FALSE;
}

INT_PTR DialogPackage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case DialogPackage::Id_NextButton:
		{
			AddTab(DialogPackage::Id_Tab, m_TabOptions, L"Options");
			AddTab(DialogPackage::Id_Tab, m_TabAdvanced, L"Advanced");

			HWND item = GetDlgItem(m_Window, DialogPackage::Id_NextButton);
			ShowWindow(item, SW_HIDE);

			item = GetDlgItem(m_Window, DialogPackage::Id_CreatePackageButton);
			ShowWindow(item, SW_SHOWNORMAL);
			SendMessage(m_Window, DM_SETDEFID, DialogPackage::Id_CreatePackageButton, 0);

			ShowWindow(m_TabInfo.GetWindow(), SW_HIDE);
			SelectTab(0);
		}
		break;

	case DialogPackage::Id_CreatePackageButton:
		{
			HWND item = GetDlgItem(m_Window, DialogPackage::Id_CreatePackageButton);
			EnableWindow(item, FALSE);

			item = GetDlgItem(m_Window, IDCANCEL);
			EnableWindow(item, FALSE);

			SelectTab(0);
			item = GetDlgItem(m_Window, DialogPackage::Id_Tab);
			EnableWindow(item, FALSE);
			EnableWindow(m_TabOptions.GetWindow(), FALSE);
			EnableWindow(m_TabAdvanced.GetWindow(), FALSE);

			item = GetDlgItem(m_TabOptions.GetWindow(), DialogPackage::TabOptions::Id_CreatingText);
			ShowWindow(item, SW_SHOWNORMAL);

			item = GetDlgItem(m_TabOptions.GetWindow(), DialogPackage::TabOptions::Id_CreatingBar);
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

void DialogPackage::SetNextButtonState()
{
	BOOL state = !(m_Name.empty() || m_Author.empty() || m_SkinFolder.second.empty());
	EnableWindow(GetDlgItem(m_Window, DialogPackage::Id_NextButton), state);
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

	if (c_Dialog->m_MergeSkins)
	{
		WritePrivateProfileString(L"rmskin", L"MergeSkins", L"1", tempFile);
	}
	else
	{
		// "Merge skins" not compatible with "Variable files"
		if (!c_Dialog->m_VariableFiles.empty())
		{
			WritePrivateProfileString(L"rmskin", L"VariableFiles", m_VariableFiles.c_str(), tempFile);
		}
	}

	WritePrivateProfileString(L"rmskin", L"MinimumRainmeter", m_MinimumRainmeter.c_str(), tempFile);
	WritePrivateProfileString(L"rmskin", L"MinimumWindows", m_MinimumWindows.c_str(), tempFile);

	// Only Skin Installer in Rainmeter 3.0.1 support UTF-8 filenames.
	m_AllowNonAsciiFilenames = DialogInstall::CompareVersions(m_MinimumRainmeter, L"3.0.1") != -1;

	// Create archive and add options file and header bitmap
	zlib_filefunc64_def zlibFileFunc = { 0 };
	fill_win32_filefunc64W(&zlibFileFunc);
	m_ZipFile = zipOpen2_64(m_TargetFile.c_str(), APPEND_STATUS_CREATE, nullptr, &zlibFileFunc);

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
	FILE* file = nullptr;
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
		HWND item = GetDlgItem(dialog->m_TabOptions.GetWindow(), DialogPackage::TabOptions::Id_CreatingBar);
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
	zip_fileinfo fi = { 0 };
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
				const DWORD bufferSize = 16UL * 1024UL;
				BYTE buffer[bufferSize] = { 0 };
				DWORD readSize = 0UL;
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

	WIN32_FIND_DATA fd = { 0 };
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
	const WCHAR* url = L"https://docs.rainmeter.net/manual/distributing-skins/";
	ShellExecute(m_Window, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

class DialogPackage::SelectFolderDialog : public Dialog
{
public:
	enum Id
	{
		Id_ExistingRadio = 1000,
		Id_ExistingCombo,
		Id_CustomRadio,
		Id_CustomEdit,
		Id_CustomBrowseButton
	};

	SelectFolderDialog(const std::wstring& existingPath) :
		m_ExistingPath(existingPath),
		m_Accepted(false)
	{
	}

	std::wstring ShowModal(HWND parent)
	{
		ShowDialogWindow(
			L"Add",
			0, 0, 200, 100,
			DS_CENTER | WS_POPUP | WS_TILEDWINDOW,
			WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT,
			parent,
			false);
		return m_Accepted ? m_Result : std::wstring();
	}

protected:
	INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		const INT_PTR baseResult = Dialog::HandleMessage(uMsg, wParam, lParam);
		switch (uMsg)
		{
		case WM_INITDIALOG:
			return OnInitDialog();
		case WM_COMMAND:
			return OnCommand(wParam, lParam);
		case WM_CLOSE:
			EndDialog(m_Window, 0);
			return TRUE;
		}
		return baseResult;
	}

private:
	INT_PTR OnInitDialog()
	{
		const Control controls[] =
		{
			Control::RadioButton(SelectFolderDialog::Id_ExistingRadio, 0,
				6, 6, 188, 13,
				WS_VISIBLE | WS_TABSTOP | WS_GROUP, 0),
			Control::ComboBox(SelectFolderDialog::Id_ExistingCombo, 0,
				17, 21, 176, 14,
				WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, 0),
			Control::RadioButton(SelectFolderDialog::Id_CustomRadio, 0,
				6, 44, 188, 13,
				WS_VISIBLE | WS_TABSTOP, 0),
			Control::Edit(SelectFolderDialog::Id_CustomEdit, 0,
				17, 59, 146, 14,
				WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_DISABLED | ES_AUTOHSCROLL, 0),
			Control::Button(SelectFolderDialog::Id_CustomBrowseButton, 0,
				168, 59, 25, 14,
				WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
			Control::Button(IDOK, 0,
				146, 82, 50, 14,
				WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0)
		};

		CreateControls(controls, _countof(controls), GetString);
		SetWindowText(GetControl(SelectFolderDialog::Id_CustomRadio), L"Add custom folder:");
		SetWindowText(GetControl(SelectFolderDialog::Id_CustomBrowseButton), L"...");
		SetWindowText(GetControl(IDOK), L"Add");
		Button_SetCheck(GetControl(SelectFolderDialog::Id_ExistingRadio), BST_CHECKED);
		EnableThemeDialogTexture(m_Window, ETDT_ENABLETAB);

		std::wstring searchPath = m_ExistingPath + L'*';
		WIN32_FIND_DATA fd = { 0 };
		HANDLE hFind = FindFirstFileEx(searchPath.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, 0);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			const WCHAR* folder = PathFindFileName(m_ExistingPath.c_str());
			std::wstring text = L"Add folder from ";
			text.append(folder, wcslen(folder) - 1);
			text += L':';
			SetWindowText(GetControl(SelectFolderDialog::Id_ExistingRadio), text.c_str());

			HWND combo = GetControl(SelectFolderDialog::Id_ExistingCombo);
			do
			{
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
					!(fd.cFileName[0] == L'.' && (!fd.cFileName[1] || fd.cFileName[1] == L'.' && !fd.cFileName[2])) &&
					wcscmp(fd.cFileName, L"Backup") != 0 &&
					wcscmp(fd.cFileName, L"@Backup") != 0 &&
					wcscmp(fd.cFileName, L"@Vault") != 0)
				{
					ComboBox_InsertString(combo, -1, fd.cFileName);
				}
			}
			while (FindNextFile(hFind, &fd));

			ComboBox_SetCurSel(combo, 0);
			FindClose(hFind);
		}

		return TRUE;
	}

	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam)
	{
		switch (LOWORD(wParam))
		{
		case SelectFolderDialog::Id_ExistingRadio:
			{
				HWND combo = GetControl(SelectFolderDialog::Id_ExistingCombo);
				EnableWindow(combo, TRUE);
				EnableWindow(GetControl(SelectFolderDialog::Id_CustomEdit), FALSE);
				EnableWindow(GetControl(SelectFolderDialog::Id_CustomBrowseButton), FALSE);
				EnableWindow(GetControl(IDOK), ComboBox_GetCurSel(combo) != -1);
			}
			break;

		case SelectFolderDialog::Id_CustomRadio:
			EnableWindow(GetControl(SelectFolderDialog::Id_ExistingCombo), FALSE);
			EnableWindow(GetControl(SelectFolderDialog::Id_CustomEdit), TRUE);
			EnableWindow(GetControl(SelectFolderDialog::Id_CustomBrowseButton), TRUE);
			SendMessage(m_Window, WM_COMMAND, MAKEWPARAM(SelectFolderDialog::Id_CustomEdit, EN_CHANGE), 0);
			break;

		case SelectFolderDialog::Id_CustomEdit:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				WCHAR buffer[MAX_PATH] = { 0 };
				Edit_GetText(GetControl(SelectFolderDialog::Id_CustomEdit), buffer, _countof(buffer));
				DWORD attributes = GetFileAttributes(buffer);
				BOOL state = attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
				EnableWindow(GetControl(IDOK), state);
			}
			break;

		case SelectFolderDialog::Id_CustomBrowseButton:
			{
				WCHAR buffer[MAX_PATH] = { 0 };
				BROWSEINFO bi = { 0 };
				bi.hwndOwner = m_Window;
				bi.ulFlags = BIF_USENEWUI | BIF_NONEWFOLDERBUTTON | BIF_RETURNONLYFSDIRS;

				PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&bi);
				if (pidl)
				{
					if (SHGetPathFromIDList(pidl, buffer))
					{
						SetWindowText(GetControl(SelectFolderDialog::Id_CustomEdit), buffer);
					}
					CoTaskMemFree(pidl);
				}
			}
			break;

		case IDOK:
			{
				WCHAR buffer[MAX_PATH] = { 0 };
				const bool existing = Button_GetCheck(GetControl(SelectFolderDialog::Id_ExistingRadio)) == BST_CHECKED;
				GetWindowText(GetControl(existing ? SelectFolderDialog::Id_ExistingCombo : SelectFolderDialog::Id_CustomEdit), buffer, _countof(buffer));
				m_Result = existing ? m_ExistingPath + buffer : buffer;
				m_Result += L'\\';
				m_Accepted = true;
				EndDialog(m_Window, 0);
			}
			break;

		case IDCANCEL:
			EndDialog(m_Window, 0);
			break;

		default:
			return FALSE;
		}

		return TRUE;
	}

	std::wstring m_ExistingPath;
	std::wstring m_Result;
	bool m_Accepted;
};

class DialogPackage::SelectPluginDialog : public Dialog
{
public:
	enum Id
	{
		Id_32BitEdit = 1000,
		Id_32BitBrowseButton,
		Id_64BitEdit,
		Id_64BitBrowseButton,
		Id_32BitLabel = 1100,
		Id_64BitLabel
	};

	SelectPluginDialog() : m_Accepted(false)
	{
	}

	std::pair<std::wstring, std::wstring> ShowModal(HWND parent)
	{
		ShowDialogWindow(
			L"Add",
			0, 0, 200, 100,
			DS_CENTER | WS_POPUP | WS_TILEDWINDOW,
			WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT,
			parent,
			false);
		return m_Accepted ? m_Plugins : std::pair<std::wstring, std::wstring>();
	}

protected:
	INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		const INT_PTR baseResult = Dialog::HandleMessage(uMsg, wParam, lParam);
		switch (uMsg)
		{
		case WM_INITDIALOG:
			return OnInitDialog();
		case WM_COMMAND:
			return OnCommand(wParam, lParam);
		case WM_CLOSE:
			EndDialog(m_Window, 0);
			return TRUE;
		}
		return baseResult;
	}

private:
	INT_PTR OnInitDialog()
	{
		const Control controls[] =
		{
			Control::Label(Id_32BitLabel, 0,
				6, 6, 188, 13,
				WS_VISIBLE, 0),
			Control::Edit(SelectPluginDialog::Id_32BitEdit, 0,
				6, 20, 157, 14,
				WS_VISIBLE | WS_BORDER | ES_READONLY | ES_AUTOHSCROLL, 0),
			Control::Button(SelectPluginDialog::Id_32BitBrowseButton, 0,
				168, 20, 25, 14,
				WS_VISIBLE | WS_TABSTOP, 0),
			Control::Label(Id_64BitLabel, 0,
				6, 42, 188, 13,
				WS_VISIBLE, 0),
			Control::Edit(SelectPluginDialog::Id_64BitEdit, 0,
				6, 55, 157, 14,
				WS_VISIBLE | WS_BORDER | ES_READONLY | ES_AUTOHSCROLL, 0),
			Control::Button(SelectPluginDialog::Id_64BitBrowseButton, 0,
				168, 55, 25, 14,
				WS_VISIBLE | WS_TABSTOP, 0),
			Control::Button(IDOK, 0,
				146, 82, 50, 14,
				WS_VISIBLE | WS_TABSTOP | WS_DISABLED | BS_DEFPUSHBUTTON, 0)
		};

		CreateControls(controls, _countof(controls), GetString);
		SetWindowText(GetControl(Id_32BitLabel), L"32-bit DLL:");
		SetWindowText(GetControl(SelectPluginDialog::Id_32BitBrowseButton), L"...");
		SetWindowText(GetControl(Id_64BitLabel), L"64-bit DLL:");
		SetWindowText(GetControl(SelectPluginDialog::Id_64BitBrowseButton), L"...");
		SetWindowText(GetControl(IDOK), L"Add");
		EnableThemeDialogTexture(m_Window, ETDT_ENABLETAB);
		return TRUE;
	}

	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam)
	{
		switch (LOWORD(wParam))
		{
		case SelectPluginDialog::Id_32BitBrowseButton:
		case SelectPluginDialog::Id_64BitBrowseButton:
			{
				WCHAR buffer[MAX_PATH] = { 0 };
				OPENFILENAME ofn = { sizeof(OPENFILENAME) };
				ofn.Flags = OFN_FILEMUSTEXIST;
				ofn.lpstrFilter = L"Plugins (.dll)\0*.dll";
				ofn.lpstrTitle = L"Select plugin file";
				ofn.lpstrDefExt = L"dll";
				ofn.nFilterIndex = 0UL;
				ofn.lpstrFile = buffer;
				ofn.nMaxFile = _countof(buffer);
				ofn.hwndOwner = m_Window;

				if (!GetOpenFileName(&ofn)) break;

				const bool x32 = LOWORD(wParam) == SelectPluginDialog::Id_32BitBrowseButton;
				WORD machine = 0;
				if (FileUtil::GetBinaryFileBitness(buffer, machine) &&
					((x32 && machine == IMAGE_FILE_MACHINE_I386) || (!x32 && machine == IMAGE_FILE_MACHINE_AMD64)))
				{
					const WCHAR* otherName = PathFindFileName(x32 ? m_Plugins.second.c_str() : m_Plugins.first.c_str());
					if (*otherName && _wcsicmp(otherName, PathFindFileName(buffer)) != 0)
					{
						MessageBox(m_Window, L"Plugins must have same name.", L"Rainmeter Skin Packager", MB_OK | MB_TOPMOST);
						break;
					}

					PathSetDlgItemPath(m_Window, x32 ? SelectPluginDialog::Id_32BitEdit : SelectPluginDialog::Id_64BitEdit, buffer);
					(x32 ? m_Plugins.first : m_Plugins.second) = buffer;
					EnableWindow(GetControl(IDOK), !m_Plugins.first.empty() && !m_Plugins.second.empty());
					break;
				}

				MessageBox(m_Window, L"Invalid plugin.", L"Rainmeter Skin Packager", MB_OK | MB_TOPMOST);
			}
			break;

		case IDOK:
			m_Accepted = true;
			EndDialog(m_Window, 0);
			break;

		case IDCANCEL:
			EndDialog(m_Window, 0);
			break;

		default:
			return FALSE;
		}

		return TRUE;
	}

	std::pair<std::wstring, std::wstring> m_Plugins;
	bool m_Accepted;
};

std::wstring DialogPackage::SelectFolder(HWND parent, const std::wstring& existingPath)
{
	SelectFolderDialog dialog(existingPath);
	return dialog.ShowModal(parent);
}

std::pair<std::wstring, std::wstring> DialogPackage::SelectPlugin(HWND parent)
{
	SelectPluginDialog dialog;
	return dialog.ShowModal(parent);
}

// -----------------------------------------------------------------------------------------------
//
//                                Info tab
//
// -----------------------------------------------------------------------------------------------

void DialogPackage::TabInfo::Create(HWND owner)
{
	Tab::CreateTabWindow(15, 15, 270, 238, owner);

	const Control controls[] =
	{
		Control::Label(Id_DescriptionLabel, 0,
			0, 0, 264, 26,
			WS_VISIBLE, 0),
		Control::GroupBox(Id_InformationGroup, 0,
			0, 35, 270, 73,
			WS_VISIBLE, 0),
		Control::Label(Id_NameLabel, 0,
			6, 51, 35, 13,
			WS_VISIBLE, 0),
		Control::Edit(DialogPackage::TabInfo::Id_NameEdit, 0,
			56, 50, 208, 14,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 0),
		Control::Label(Id_AuthorLabel, 0,
			6, 69, 35, 13,
			WS_VISIBLE, 0),
		Control::Edit(DialogPackage::TabInfo::Id_AuthorEdit, 0,
			56, 68, 208, 14,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 0),
		Control::Label(Id_VersionLabel, 0,
			6, 87, 35, 13,
			WS_VISIBLE, 0),
		Control::Edit(DialogPackage::TabInfo::Id_VersionEdit, 0,
			56, 86, 140, 14,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 0),
		Control::GroupBox(Id_ComponentsGroup, 0,
			0, 113, 270, 108,
			WS_VISIBLE, 0),
		Control::ListView(DialogPackage::TabInfo::Id_ComponentsList, 0,
			6, 128, 182, 86,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER, 0),
		Control::Button(DialogPackage::TabInfo::Id_AddSkinButton, 0,
			194, 128, 70, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		Control::Button(DialogPackage::TabInfo::Id_AddLayoutButton, 0,
			194, 147, 70, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		Control::Button(DialogPackage::TabInfo::Id_AddPluginButton, 0,
			194, 165, 70, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		Control::Button(DialogPackage::TabInfo::Id_RemoveButton, 0,
			194, 200, 70, 14,
			WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 0),
		Control::LinkLabel(DialogPackage::TabInfo::Id_WhatIsLink, 0,
			0, 227, 264, 13,
			WS_VISIBLE | WS_TABSTOP, 0)
	};

	CreateControls(controls, _countof(controls), GetString);
	SetWindowText(GetControl(Id_DescriptionLabel), L"Enter the information and select the components to use for the .rmskin package.");
	SetWindowText(GetControl(Id_InformationGroup), L"Information");
	SetWindowText(GetControl(Id_NameLabel), L"Name:");
	SetWindowText(GetControl(Id_AuthorLabel), L"Author:");
	SetWindowText(GetControl(Id_VersionLabel), L"Version:");
	SetWindowText(GetControl(Id_ComponentsGroup), L"Components");
	SetWindowText(GetControl(DialogPackage::TabInfo::Id_AddSkinButton), L"Add skin...");
	SetWindowText(GetControl(DialogPackage::TabInfo::Id_AddLayoutButton), L"Add layout...");
	SetWindowText(GetControl(DialogPackage::TabInfo::Id_AddPluginButton), L"Add plugin...");
	SetWindowText(GetControl(DialogPackage::TabInfo::Id_RemoveButton), L"Remove");
	SetWindowText(GetControl(DialogPackage::TabInfo::Id_WhatIsLink), L"<A>What is a .rmskin package?</A>");
}

void DialogPackage::TabInfo::Initialize()
{
	m_Initialized = true;

	HWND item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_NameEdit);
	Edit_SetCueBannerText(item, L"...");

	item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_AuthorEdit);
	Edit_SetCueBannerText(item, L"...");

	item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_VersionEdit);
	Edit_SetCueBannerText(item, L"...");

	item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_ComponentsList);

	DWORD extendedFlags = LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
	SetWindowTheme(item, L"explorer", nullptr);
	ListView_EnableGroupView(item, TRUE);
	ListView_SetExtendedListViewStyleEx(item, 0, extendedFlags);

	// Add columns
	LVCOLUMN lvc = { 0 };
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.iSubItem = 0;
	lvc.cx = 252;
	lvc.pszText = (WCHAR*)L"Name";
	ListView_InsertColumn(item, 0, &lvc);

	// Add groups
	LVGROUP lvg = { 0 };
	lvg.cbSize = sizeof(LVGROUP);
	lvg.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
	lvg.state = LVGS_COLLAPSIBLE;
	lvg.iGroupId = 0;
	lvg.pszHeader = (WCHAR*)L"Skin";
	ListView_InsertGroup(item, -1, &lvg);
	lvg.iGroupId = 1;
	lvg.pszHeader = (WCHAR*)L"Layouts";
	ListView_InsertGroup(item, -1, &lvg);
	lvg.iGroupId = 2;
	lvg.pszHeader = (WCHAR*)L"Plugins";
	ListView_InsertGroup(item, -1, &lvg);
}

INT_PTR DialogPackage::TabInfo::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
	case DialogPackage::TabInfo::Id_AddSkinButton:
		{
			c_Dialog->m_SkinFolder.second = SelectFolder(m_Window, g_Data.skinsPath);
			if (!c_Dialog->m_SkinFolder.second.empty())
			{
				c_Dialog->m_SkinFolder.first = PathFindFileName(c_Dialog->m_SkinFolder.second.c_str());
				c_Dialog->m_SkinFolder.first.pop_back();	// Remove slash

				HWND item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_ComponentsList);
				LVITEM lvi = { 0 };
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

	case DialogPackage::TabInfo::Id_AddLayoutButton:
		{
			std::wstring folder = SelectFolder(m_Window, g_Data.settingsPath + L"Layouts\\");
			if (!folder.empty())
			{
				std::wstring name = PathFindFileName(folder.c_str());
				name.pop_back();	// Remove slash

				if (c_Dialog->m_LayoutFolders.emplace(name, folder).second)
				{
					HWND item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_ComponentsList);
					LVITEM lvi = { 0 };
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

	case DialogPackage::TabInfo::Id_AddPluginButton:
		{
			std::pair<std::wstring, std::wstring> plugins = SelectPlugin(m_Window);
			std::wstring name = PathFindFileName(plugins.first.c_str());
			if (!name.empty() && c_Dialog->m_PluginFolders.emplace(name, plugins).second)
			{
				HWND item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_ComponentsList);
				LVITEM lvi = { 0 };
				lvi.mask = LVIF_TEXT | LVIF_GROUPID;
				lvi.iItem = (int)c_Dialog->m_PluginFolders.size() + 1;
				lvi.iSubItem = 0;
				lvi.iGroupId = 2;
				lvi.pszText = (WCHAR*)name.c_str();
				ListView_InsertItem(item, &lvi);
			}
		}
		break;

	case DialogPackage::TabInfo::Id_RemoveButton:
		{
			HWND item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_ComponentsList);
			int sel = ListView_GetNextItem(item, -1, LVNI_FOCUSED | LVNI_SELECTED);
			if (sel != -1)
			{
				WCHAR buffer[MAX_PATH] = { 0 };

				// Remove unchecked items from the component sets
				LVITEM lvi = { 0 };
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
					item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_AddSkinButton);
					EnableWindow(item, TRUE);
					c_Dialog->m_SkinFolder.first.clear();
					c_Dialog->m_SkinFolder.second.clear();
					c_Dialog->SetNextButtonState();
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

	case DialogPackage::TabInfo::Id_NameEdit:
	case DialogPackage::TabInfo::Id_AuthorEdit:
	case DialogPackage::TabInfo::Id_VersionEdit:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			WCHAR buffer[64] = { 0 };
			int len = GetWindowText((HWND)lParam, buffer, _countof(buffer));
			if (LOWORD(wParam) == DialogPackage::TabInfo::Id_NameEdit)
			{
				c_Dialog->m_Name.assign(buffer, len);
			}
			else if (LOWORD(wParam) == DialogPackage::TabInfo::Id_AuthorEdit)
			{
				c_Dialog->m_Author.assign(buffer, len);
			}
			else // if (LOWORD(wParam) == DialogPackage::TabInfo::Id_VersionEdit)
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
			if (nm->idFrom == DialogPackage::TabInfo::Id_ComponentsList)
			{
				BOOL selected = (nmlv->uNewState & LVIS_SELECTED);

				HWND item = GetDlgItem(m_Window, DialogPackage::TabInfo::Id_RemoveButton);
				EnableWindow(item, selected);
			}
		}
		break;

	case NM_CLICK:
		{
			if (nm->idFrom == DialogPackage::TabInfo::Id_WhatIsLink)
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

void DialogPackage::TabOptions::Create(HWND owner)
{
	Tab::CreateTabWindow(15, 30, 270, 220, owner);

	const Control controls[] =
	{
		Control::Label(Id_SaveLabel, 0,
			0, 0, 264, 13,
			WS_VISIBLE, 0),
		Control::Edit(DialogPackage::TabOptions::Id_FileEdit, 0,
			0, 17, 240, 14,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_READONLY | ES_AUTOHSCROLL, 0),
		Control::Button(DialogPackage::TabOptions::Id_FileBrowseButton, 0,
			245, 17, 25, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		Control::GroupBox(Id_AfterInstallGroup, 0,
			0, 101, 270, 58,
			WS_VISIBLE, 0),
		Control::RadioButton(DialogPackage::TabOptions::Id_DoNothingRadio, 0,
			6, 116, 85, 13,
			WS_VISIBLE | WS_TABSTOP | WS_GROUP, 0),
		Control::RadioButton(DialogPackage::TabOptions::Id_LoadSkinRadio, 0,
			6, 129, 85, 13,
			WS_VISIBLE | WS_TABSTOP, 0),
		Control::Edit(DialogPackage::TabOptions::Id_LoadSkinEdit, 0,
			96, 126, 138, 14,
			WS_TABSTOP | WS_BORDER | ES_READONLY | ES_AUTOHSCROLL, 0),
		Control::Button(DialogPackage::TabOptions::Id_LoadSkinBrowseButton, 0,
			239, 126, 25, 14,
			WS_TABSTOP, 0),
		Control::RadioButton(DialogPackage::TabOptions::Id_LoadLayoutRadio, 0,
			6, 142, 85, 13,
			WS_VISIBLE | WS_TABSTOP, 0),
		Control::ComboBox(DialogPackage::TabOptions::Id_LoadLayoutCombo, 0,
			96, 139, 168, 14,
			WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, 0),
		Control::GroupBox(Id_RequirementsGroup, 0,
			0, 164, 270, 53,
			WS_VISIBLE | WS_GROUP, 0),
		Control::Label(Id_RainmeterVersionLabel, 0,
			6, 180, 85, 13,
			WS_VISIBLE, 0),
		Control::Edit(DialogPackage::TabOptions::Id_RainmeterVersionEdit, 0,
			96, 177, 80, 14,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 0),
		Control::Label(Id_WindowsVersionLabel, 0,
			6, 198, 85, 13,
			WS_VISIBLE, 0),
		Control::ComboBox(DialogPackage::TabOptions::Id_WindowsVersionCombo, 0,
			96, 195, 80, 14,
			WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, 0),
		Control::Label(DialogPackage::TabOptions::Id_CreatingText, 0,
			0, 0, 270, 100,
			0, 0),
		Control::ProgressBar(DialogPackage::TabOptions::Id_CreatingBar, 0,
			0, 15, 270, 11,
			PBS_MARQUEE | WS_BORDER, 0)
	};

	CreateControls(controls, _countof(controls), GetString);
	SetWindowText(GetControl(Id_SaveLabel), L"Save package to:");
	SetWindowText(GetControl(DialogPackage::TabOptions::Id_FileBrowseButton), L"...");
	SetWindowText(GetControl(Id_AfterInstallGroup), L"After installation");
	SetWindowText(GetControl(DialogPackage::TabOptions::Id_DoNothingRadio), L"Do nothing");
	SetWindowText(GetControl(DialogPackage::TabOptions::Id_LoadSkinRadio), L"Load skin");
	SetWindowText(GetControl(DialogPackage::TabOptions::Id_LoadSkinBrowseButton), L"...");
	SetWindowText(GetControl(DialogPackage::TabOptions::Id_LoadLayoutRadio), L"Load layout");
	SetWindowText(GetControl(Id_RequirementsGroup), L"Minimum requirements");
	SetWindowText(GetControl(Id_RainmeterVersionLabel), L"Rainmeter version:");
	SetWindowText(GetControl(Id_WindowsVersionLabel), L"Windows version:");
	SetWindowText(GetControl(DialogPackage::TabOptions::Id_CreatingText), L"Creating...");
}

void DialogPackage::TabOptions::Initialize()
{
	m_Initialized = true;

	WCHAR buffer[MAX_PATH] = { 0 };
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

	HWND item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_FileEdit);
	SetWindowText(item,c_Dialog->m_TargetFile.c_str());

	item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadLayoutRadio);
	if (c_Dialog->m_LayoutFolders.empty())
	{
		EnableWindow(item, FALSE);

		item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_DoNothingRadio);
		Button_SetCheck(item, BST_CHECKED);
	}
	else
	{
		c_Dialog->m_LoadLayout = true;
		c_Dialog->m_Load = (*c_Dialog->m_LayoutFolders.cbegin()).first;

		Button_SetCheck(item, BST_CHECKED);

		item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadLayoutCombo);
		ShowWindow(item, SW_SHOWNORMAL);

		for (auto iter = c_Dialog->m_LayoutFolders.cbegin(); iter != c_Dialog->m_LayoutFolders.cend(); ++iter)
		{
			ComboBox_AddString(item, (*iter).first.c_str());
		}
		ComboBox_SetCurSel(item, 0);
	}

	item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinEdit);
	Edit_SetCueBannerText(item, L"Select skin");

	item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_RainmeterVersionEdit);
	_snwprintf_s(buffer, _TRUNCATE, L"%s.%i", APPVERSION, revision_number);
	SetWindowText(item, buffer);
	c_Dialog->m_MinimumRainmeter = buffer;

	item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_WindowsVersionCombo);
	ComboBox_AddString(item, L"XP");
	ComboBox_AddString(item, L"Vista");
	ComboBox_AddString(item, L"7");
	ComboBox_AddString(item, L"8");
	ComboBox_AddString(item, L"10");
	ComboBox_SetCurSel(item, 2);
	c_Dialog->m_MinimumWindows = g_OsNameVersions[0].version;
}

INT_PTR DialogPackage::TabOptions::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
	case DialogPackage::TabOptions::Id_FileBrowseButton:
		{
			WCHAR buffer[MAX_PATH] = { 0 };
			HWND item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_FileEdit);
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

	case DialogPackage::TabOptions::Id_DoNothingRadio:
		{
			HWND item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinEdit);
			ShowWindow(item, SW_HIDE);
			item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinBrowseButton);
			ShowWindow(item, SW_HIDE);
			item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadLayoutCombo);
			ShowWindow(item, SW_HIDE);

			c_Dialog->m_Load.clear();
		}
		break;

	case DialogPackage::TabOptions::Id_LoadSkinRadio:
		{
			HWND item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinEdit);
			ShowWindow(item, SW_SHOWNORMAL);

			WCHAR buffer[MAX_PATH] = { 0 };
			GetWindowText(item, buffer, _countof(buffer));
			c_Dialog->m_Load = buffer;
			c_Dialog->m_LoadLayout = false;

			item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinBrowseButton);
			ShowWindow(item, SW_SHOWNORMAL);
			item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadLayoutCombo);
			ShowWindow(item, SW_HIDE);
		}
		break;

	case DialogPackage::TabOptions::Id_LoadLayoutRadio:
		{
			HWND item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinEdit);
			ShowWindow(item, SW_HIDE);
			item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinBrowseButton);
			ShowWindow(item, SW_HIDE);
			item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadLayoutCombo);
			ShowWindow(item, SW_SHOWNORMAL);

			WCHAR buffer[MAX_PATH] = { 0 };
			GetWindowText(item, buffer, _countof(buffer));
			c_Dialog->m_Load = buffer;
			c_Dialog->m_LoadLayout = true;
		}
		break;

	case DialogPackage::TabOptions::Id_LoadLayoutCombo:
		if (HIWORD(wParam) == CBN_SELENDOK)
		{
			WCHAR buffer[MAX_PATH] = { 0 };
			HWND item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadLayoutCombo);
			GetWindowText(item, buffer, _countof(buffer));
			c_Dialog->m_Load = buffer;
			c_Dialog->m_LoadLayout = true;
		}
		break;

	case DialogPackage::TabOptions::Id_LoadSkinBrowseButton:
		{
			WCHAR buffer[MAX_PATH] = { 0 };
			HWND item = GetDlgItem(m_Window, DialogPackage::TabOptions::Id_LoadSkinEdit);
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

	case DialogPackage::TabOptions::Id_RainmeterVersionEdit:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			WCHAR buffer[32] = { 0 };
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

	case DialogPackage::TabOptions::Id_WindowsVersionCombo:
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

void DialogPackage::TabAdvanced::Create(HWND owner)
{
	Tab::CreateTabWindow(15, 30, 270, 220, owner);

	const Control controls[] =
	{
		Control::Label(Id_HeaderLabel, 0,
			0, 3, 85, 13,
			WS_VISIBLE, 0),
		Control::Edit(DialogPackage::TabAdvanced::Id_HeaderEdit, 0,
			90, 0, 150, 14,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_READONLY | ES_AUTOHSCROLL, 0),
		Control::Button(DialogPackage::TabAdvanced::Id_HeaderBrowseButton, 0,
			245, 0, 25, 14,
			WS_VISIBLE | WS_TABSTOP, 0),
		Control::Label(Id_VariablesLabel, 0,
			0, 24, 85, 13,
			WS_VISIBLE, 0),
		Control::Edit(DialogPackage::TabAdvanced::Id_VariableFilesEdit, 0,
			90, 21, 180, 14,
			WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 0),
		Control::CheckBox(DialogPackage::TabAdvanced::Id_MergeSkinsCheck, 0,
			0, 42, 85, 13,
			WS_VISIBLE | WS_TABSTOP, 0),
		Control::LinkLabel(DialogPackage::TabAdvanced::Id_HelpLink, 0,
			0, 210, 264, 13,
			WS_VISIBLE | WS_TABSTOP, 0)
	};

	CreateControls(controls, _countof(controls), GetString);
	SetWindowText(GetControl(Id_HeaderLabel), L"Header image:");
	SetWindowText(GetControl(DialogPackage::TabAdvanced::Id_HeaderBrowseButton), L"...");
	SetWindowText(GetControl(Id_VariablesLabel), L"Variables files:");
	SetWindowText(GetControl(DialogPackage::TabAdvanced::Id_MergeSkinsCheck), L"Merge skins");
	SetWindowText(GetControl(DialogPackage::TabAdvanced::Id_HelpLink), L"<A>Help</A>");
}

void DialogPackage::TabAdvanced::Initialize()
{
	m_Initialized = true;
}

INT_PTR DialogPackage::TabAdvanced::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
	case DialogPackage::TabAdvanced::Id_HeaderBrowseButton:
		{
			WCHAR buffer[MAX_PATH] = { 0 };
			HWND item = GetDlgItem(m_Window, DialogPackage::TabAdvanced::Id_HeaderEdit);
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

	case DialogPackage::TabAdvanced::Id_VariableFilesEdit:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			int length = GetWindowTextLength((HWND)lParam);
			c_Dialog->m_VariableFiles.resize(length);
			GetWindowText((HWND)lParam, &c_Dialog->m_VariableFiles[0], length + 1);
		}
		break;

	case DialogPackage::TabAdvanced::Id_MergeSkinsCheck:
		{
			c_Dialog->m_MergeSkins = !c_Dialog->m_MergeSkins;

			// "Merge skins" not compatible with "Variable files"
			HWND item = GetDlgItem(m_Window, DialogPackage::TabAdvanced::Id_VariableFilesEdit);
			Edit_Enable(item, !c_Dialog->m_MergeSkins);
		}
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
			if (nm->idFrom == DialogPackage::TabAdvanced::Id_HelpLink)
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
