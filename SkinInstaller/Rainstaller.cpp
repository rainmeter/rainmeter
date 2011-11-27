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
#include "resource.h"
#include "unzip.h"
#include "Application.h"
#include "Rainstaller.h"

RMSKIN_DATA data;
HBITMAP hBitmap;
HANDLE hMapFile;
LPCTSTR pBuffer;
std::wstring exeFile;
#define APP_NAME L"Rainstaller"

/*
** Entry point
*/
int Rainstaller(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	if (wcscmp(lpCmdLine, L"/ELEVATE") == 0)
	{
		if (!IsCurrentProcessAdmin())
		{
			std::wstring error = L"Elevated process does not have administrative rights.";
			MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
			return 1;
		}

		// If the main (or dialog) process of Rainstaller isn't running as admin, it calls CreateProcessElevated()
		// to create create an elevated process of Rainstaller with the /ELEVATE switch.
		hMapFile = OpenFileMapping(FILE_MAP_READ, FALSE, OBJECT_NAME);
		if (hMapFile == NULL)
		{
			std::wstring error = L"Error in elevated process: unable to open file mapping.";
			MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
			return 1;
		}

		pBuffer = (LPCTSTR)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 2048);
		if (pBuffer == NULL)
		{
			CloseHandle(hMapFile);
			std::wstring error = L"Error in elevated process: unable to view map of file.";
			MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
			return 1;
		}

		// Split the shared buffer into tokens
		std::vector<std::wstring> vec = Tokenize(pBuffer, L"\"");
		UnmapViewOfFile(pBuffer);
		CloseHandle(hMapFile);

		// Check that the .rmskin file is acessible from elevated account
		data.rmskinFile = vec[0];
		if (_waccess(data.rmskinFile.c_str(), 0) == -1)
		{
			std::wstring error = L"Error in elevated process: Unable to access .rmskin file.\n\nMake sure that the .rmskin file saved at a location accessible to all users and try again.";
			MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
			return 1;
		}

		// Assign the tokens to variables
		data.instType = INSTTYPE_ADMIN;
		data.rootLen = _wtoi(vec[1].c_str());
		data.rainmeterPath = vec[2];
		data.backupFolder = vec[3];
		data.addonsList = vec[4];
		data.pluginsList = vec[5];
		data.fontsList = vec[6];
		if (vec[7] == L"1") data.rainmeterFonts = true;

		if (!InstallComponents(&data))
		{
			std::wstring error = L"Error in elevated process: Install failed.";
			MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
			return 1;
		}
		return 0;
	}

	// Check whether Rainstaller.exe is already running and bring it to front if so
	HANDLE hMutex;
	if (IsRunning(L"RainstallerMutex_6D42B76464DA", &hMutex))
	{
		HWND hwnd = FindWindow(L"#32770", L"Rainstaller");
		SetForegroundWindow(hwnd);
		return 0;
	}

	// Avoid loading a dll from current directory
	SetDllDirectory(L"");
	WCHAR buffer[MAX_PATH];

	GetModuleFileName(hInstance, buffer, MAX_PATH);
	exeFile = buffer;

	// Remove the module's name from the path
	WCHAR* pos = wcsrchr(buffer, L'\\');
	if (pos)
	{
		*(pos + 1) = L'\0';
	}

	std::wstring str = buffer;
	str += L"Rainmeter.exe";
	if (_waccess(str.c_str(), 0) != 0)
	{
		std::wstring error = L"Unable to locate Rainmeter.";
		MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
		return 1;
	}

	data.rainmeterPath = buffer;
	str.replace(str.length() - 3, 3, L"ini");

	// Find the settings file and read skins path off it
	if (_waccess(str.c_str(), 0) == 0)
	{
		data.iniPath = buffer;
		if (GetPrivateProfileString(L"Rainmeter", L"SkinPath", L"", buffer, MAX_LINE_LENGTH, str.c_str()) > 0)
		{
			data.skinsPath = buffer;
		}
		else
		{
			data.skinsPath = data.rainmeterPath;
			data.skinsPath += L"Skins\\";
		}
	}
	else
	{
		HRESULT hr = SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buffer);
		wcscat(buffer, L"\\Rainmeter\\");
		data.iniPath = buffer;
		wcscat(buffer, L"Rainmeter.ini");
		if (SUCCEEDED(hr) && _waccess(buffer, 0) == 0)
		{
			if (GetPrivateProfileString(L"Rainmeter", L"SkinPath", L"", buffer, MAX_LINE_LENGTH, buffer) > 0)
			{
				data.skinsPath = buffer;
			}
			else
			{
				std::wstring error = L"SkinPath not found.\nMake sure that Rainmeter has been run at least once.";
				MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
				return 1;
			}
		}
		else
		{
			std::wstring error = L"Rainmeter.ini not found.\nMake sure that Rainmeter has been run at least once.";
			MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
			return 1;
		}
	}

	data.rmskinFile = lpCmdLine;
	if (data.rmskinFile.empty())
	{
		// Show the Open File dialog if no arguments were given
		OPENFILENAME ofn = {0};
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.lpstrFilter = L"Rainmeter skin file (.rmskin)\0*.rmskin;*.zip";
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = buffer;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrTitle = L"Select Rainmeter skin file";
		ofn.lpstrDefExt = L"rmskin";
		ofn.Flags = OFN_FILEMUSTEXIST;

		buffer[0] = L'\0';
		if (!GetOpenFileName((OPENFILENAME*)&ofn)) return 0;	// Abort without warning on cancel
		data.rmskinFile = buffer;
	}
	else if (data.rmskinFile[0] == L'\"')
	{
		// Remove first and last quote if needed
		data.rmskinFile.erase(0, 1);
		data.rmskinFile.resize(data.rmskinFile.length() - 1);
	}

	return ReadArchive() ? DialogBox(hInstance, MAKEINTRESOURCE(IDD_INSTALLER_DIALOG), NULL, (DLGPROC)DlgProc) : 1;
}

/*
** Main dialog window procedure
*/
BOOL CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		InitDialog(hwnd);
		SetForegroundWindow(hwnd);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CANCEL_BUTTON:
			EndDialog(hwnd, 0);
			break;

		case IDC_INSTALL_BUTTON:
			InitInstall(hwnd);
			break;

		case IDC_SKINS_LABEL:
			EnableWindow((HWND)lParam, 0);
			data.skinsList.clear();
			break;

		case IDC_THEMES_LABEL:
			EnableWindow((HWND)lParam, 0);
			data.themesList.clear();
			break;

		case IDC_ADDONS_LABEL:
			EnableWindow((HWND)lParam, 0);
			data.addonsList.clear();
			break;

		case IDC_PLUGINS_LABEL:
			EnableWindow((HWND)lParam, 0);
			data.pluginsList.clear();
			break;

		case IDC_FONTS_LABEL:
			EnableWindow((HWND)lParam, 0);
			data.fontsList.clear();
			break;
		}
		break;

	case WM_TIMER:
		switch (wParam)
		{
		case TIMER_THREAD:
			{
				DWORD exitCode;
				GetExitCodeThread(data.instHandle, &exitCode);
				if (exitCode != STILL_ACTIVE)
				{
					KillTimer(hwnd, TIMER_THREAD);
					CloseHandle(data.instHandle);

					if (exitCode != 0)
					{
						std::wstring error = L"Install thead failed.";
						MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
					}
					else if (data.launchRainmeter)
					{
						LaunchRainmeter();
					}
					EndDialog(hwnd, 0);
				}
			}
			break;

		case TIMER_PROCESS:
			{
				DWORD exitCode;
				GetExitCodeProcess(data.instHandle, &exitCode);
				if (exitCode != STILL_ACTIVE)
				{
					KillTimer(hwnd, TIMER_PROCESS);
					UnmapViewOfFile(pBuffer);
					CloseHandle(hMapFile);
					CloseHandle(data.instHandle);

					if (exitCode == 0)
					{
						if (!data.skinsList.empty() || !data.themesList.empty())
						{
							data.instType = INSTTYPE_NOADMIN;
							data.instHandle = (HANDLE)_beginthreadex(NULL, 0, CreateInstallThread, &data, 0, NULL);
							SetTimer(hwnd, TIMER_THREAD, 100, NULL);
							break;
						}
						else if (data.launchRainmeter)
						{
							LaunchRainmeter();
						}
					}
					EndDialog(hwnd, 0);
				}
			}
			break;
		}
		break;

	case WM_CLOSE:
		// Don't close dialog if install is running
		if (!data.instHandle) EndDialog(hwnd, 0);
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

void InitDialog(HWND hwnd)
{
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_INSTALLER)));
	SendMessage(GetDlgItem(hwnd, IDC_LAUNCH_CHECKBOX), BM_SETCHECK, BST_CHECKED, 0);
	ShowWindow(GetDlgItem(hwnd, IDC_PROGRESSBAR), SW_HIDE);

	if (data.loadTheme.empty())
	{
		ShowWindow(GetDlgItem(hwnd, IDC_THEME_CHECKBOX), SW_HIDE);
	}
	else
	{
		std::wstring text = L"Apply theme (";
		text += data.loadTheme;
		text += L")";
		SendMessage(GetDlgItem(hwnd, IDC_THEME_CHECKBOX), BM_SETCHECK, BST_CHECKED, 0);
		SetDlgItemText(hwnd, IDC_THEME_CHECKBOX, text.c_str());
	}

	if (data.backupFolder.empty())	// Hide the backup footnote if not needed
	{
		RECT dlgRect, sepRect;
		GetWindowRect(hwnd, &dlgRect);
		GetWindowRect(GetDlgItem(hwnd, IDC_SEPERATOR), &sepRect);

		// Width is kept same, the height is cut to the relative y-pos of the sepeator
		SetWindowPos(hwnd, NULL, NULL, NULL, dlgRect.right - dlgRect.left,
											 dlgRect.bottom - dlgRect.top - (dlgRect.bottom - sepRect.bottom) + 1,
											 SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOZORDER);
	}
	else
	{
		HICON hIcon = (HICON)LoadIcon(NULL, IDI_WARNING);
		SendMessage(GetDlgItem(hwnd, IDC_WARNING_ICON), STM_SETICON, (WPARAM)hIcon, 0);
	}

	// Show UAC sheild if needed
	if (!IsCurrentProcessAdmin() && (!data.addonsList.empty() || !data.fontsList.empty() || !data.pluginsList.empty()))
	{
		SendMessage(GetDlgItem(hwnd, IDC_INSTALL_BUTTON), BCM_SETSHIELD, 0, (LPARAM)TRUE);
	}

	// Set custom header
	if (hBitmap)
	{
		SendMessage(GetDlgItem(hwnd, IDC_BITMAP), STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);
	}

	// Get the system font and size (Tahoma/8 on XP, Segoe UI/9 on Vista+)
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(NONCLIENTMETRICS) - sizeof(ncm.iPaddedBorderWidth);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
	HFONT hFont = CreateFontIndirect(&ncm.lfMenuFont);
	ncm.lfMenuFont.lfWeight = FW_BOLD;
	HFONT hFontBold = CreateFontIndirect(&ncm.lfMenuFont);

	SendMessage(GetDlgItem(hwnd, IDC_NAME_LABEL), WM_SETFONT, (WPARAM)hFontBold, 0);
	SendMessage(GetDlgItem(hwnd, IDC_AUTHOR_LABEL), WM_SETFONT, (WPARAM)hFontBold, 0);
	SendMessage(GetDlgItem(hwnd, IDC_VERSION_LABEL), WM_SETFONT, (WPARAM)hFontBold, 0);
	SendMessage(GetDlgItem(hwnd, IDC_SKINS_LABEL), WM_SETFONT, (WPARAM)hFontBold, 0);
	SendMessage(GetDlgItem(hwnd, IDC_THEMES_LABEL), WM_SETFONT, (WPARAM)hFontBold, 0);
	SendMessage(GetDlgItem(hwnd, IDC_ADDONS_LABEL), WM_SETFONT, (WPARAM)hFontBold, 0);
	SendMessage(GetDlgItem(hwnd, IDC_PLUGINS_LABEL), WM_SETFONT, (WPARAM)hFontBold, 0);
	SendMessage(GetDlgItem(hwnd, IDC_FONTS_LABEL), WM_SETFONT, (WPARAM)hFontBold, 0);
	SendMessage(GetDlgItem(hwnd, IDC_NAME_VALUE), WM_SETFONT, (WPARAM)hFont, 0);
	SendMessage(GetDlgItem(hwnd, IDC_AUTHOR_VALUE), WM_SETFONT, (WPARAM)hFont, 0);
	SendMessage(GetDlgItem(hwnd, IDC_VERSION_VALUE), WM_SETFONT, (WPARAM)hFont, 0);
	SendMessage(GetDlgItem(hwnd, IDC_SKINS_VALUE), WM_SETFONT, (WPARAM)hFont, 0);
	SendMessage(GetDlgItem(hwnd, IDC_THEMES_VALUE), WM_SETFONT, (WPARAM)hFont, 0);
	SendMessage(GetDlgItem(hwnd, IDC_ADDONS_VALUE), WM_SETFONT, (WPARAM)hFont, 0);
	SendMessage(GetDlgItem(hwnd, IDC_PLUGINS_VALUE), WM_SETFONT, (WPARAM)hFont, 0);
	SendMessage(GetDlgItem(hwnd, IDC_FONTS_VALUE), WM_SETFONT, (WPARAM)hFont, 0);
	SendMessage(GetDlgItem(hwnd, IDC_THEME_CHECKBOX), WM_SETFONT, (WPARAM)hFont, 0);
	SendMessage(GetDlgItem(hwnd, IDC_LAUNCH_CHECKBOX), WM_SETFONT, (WPARAM)hFont, 0);
	SendMessage(GetDlgItem(hwnd, IDC_INSTALL_BUTTON), WM_SETFONT, (WPARAM)hFont, 0);
	SendMessage(GetDlgItem(hwnd, IDC_CANCEL_BUTTON), WM_SETFONT, (WPARAM)hFont, 0);
	SendMessage(GetDlgItem(hwnd, IDC_BACKUP_LABEL), WM_SETFONT, (WPARAM)hFont, 0);

	SetDlgItemText(hwnd, IDC_NAME_VALUE, data.packageName.c_str());
	SetDlgItemText(hwnd, IDC_AUTHOR_VALUE, data.packageAuthor.c_str());
	SetDlgItemText(hwnd, IDC_VERSION_VALUE, data.packageVersion.c_str());
	data.skinsList.empty() ? EnableWindow(GetDlgItem(hwnd, IDC_SKINS_LABEL), 0) : SetDlgItemText(hwnd, IDC_SKINS_VALUE, data.skinsList.c_str());
	data.themesList.empty() ? EnableWindow(GetDlgItem(hwnd, IDC_THEMES_LABEL), 0) : SetDlgItemText(hwnd, IDC_THEMES_VALUE, data.themesList.c_str());
	data.addonsList.empty() ? EnableWindow(GetDlgItem(hwnd, IDC_ADDONS_LABEL), 0) : SetDlgItemText(hwnd, IDC_ADDONS_VALUE, data.addonsList.c_str());
	data.pluginsList.empty() ? EnableWindow(GetDlgItem(hwnd, IDC_PLUGINS_LABEL), 0) : SetDlgItemText(hwnd, IDC_PLUGINS_VALUE, data.pluginsList.c_str());
	data.fontsList.empty() ? EnableWindow(GetDlgItem(hwnd, IDC_FONTS_LABEL), 0) : SetDlgItemText(hwnd, IDC_FONTS_VALUE, data.fontsList.c_str());

	// Enable tooltips for the component lists
	HWND hwndTT = CreateWindow(TOOLTIPS_CLASS, NULL, WS_POPUP, 0, 0, 0, 0, hwnd, NULL, NULL, 0);
	if (hwndTT)
	{
		SendMessage(hwndTT, TTM_ACTIVATE, TRUE, 0);

		TOOLINFO toolinfo = {0};
		toolinfo.cbSize = sizeof(TOOLINFO);
		toolinfo.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
		toolinfo.hwnd = hwnd;
		toolinfo.hinst = NULL;

		toolinfo.uId = (LPARAM)GetDlgItem(hwnd, IDC_SKINS_VALUE);
		toolinfo.lpszText = (LPTSTR)data.skinsList.c_str();
		SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)&toolinfo);

		toolinfo.uId = (LPARAM)GetDlgItem(hwnd, IDC_THEMES_VALUE);
		toolinfo.lpszText = (LPTSTR)data.themesList.c_str();
		SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)&toolinfo);

		toolinfo.uId = (LPARAM)GetDlgItem(hwnd, IDC_ADDONS_VALUE);
		toolinfo.lpszText = (LPTSTR)data.addonsList.c_str();
		SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)&toolinfo);

		toolinfo.uId = (LPARAM)GetDlgItem(hwnd, IDC_PLUGINS_VALUE);
		toolinfo.lpszText = (LPTSTR)data.pluginsList.c_str();
		SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)&toolinfo);

		toolinfo.uId = (LPARAM)GetDlgItem(hwnd, IDC_FONTS_VALUE);
		toolinfo.lpszText = (LPTSTR)data.fontsList.c_str();
		SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)&toolinfo);
	}
}

void InitInstall(HWND hwnd)
{
	// Disable the X button
	HMENU hMenu = GetSystemMenu(hwnd, false);
	RemoveMenu(hMenu, GetMenuItemCount(hMenu) - 1, MF_DISABLED | MF_BYPOSITION);

	EnableWindow(GetDlgItem(hwnd, IDC_CANCEL_BUTTON), 0);
	EnableWindow(GetDlgItem(hwnd, IDC_INSTALL_BUTTON), 0);
	EnableWindow(GetDlgItem(hwnd, IDC_THEME_CHECKBOX), 0);
	EnableWindow(GetDlgItem(hwnd, IDC_LAUNCH_CHECKBOX), 0);

	if (SendMessage(GetDlgItem(hwnd, IDC_LAUNCH_CHECKBOX), BM_GETCHECK, 0, 0) == BST_CHECKED) data.launchRainmeter = true;

	if (IsCurrentProcessAdmin())
	{
		data.instType = INSTTYPE_FULL;
		data.instHandle = (HANDLE)_beginthreadex(NULL, 0, CreateInstallThread, &data, 0, NULL);
		SetTimer(hwnd, TIMER_THREAD, 250, NULL);
	}
	else
	{
		if (!data.addonsList.empty() || !data.pluginsList.empty() || !data.fontsList.empty())
		{
			data.instHandle = CreateProcessElevated(hwnd);
			if (data.instHandle == NULL)
			{
				std::wstring error = L"Failed to create elevated process.";
				MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
				UnmapViewOfFile(pBuffer);
				CloseHandle(hMapFile);
				EndDialog(hwnd, 0);
			}
			SetTimer(hwnd, TIMER_PROCESS, 250, NULL);
		}
		else if (!data.skinsList.empty() || !data.themesList.empty())
		{
			data.instType = INSTTYPE_NOADMIN;
			data.instHandle = (HANDLE)_beginthreadex(NULL, 0, CreateInstallThread, &data, 0, NULL);
			SetTimer(hwnd, TIMER_THREAD, 250, NULL);
		}
	}

	// Hide the backup text if necessary and show the progress bar
	ShowWindow(GetDlgItem(hwnd, IDC_PROGRESSBAR), SW_SHOWNORMAL);
	SendMessage(GetDlgItem(hwnd, IDC_PROGRESSBAR), PBM_SETMARQUEE, (WPARAM)true, IsAboveVista() ? 0 : 50);
	SendMessage(GetDlgItem(hwnd, IDC_INSTALL_BUTTON), BCM_SETSHIELD, 0, (LPARAM)FALSE);
	SetDlgItemText(hwnd, IDC_INSTALL_BUTTON, L"Installing..");

	// Clear loadTheme if checkbox unchecked
	if (SendMessage(GetDlgItem(hwnd, IDC_THEME_CHECKBOX), BM_GETCHECK, 0, 0) != BST_CHECKED) data.loadTheme.clear();
}

/*
** Run As dialog window prodcedure(for Vista/7 with UAC disabled)
*/
BOOL CALLBACK RunAsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Based on the work of Anders Kjersem
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			HMODULE hDLL = LoadLibrary(L"shell32.dll");
			HICON hIcon = (HICON)LoadImage(hDLL, MAKEINTRESOURCE(220), IMAGE_ICON, 16, 16, LR_SHARED);
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			hIcon = (HICON)LoadImage(hDLL, MAKEINTRESOURCE(194), IMAGE_ICON, 32, 32, LR_SHARED);
			SendMessage(GetDlgItem(hwnd, IDC_KEY_ICON), STM_SETICON, (WPARAM)hIcon, 0);
			FreeLibrary(hDLL);

			SendMessage(GetDlgItem(hwnd, IDC_USERNAME), EM_LIMITTEXT, (WPARAM)256, 0);
			SendMessage(GetDlgItem(hwnd, IDC_PASSWORD), EM_LIMITTEXT, (WPARAM)256, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SPECIFIED_BUTTON), BM_SETCHECK, BST_CHECKED, 0);
		}
		return TRUE;

	case WM_CLOSE:
		return EndDialog(hwnd, 1);

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				WCHAR wszPwd[256];
				WCHAR wszUser[256];
				PROCESS_INFORMATION pi = {0};
				STARTUPINFO si = {sizeof(STARTUPINFO)};
				SendMessage(GetDlgItem(hwnd,IDC_USERNAME), WM_GETTEXT, (WPARAM)256, (LPARAM)wszUser);
				SendMessage(GetDlgItem(hwnd,IDC_PASSWORD), WM_GETTEXT, (WPARAM)256, (LPARAM)wszPwd);

				WCHAR arg[MAX_PATH];
				_snwprintf_s(arg, _TRUNCATE, L"\"%s\" /ELEVATE", exeFile.c_str());
				if (!CreateProcessWithLogonW(wszUser, 0, wszPwd, LOGON_WITH_PROFILE, NULL, arg, 0, 0, 0, &si, &pi))
				{
					std::wstring error = L"Unable to logon with given creditials.\nPlease ensure that both the username and password are correct.";
					MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
				}
				else
				{
					if (pi.hProcess)
					{
						data.instHandle = pi.hProcess;
						EndDialog(hwnd, 0);
					}
					else
					{
						EndDialog(hwnd, 1);
					}
				}
			}
			break;

		case IDCANCEL:
			EndDialog(hwnd, 1);
			break;
		}
		break;
	}
	return FALSE;
}

/*
** Go through archive, list contents, and read settings
*/
bool ReadArchive()
{
	unzFile ufile = unzOpen(ConvertToAscii(data.rmskinFile.c_str()).c_str());
	if (!ufile)
	{
		std::wstring error = L"The specified file is not a valid archive:\n";
		error += data.rmskinFile;
		MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
		return false;
	}

	// Get temp file path for the configuration file/bitmap
	WCHAR tempFile[MAX_PATH];
	ExpandEnvironmentStrings(L"%TEMP%\\~RainstallerData.TMP", tempFile, MAX_PATH);

	char cBuffer[MAX_PATH * 3];
	WCHAR buffer[MAX_PATH];
	unz_file_info ufi;

	// Loop through the contents of the archive until the settings file is found
	do
	{
		if (unzGetCurrentFileInfo(ufile, &ufi, cBuffer, MAX_PATH * 3, NULL, 0, NULL, 0) != UNZ_OK) break;
		MultiByteToWideChar(CP_ACP, 0, cBuffer, strlen(cBuffer) + 1, buffer, MAX_PATH);
		while (WCHAR* pos = wcschr(buffer, L'\\')) *pos = L'/';
		WCHAR* fileName = wcsrchr(buffer, L'/');

		if (!fileName)
		{
			fileName = buffer;
		}
		else
		{
			++fileName;	// Get rid of leading slash
		}

		if (_wcsicmp(fileName, L"RMSKIN.ini") == 0 || _wcsicmp(fileName, L"Rainstaller.cfg") == 0)
		{
			if (!ExtractCurrentFile(ufile, tempFile)) return false;

			// The number of characters in path before the configuration file
			data.rootLen = wcslen(buffer) - wcslen(fileName);
			break;
		}
		else if (_wcsicmp(fileName, L"RMSKIN.bmp") == 0 || _wcsicmp(fileName, L"Rainstaller.bmp") == 0)
		{
			if (!ExtractCurrentFile(ufile, tempFile)) return false;
			hBitmap = (HBITMAP)LoadImage(NULL, tempFile, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		}
	} while (unzGoToNextFile(ufile) == UNZ_OK);

	if (!ReadSettings(tempFile))
	{
		unzClose(ufile);
		std::wstring error = L"Invalid settings file in:\n";
		error += data.rmskinFile;
		MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
		return false;
	}

	bool backupRequired = false;
	WCHAR* filePath = buffer;
	filePath += data.rootLen;
	WCHAR* filePath6(filePath), * filePath7(filePath), * filePath14(filePath);
	filePath6 += 6;		// Relative to path to the beginning of the skin/font root
	filePath7 += 7;		// Relative to path to the beginning of the theme/addon root
	filePath14 += 14;	// Relative to path to the beginning of the plugin root

	std::wstring themesPath(data.iniPath), addonsPath(data.rainmeterPath), pluginsPath(data.rainmeterPath);
	themesPath += L"Themes\\";
	addonsPath += L"Addons\\";
	pluginsPath += L"Plugins\\";

	// Loop through the archive a second time now that we know the where the contents are. Ugly code ahead!
	unzGoToFirstFile(ufile);
	do
	{
		if (unzGetCurrentFileInfo(ufile, &ufi, cBuffer, MAX_PATH * 3, NULL, 0, NULL, 0) != UNZ_OK) break;
		MultiByteToWideChar(CP_ACP, 0, cBuffer, strlen(cBuffer) + 1, buffer, MAX_PATH);
		while (WCHAR* pos = wcschr(buffer, L'\\')) *pos = L'/';

		if (_wcsnicmp(filePath, L"Skins/", 6) == 0)
		{
			WCHAR* pos = wcschr(filePath6, L'/');
			if (pos)
			{
				pos[0] = L'\0';
				std::wstring name(filePath6), root(filePath);
				root += L"/";
				int len = root.length();

				// Loop until we get to another skin (or another component altogether)
				while (unzGoToNextFile(ufile) == UNZ_OK)
				{
					unzGoToNextFile(ufile);
					unzGetCurrentFileInfo(ufile, &ufi, cBuffer, MAX_PATH * 3, NULL, 0, NULL, 0);
					MultiByteToWideChar(CP_ACP, 0, cBuffer, strlen(cBuffer) + 1, buffer, MAX_PATH);
					while (WCHAR* pos = wcschr(buffer, L'\\')) *pos = L'/';
					if (_wcsnicmp(filePath, root.c_str(), len) != 0) break;
				}
				
				if (_wcsicmp(name.c_str(), L"Backup") != 0)
				{
					// Add the folder to list
					if (!data.skinsList.empty()) data.skinsList += L" | ";
					data.skinsList += name;
					name.insert(0, data.skinsPath);
					if (_waccess(name.c_str(), 0) == 0)
					{
						data.skinsList += L"*";
						backupRequired = true;
					}
				}
				continue;
			}
		}
		else if (_wcsnicmp(filePath, L"Themes/", 7) == 0)
		{
			WCHAR* pos = wcschr(filePath7, L'/');
			if (pos)
			{
				pos[0] = L'\0';
				std::wstring name(filePath7), root(filePath);
				root += L"/";
				int len = root.length();

				while (unzGoToNextFile(ufile) == UNZ_OK)
				{
					unzGetCurrentFileInfo(ufile, &ufi, cBuffer, MAX_PATH * 3, NULL, 0, NULL, 0);
					MultiByteToWideChar(CP_ACP, 0, cBuffer, strlen(cBuffer) + 1, buffer, MAX_PATH);
					while (WCHAR* pos = wcschr(buffer, L'\\')) *pos = L'/';
					if (_wcsnicmp(filePath, root.c_str(), len) != 0) break;
				}

				if (_wcsicmp(name.c_str(), L"Backup") != 0)
				{
					if (!data.themesList.empty()) data.themesList += L" | ";
					data.themesList += name;
					name.insert(0, themesPath);
					if (_waccess(name.c_str(), 0) == 0)
					{
						data.themesList += L"*";
						backupRequired = true;
					}
				}
				continue;
			}
		}
		else if (_wcsnicmp(filePath, L"Addons/", 7) == 0)
		{
			WCHAR* pos = wcschr(filePath7, L'/');
			if (pos)
			{
				pos[0] = L'\0';
				std::wstring name(filePath7), root(filePath);
				root += L"/";
				int len = root.length();

				while (unzGoToNextFile(ufile) == UNZ_OK)
				{
					unzGetCurrentFileInfo(ufile, &ufi, cBuffer, MAX_PATH * 3, NULL, 0, NULL, 0);
					MultiByteToWideChar(CP_ACP, 0, cBuffer, strlen(cBuffer) + 1, buffer, MAX_PATH);
					while (WCHAR* pos = wcschr(buffer, L'\\')) *pos = L'/';
					if (_wcsnicmp(filePath, root.c_str(), len) != 0) break;
				}

				if (_wcsicmp(name.c_str(), L"Backup") != 0 && !IsDefaultAddon(name.c_str()))
				{
					if (!data.addonsList.empty()) data.addonsList += L" | ";
					data.addonsList += name;
					name.insert(0, addonsPath);
					if (_waccess(name.c_str(), 0) == 0)
					{
						data.addonsList += L"*";
						backupRequired = true;
					}
				}
				continue;
			}
		}
		else if (_wcsnicmp(filePath, L"Fonts/", 6) == 0 && filePath6[0] != L'\0' &&
				 !wcschr(filePath14, L'/') &&
				 _wcsicmp(wcschr(filePath6, L'.'), L".ttf") == 0)
		{
			if (!data.fontsList.empty()) data.fontsList += L" | ";
			data.fontsList += filePath6;
		}
		else if (_wcsnicmp(filePath, PLUGINS_ROOT, 14) == 0 &&
				 filePath14[0] != L'\0' &&
				 !wcschr(filePath14, L'/') &&
				 _wcsicmp(wcschr(filePath14, L'.'), L".dll") == 0
				 && !IsDefaultPlugin(filePath14))
		{
			if (!data.pluginsList.empty()) data.pluginsList += L" | ";

			std::wstring name = filePath14;
			data.pluginsList += name;
			name.insert(0, pluginsPath);
			if (_waccess(name.c_str(), 0) == 0)
			{
				backupRequired = true;
				data.pluginsList += L"*";
			}
		}
	} while (unzGoToNextFile(ufile) == UNZ_OK);

	unzClose(ufile);
	DeleteFile(tempFile);

	if (!data.skinsList.empty() || !data.themesList.empty() || !data.fontsList.empty() || !data.addonsList.empty() || !data.pluginsList.empty())
	{
		if (backupRequired)	// Get current time as the name for backup folders
		{
			SYSTEMTIME lt;
			GetLocalTime(&lt);
			_snwprintf_s(buffer, _TRUNCATE, L"%02d.%02d.%02d %02d.%02d-%02d", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond);
			data.backupFolder = buffer;
		}
		return true;
	}
	else
	{
		std::wstring error = L"The specified file did not contain any components to install:\n";
		error += data.rmskinFile;
		MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
		return false;
	}
}

bool ExtractCurrentFile(unzFile& ufile, LPCTSTR fileName)
{
	if (fileName[wcslen(fileName) - 1] == L'/')
	{
		CreateDirectory(fileName, 0);
		return true;
	}

	HANDLE hFile = CreateFile(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		// Some zip files don't list directories, so try creating all directories first
		std::wstring path = fileName;
		std::wstring::size_type pos = 0;
		while (true)
		{
			pos = path.find_first_of(L"/", pos);
			if (pos == std::wstring::npos) break;
			++pos;

			std::wstring dir = path.substr(0, pos);
			if (_waccess(dir.c_str(), 0) == -1) CreateDirectory(dir.c_str(), 0);
		}

		hFile = CreateFile(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			// Creating the directories didn't help..
			std::wstring error = L"Unable to create file: ";
			error += fileName;
			MessageBox(NULL, error.c_str(), APP_NAME, MB_OK);
			return false;
		}
	}

	bool ret = true;
	int res = unzOpenCurrentFile(ufile);
	if (res != UNZ_OK)
	{
		std::wstring error = L"Unable to open file for extraction: ";
		error += fileName;
		MessageBox(NULL, error.c_str(), APP_NAME, MB_OK);
		ret = false;
	}
	else
	{
		do
		{
			char buffer[16384];
			DWORD dwWritten;
			res = unzReadCurrentFile(ufile, buffer, 16384);

			if (res < 0	|| (res > 0 && !WriteFile(hFile, (LPCVOID)buffer, res, &dwWritten, NULL)))
			{
				std::wstring error = L"Unable to write file: ";
				error += fileName;
				MessageBox(NULL, error.c_str(), APP_NAME, MB_OK);
				ret = false;
				break;
			}
		} while (res > 0);

		if (unzCloseCurrentFile(ufile) == UNZ_CRCERROR)
		{
			std::wstring error = L"Checksum error (archive is damaged) with file: ";
			error += fileName;
			MessageBox(NULL, error.c_str(), APP_NAME, MB_OK);
			ret = false;
		}
	}

	if (!ret) unzClose(ufile);
	CloseHandle(hFile);
	return ret;
}

bool ReadSettings(LPCTSTR filePath)
{
	std::wstring rainmeterFile = data.rainmeterPath;
	rainmeterFile += L"Rainmeter.exe";
	std::wstring rainmeterVersion = GetFileVersion(rainmeterFile);

	WCHAR buffer[MAX_LINE_LENGTH];

	if (GetPrivateProfileString(L"Rainstaller", L"Name", L"", buffer, 64, filePath) > 0)
	{
		// Old, pre-2.0 format
		data.packageName = buffer;

		GetPrivateProfileString(L"Rainstaller", L"Author", L"", buffer, 64, filePath);
		data.packageAuthor = buffer;

		GetPrivateProfileString(L"Rainstaller", L"Version", L"", buffer, 64, filePath);
		data.packageVersion = buffer;

		if ((GetPrivateProfileString(L"Rainstaller", L"LaunchType", L"", buffer, MAX_LINE_LENGTH, filePath) > 0))
		{
			if (_wcsicmp(buffer, L"load") == 0)
			{
				GetPrivateProfileString(L"Rainstaller", L"LaunchCommand", L"", buffer, MAX_LINE_LENGTH, filePath);
				data.loadSkins = buffer;
			}
			else
			{
				GetPrivateProfileString(L"Rainstaller", L"LaunchCommand", L"", buffer, MAX_LINE_LENGTH, filePath);
				data.loadTheme = buffer;
			}
		}
		if (GetPrivateProfileString(L"Rainstaller", L"RainmeterFonts", L"", buffer, MAX_LINE_LENGTH, filePath) > 0)
		{
			data.rainmeterFonts = (buffer[0] == L'1') ? true : false;
		}
		if (GetPrivateProfileString(L"Rainstaller", L"Merge", L"", buffer, MAX_LINE_LENGTH, filePath) > 0)
		{
			data.mergeSkins = (buffer[0] == L'1' || buffer[0] == L'2') ? true : false;
		}
		if (GetPrivateProfileString(L"Rainstaller", L"KeepVar", L"", buffer, MAX_LINE_LENGTH, filePath) > 0)
		{
			data.keepVariables = buffer;
		}
		if ((GetPrivateProfileString(L"Rainstaller", L"MinRainmeterVer", L"", buffer, MAX_LINE_LENGTH, filePath) > 0)
			&& (CompareVersions(buffer, rainmeterVersion) == 1))
		{
			std::wstring error = L"This package requires Rainmeter version ";
			error += buffer;
			error += L" or higher.\nDownload the latest version at rainmeter.net and try again.";
			MessageBox(NULL, error.c_str(), APP_NAME, MB_OK | MB_ICONERROR);
			return false;
		}

		return true;
	}
	else if (GetPrivateProfileString(L"RMSKIN", L"PackageName", L"", buffer, 64, filePath) > 0)
	{
		// New format
		data.packageName = buffer;

		GetPrivateProfileString(L"RMSKIN", L"PackageAuthor", L"", buffer, 64, filePath);
		data.packageAuthor = buffer;

		GetPrivateProfileString(L"RMSKIN", L"PackageVersion", L"", buffer, 64, filePath);
		data.packageVersion = buffer;

		if (GetPrivateProfileString(L"RMSKIN", L"LoadTheme", L"", buffer, MAX_LINE_LENGTH, filePath) > 0)
		{
			data.loadTheme = buffer;
		}
		if (GetPrivateProfileString(L"RMSKIN", L"LoadSkins", L"", buffer, MAX_LINE_LENGTH, filePath) > 0)
		{
			data.loadSkins = buffer;
		}
		if (GetPrivateProfileString(L"RMSKIN", L"FontDirectory", L"", buffer, MAX_LINE_LENGTH, filePath) > 0)
		{
			data.rainmeterFonts = (_wcsicmp(buffer, L"rainmeter") == 0) ? true : false;
		}
		if (GetPrivateProfileString(L"RMSKIN", L"MergeSkins", L"", buffer, MAX_LINE_LENGTH, filePath) > 0)
		{
			data.mergeSkins = (buffer[0] == L'1') ? true : false;
		}
		if (GetPrivateProfileString(L"RMSKIN", L"KeepVariables", L"", buffer, MAX_LINE_LENGTH, filePath) > 0)
		{
			data.keepVariables = buffer;
		}
		if ((GetPrivateProfileString(L"RMSKIN", L"MinimumRainmeter", L"", buffer, MAX_LINE_LENGTH, filePath) > 0)
			&& (CompareVersions(buffer, rainmeterVersion) == 1))
		{
			std::wstring error = L"This package requires Rainmeter version ";
			error += buffer;
			error += L" or higher.\nDownload the latest version at rainmeter.net and try again.";
			MessageBox(NULL, error.c_str(), APP_NAME, MB_OK | MB_ICONERROR);
			return false;
		}
		if ((GetPrivateProfileString(L"RMSKIN", L"MinimumDotNET", L"", buffer, MAX_LINE_LENGTH, filePath) > 0)
			&& (CompareVersions(buffer, GetDotNETVersion()) == 1))
		{
			std::wstring error = L"This packages requires .NET framework version ";
			error += buffer;
			error += L" or higher.\nEnsure that you have the required .NET framework and try again.";
			MessageBox(NULL, error.c_str(), APP_NAME, MB_OK | MB_ICONERROR);
			return false;
		}
		if ((GetPrivateProfileString(L"RMSKIN", L"MinimumWindows", L"", buffer, MAX_LINE_LENGTH, filePath) > 0)
			&& (CompareVersions(buffer, GetWindowsVersion()) == 1))
		{
			std::wstring error = L"This package requires Windows ";
			error += buffer;
			error += L" or higher.\nContact the package author for more information.";
			MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
			return false;
		}

		return true;
	}
	else
	{
		return false;
	}
}

bool IsDefaultPlugin(LPCTSTR plugin)
{
	return (_wcsicmp(plugin, L"AdvancedCPU.dll") == 0 ||
			_wcsicmp(plugin, L"CoreTemp.dll") == 0 ||
			_wcsicmp(plugin, L"FolderInfo.dll") == 0 ||
			_wcsicmp(plugin, L"InputText.dll") == 0 ||
			_wcsicmp(plugin, L"iTunesPlugin.dll") == 0 ||
			_wcsicmp(plugin, L"MediaKey.dll") == 0 ||
			_wcsicmp(plugin, L"NowPlaying.dll") == 0 ||
			_wcsicmp(plugin, L"PerfMon.dll") == 0 ||
			_wcsicmp(plugin, L"PingPlugin.dll") == 0 ||
			_wcsicmp(plugin, L"PowerPlugin.dll") == 0 ||
			_wcsicmp(plugin, L"QuotePlugin.dll") == 0 ||
			_wcsicmp(plugin, L"RecycleManager.dll") == 0 ||
			_wcsicmp(plugin, L"ResMon.dll") == 0 ||
			_wcsicmp(plugin, L"SpeedFanPlugin.dll") == 0 ||
			_wcsicmp(plugin, L"SysInfo.dll") == 0 ||
			_wcsicmp(plugin, L"VirtualDesktops.dll") == 0 ||
			_wcsicmp(plugin, L"WebParser.dll") == 0 ||
			_wcsicmp(plugin, L"WifiStatus.dll") == 0 ||
			_wcsicmp(plugin, L"Win7AudioPlugin.dll") == 0 ||
			_wcsicmp(plugin, L"WindowMessagePlugin.dll") == 0) ? true : false;
}

bool IsDefaultAddon(LPCTSTR addon)
{
	return (_wcsnicmp(addon, L"RainBackup", 10) == 0 ||
			_wcsnicmp(addon, L"Rainstaller", 11) == 0) ? true : false;
}

/*
** Backup and install components
*/
bool InstallComponents(RMSKIN_DATA* data)
{
	if (!CloseRainmeterIfActive())
	{
		std::wstring error = L"Failed to close Rainmeter.";
		MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
		return false;
	}

	int result;
	char cBuffer[MAX_PATH * 3];
	WCHAR buffer[MAX_PATH];
	unzFile ufile = unzOpen(ConvertToAscii(data->rmskinFile.c_str()).c_str());
	unz_file_info ufi;
	WCHAR* filePath = buffer;
	filePath += data->rootLen;

	// Loop through the archive and install stuff. Get ready to scratch your head.
	do
	{
		// Assume no errors as we've already looped through archive
		unzGetCurrentFileInfo(ufile, &ufi, cBuffer, MAX_PATH * 3, NULL, 0, NULL, 0);
		MultiByteToWideChar(CP_ACP, 0, cBuffer, strlen(cBuffer) + 1, buffer, MAX_PATH);
		while (WCHAR* pos = wcschr(buffer, L'\\')) *pos = L'/';

		if (data->instType & INSTTYPE_NOADMIN)
		{
			if (_wcsnicmp(filePath, L"Skins/", 6) == 0 && wcslen(filePath) > 6 && !data->skinsList.empty())
			{
				WCHAR* fileName = filePath;
				fileName += 6;
				SetCurrentDirectory(data->skinsPath.c_str());
				if (!data->mergeSkins && !BackupComponent(data->backupFolder, data->skinsList, data->skinsPath)) break;

				// Loop and extract everything in Skins/
				do
				{
					if (!ExtractCurrentFile(ufile, fileName)) return false;
					result = unzGoToNextFile(ufile);
					unzGetCurrentFileInfo(ufile, &ufi, cBuffer, MAX_PATH * 3, NULL, 0, NULL, 0);
					MultiByteToWideChar(CP_ACP, 0, cBuffer, strlen(cBuffer) + 1, buffer, MAX_PATH);
					while (WCHAR* pos = wcschr(buffer, L'\\')) *pos = L'/';
				} while (result == UNZ_OK && _wcsnicmp(filePath, L"Skins/", 6) == 0);

				if (!data->keepVariables.empty()) KeepVariables(data->backupFolder, data->skinsPath, data->keepVariables);
				continue;	// To skip the unzGotoNextFile at end of loop
			}
			else if (_wcsnicmp(filePath, L"Themes/", 7) == 0 && wcslen(filePath) > 7 && !data->themesList.empty())
			{
				std::wstring themesPath = data->iniPath;
				themesPath += L"Themes\\";
				SetCurrentDirectory(data->iniPath.c_str());
				if (!BackupComponent(data->backupFolder, data->themesList, themesPath)) break;

				do
				{
					if (!ExtractCurrentFile(ufile, filePath)) return false;

					if (wcsstr(filePath, L"Rainmeter.thm"))	// Remove per-user values from Rainmeter.thm
					{
						WritePrivateProfileString(L"Rainmeter", L"SkinPath", NULL, filePath);
						WritePrivateProfileString(L"Rainmeter", L"ConfigEditor", NULL, filePath);
						WritePrivateProfileString(L"Rainmeter", L"DisableDragging", NULL, filePath);
						WritePrivateProfileString(L"Rainmeter", L"DisableRDP", NULL, filePath);
						WritePrivateProfileString(L"Rainmeter", L"DisableVersionCheck", NULL, filePath);
						WritePrivateProfileString(L"Rainmeter", L"Logging", NULL, filePath);
						WritePrivateProfileString(L"illustro", L"Version", NULL, filePath);
					}

					result = unzGoToNextFile(ufile);
					unzGetCurrentFileInfo(ufile, &ufi, cBuffer, MAX_PATH * 3, NULL, 0, NULL, 0);
					MultiByteToWideChar(CP_ACP, 0, cBuffer, strlen(cBuffer) + 1, buffer, MAX_PATH);
					while (WCHAR* pos = wcschr(buffer, L'\\')) *pos = L'/';
				} while (result == UNZ_OK && _wcsnicmp(filePath, L"Themes/", 7) == 0);
				continue;
			}
		}

		if (data->instType & INSTTYPE_ADMIN)
		{
			if (_wcsnicmp(filePath, L"Addons/", 7) == 0 && wcslen(filePath) > 7 && !data->addonsList.empty())
			{
				WCHAR* fileName = filePath;
				fileName += 7;

				std::wstring addonsPath = data->rainmeterPath;
				addonsPath += L"Addons\\";
				SetCurrentDirectory(data->rainmeterPath.c_str());
				if (!BackupComponent(data->backupFolder, data->addonsList, addonsPath)) break;

				do
				{
					if (!IsDefaultAddon(fileName))
					{
						if (!ExtractCurrentFile(ufile, filePath)) return false;
					}

					result = unzGoToNextFile(ufile);
					unzGetCurrentFileInfo(ufile, &ufi, cBuffer, MAX_PATH * 3, NULL, 0, NULL, 0);
				} while (result == UNZ_OK && _wcsnicmp(filePath, L"Addons/", 7) == 0);
				continue;
			}
			else if (_wcsnicmp(filePath, L"Fonts/", 6) == 0 && wcslen(filePath) > 6 && !data->fontsList.empty())
			{
				std::wstring fontsPath;
				HMODULE hDLL;
				GETFONTRESOURCEINFO GetFontResourceInfo;
				HKEY hKey;

				if (data->rainmeterFonts)
				{
					fontsPath = data->rainmeterPath;
					fontsPath += L"Fonts\\";
					CreateDirectory(fontsPath.c_str(), NULL);
				}
				else
				{
					WCHAR buffer[MAX_PATH];
					if (!SHGetSpecialFolderPath(NULL, buffer, CSIDL_FONTS, FALSE)) break;
					fontsPath = buffer;
					fontsPath += L"\\";

					// Undocumented API
					hDLL = LoadLibrary(L"gdi32.dll");
					GetFontResourceInfo = (GETFONTRESOURCEINFO)GetProcAddress(hDLL, "GetFontResourceInfoW");

					if (!GetFontResourceInfo ||
						RegOpenKeyEx(HKEY_LOCAL_MACHINE,
									 L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts",
									 0,
									 KEY_READ | KEY_WRITE,
									 &hKey) != ERROR_SUCCESS)
					{
						std::wstring error = L"Failed to access GetFontResourceInfo.";
						MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
						FreeLibrary(hDLL);
						break;
					}
				}

				SetCurrentDirectory(fontsPath.c_str());
				WCHAR* fileName = filePath;
				fileName += 6;

				do
				{
					if (!wcschr(fileName, L'/') && _wcsicmp(wcschr(fileName, L'.'), L".ttf") == 0 && _waccess(fileName, 0) == -1)
					{
						if (!ExtractCurrentFile(ufile, fileName)) return false;

						if (!data->rainmeterFonts)
						{
							// Write the font information to registry so that it will be enumerated by Rainmeter
							DWORD dwSize = MAX_PATH;
							WCHAR buffer[MAX_PATH];
							AddFontResource(fileName);
							if (GetFontResourceInfo(fileName, &dwSize, &buffer, 1) != 0)
							{
								wcscat(buffer, L" (TrueType)");
								if (RegQueryValueEx(hKey, buffer, NULL, NULL, NULL, NULL) == ERROR_FILE_NOT_FOUND)
								{
									RegSetValueEx(hKey, buffer, 0, REG_SZ, (PBYTE)fileName, ((DWORD)wcslen(fileName) + 1) * sizeof(WCHAR));
								}
								else
								{
									// Font is already installed with a different filename
									RemoveFontResource(fileName);
									DeleteFile(fileName);
								}
							}
						}
					}

					result = unzGoToNextFile(ufile);
					unzGetCurrentFileInfo(ufile, &ufi, cBuffer, MAX_PATH * 3, NULL, 0, NULL, 0);
					MultiByteToWideChar(CP_ACP, 0, cBuffer, strlen(cBuffer) + 1, buffer, MAX_PATH);
					while (WCHAR* pos = wcschr(buffer, L'\\')) *pos = L'/';
				} while (result == UNZ_OK && _wcsnicmp(filePath, L"Fonts/", 6) == 0);

				FreeLibrary(hDLL);
				if (!data->rainmeterFonts) PostMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
				continue;
			}
			else if (_wcsnicmp(filePath, PLUGINS_ROOT, 14) == 0 && wcslen(filePath) > 14 && !data->pluginsList.empty())
			{
				std::wstring pluginsPath = data->rainmeterPath;
				pluginsPath += L"Plugins\\";
				SetCurrentDirectory(pluginsPath.c_str());
				WCHAR* fileName = filePath;
				fileName += 14;

				if (!BackupComponent(data->backupFolder, data->pluginsList, pluginsPath)) break;

				do
				{
					if (!wcschr(fileName, L'/') && _wcsicmp(wcschr(fileName, L'.'), L".dll") == 0 && !IsDefaultPlugin(fileName))
					{
						if (!ExtractCurrentFile(ufile, fileName)) return false;
					}

					result = unzGoToNextFile(ufile);
					unzGetCurrentFileInfo(ufile, &ufi, cBuffer, MAX_PATH * 3, NULL, 0, NULL, 0);
					MultiByteToWideChar(CP_ACP, 0, cBuffer, strlen(cBuffer) + 1, buffer, MAX_PATH);
				} while (result == UNZ_OK && _wcsnicmp(filePath, PLUGINS_ROOT, 14) == 0);
				continue;
			}
		}

		result = unzGoToNextFile(ufile);
	} while (result == UNZ_OK);

	unzClose(ufile);
	return (result == UNZ_END_OF_LIST_OF_FILE) ? true : false;
}

bool BackupComponent(const std::wstring& backupFolder, const std::wstring& list, const std::wstring& path)
{
	if (!list.empty() && (list.find(L"*") != std::wstring::npos))
	{
		std::vector<std::wstring> vec = Tokenize(list, L"|");
		std::vector<std::wstring>::size_type vecSize = vec.size();

		std::wstring tmpFrom(path), tmpTo(path);
		tmpTo += L"Backup\\";
		if (_waccess(tmpTo.c_str(), 0) == -1) CreateDirectory(tmpTo.c_str(), NULL);

		tmpTo += backupFolder;
		CreateDirectory(tmpTo.c_str(), NULL);

		for (unsigned int i = 0; i < vecSize; ++i)
		{
			if (vec[i][vec[i].length() - 1] == L'*')
			{
				vec[i].resize(vec[i].length() - 1);	// Get rid of trailing asterisk
				tmpFrom += vec[i];

				if (!CopyFiles(tmpFrom.c_str(), tmpTo.c_str(), true))
				{
					std::wstring error = L"Unable to backup from:\n";
					error += tmpFrom;
					error += L"\n\nTo:\n";
					error += tmpTo;
					MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
					return false;
				}

				tmpFrom.resize(tmpFrom.length() - vec[i].length());
			}
		}
	}

	return true;
}

void KeepVariables(const std::wstring& backupFolder, const std::wstring& skinsPath, const std::wstring& fileList)
{
	WCHAR keyname[32767];	// Max size returned by GetPrivateProfileSection
	WCHAR buffer[4];
	std::wstring currKey, currValue;
	std::vector<std::wstring> vec = Tokenize(fileList, L"|");

	for (unsigned int i = 0, max = vec.size(); i < max; ++i)
	{
		std::wstring fromPath = skinsPath;
		fromPath += L"Backup\\";
		fromPath += backupFolder;
		fromPath += L"\\";
		fromPath += vec[i];
		std::wstring toPath = skinsPath;
		toPath += vec[i];

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

void PreserveSetting(std::wstring& iniFile, std::wstring& backupFile, LPCTSTR section, LPCTSTR key)
{
	WCHAR buffer[MAX_LINE_LENGTH];
	if (GetPrivateProfileString(section, key, L"", buffer, MAX_LINE_LENGTH, iniFile.c_str()) == 0 &&
		GetPrivateProfileString(section, key, L"", buffer, MAX_LINE_LENGTH, backupFile.c_str()) > 0)
	{
		WritePrivateProfileString(section, key, buffer, iniFile.c_str());
	}
}

void LaunchRainmeter()
{
	SetCurrentDirectory(data.rainmeterPath.c_str());

	// Take a copy of current Rainmeter.ini before doing anything
	std::wstring iniFile = data.iniPath;
	iniFile += L"Rainmeter.ini";
	std::wstring backupFile = data.iniPath;
	backupFile += L"Themes\\Backup\\";
	CreateDirectory(backupFile.c_str(), NULL);
	backupFile += L"Rainmeter.thm";
	CopyFiles(iniFile, backupFile, false);

	if (!data.loadTheme.empty())
	{
		std::wstring themeFile = data.iniPath;
		themeFile += L"Themes\\";
		themeFile += data.loadTheme;
		themeFile += L"\\Rainmeter.thm";
		if (_waccess(themeFile.c_str(), 0) == 0)
		{
			CopyFiles(themeFile, iniFile, false);
			PreserveSetting(iniFile, backupFile, L"Rainmeter", L"SkinPath");
			PreserveSetting(iniFile, backupFile, L"Rainmeter", L"ConfigEditor");
			PreserveSetting(iniFile, backupFile, L"Rainmeter", L"LogViewer");
			PreserveSetting(iniFile, backupFile, L"Rainmeter", L"Logging");
			PreserveSetting(iniFile, backupFile, L"Rainmeter", L"DisableVersionCheck");
			PreserveSetting(iniFile, backupFile, L"Rainmeter", L"Language");
			PreserveSetting(iniFile, backupFile, L"Rainmeter", L"TrayExecuteL");
			PreserveSetting(iniFile, backupFile, L"Rainmeter", L"TrayExecuteM");
			PreserveSetting(iniFile, backupFile, L"Rainmeter", L"TrayExecuteR");
			PreserveSetting(iniFile, backupFile, L"Rainmeter", L"TrayExecuteDL");
			PreserveSetting(iniFile, backupFile, L"Rainmeter", L"TrayExecuteDM");
			PreserveSetting(iniFile, backupFile, L"Rainmeter", L"TrayExecuteDR");
		}
	}

	// Execute and wait up to a minute for Rainmeter to process all messages
	SHELLEXECUTEINFO sei = {0};
	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.fMask = SEE_MASK_WAITFORINPUTIDLE | SEE_MASK_UNICODE;
	sei.hwnd = NULL;
	sei.lpVerb = NULL;
	sei.lpFile = L"Rainmeter.exe";
	sei.lpParameters = NULL;
	sei.nShow = SW_SHOWNORMAL;
	ShellExecuteEx(&sei);

	if (!data.loadSkins.empty())
	{
		std::wstring::size_type pos;
		std::wstring bang;
		std::vector<std::wstring> vec = Tokenize(data.loadSkins, L"|");

		for (unsigned int i = 0, max = vec.size(); i < max; ++i)
		{
			pos = vec[i].find_last_of(L"\\");
			if (pos != std::wstring::npos)
			{
				// Append with [!RainmeterActivateConfig "Config" "File.ini"]
				bang += L"[!RainmeterActivateConfig \"";
				bang += vec[i].substr(0, pos);
				bang += L"\" \"";
				bang += vec[i].substr(pos + 1);
				bang += L"\"]";
			}
		}

		if (!bang.empty())
		{
			bang.insert(0, L"!Execute ");
			sei.fMask = SEE_MASK_UNICODE;
			sei.lpParameters = (LPCTSTR)bang.c_str();
			ShellExecuteEx(&sei);
		}
	}
}

/*
** Create elevated process (to install addons, fonts, and plugins)
*/
HANDLE CreateProcessElevated(HWND hwnd)
{
	WCHAR secDesc[SECURITY_DESCRIPTOR_MIN_LENGTH];
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = &secDesc;
	InitializeSecurityDescriptor(sa.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(sa.lpSecurityDescriptor, TRUE, 0, FALSE);

	// Create shared memory
	hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, 2048, OBJECT_NAME);
	if (hMapFile == NULL)
	{
		std::wstring error = L"Failed to create elevated process (unable to create file mapping).";
		MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
		return NULL;
	}

	pBuffer = (LPCTSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 2048);
	if (pBuffer == NULL)
	{
		CloseHandle(hMapFile);
		std::wstring error = L"Failed to create elevated process (unable to view file mapping).";
		MessageBox(NULL, error.c_str(), APP_NAME, MB_ERROR);
		return NULL;
	}

	WCHAR buffer[2048];

	// Copy values that are to be passed to the elevated process into buffer
	_snwprintf_s(buffer, _TRUNCATE, L"%s\"%i\"%s\"%s\"%s\"%s\"%s\"%i",
									data.rmskinFile.c_str(),
									data.rootLen,
									data.rainmeterPath.c_str(),
									data.backupFolder.empty() ? L" " : data.backupFolder.c_str(),
									data.addonsList.empty() ? L" " : data.addonsList.c_str(),
									data.pluginsList.empty() ? L" " : data.pluginsList.c_str(),
									data.fontsList.empty() ? L" " : data.fontsList.c_str(),
									data.rainmeterFonts ? 1 : 0);

	CopyMemory((PVOID)pBuffer, buffer, sizeof(buffer));

	// If UAC is not active, show custom Run As dialog. There is a bug in Vista/7, which makes elevation
	// impossible when UAC is disabled
	if (IsAboveVista() && !IsActiveUAC())
	{
		return (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_RUNAS), hwnd, (DLGPROC)RunAsProc) == 0) ? data.instHandle : NULL;
	}

	SHELLEXECUTEINFO sei = {0};
	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.hwnd = NULL;
	sei.lpVerb = L"runas";
	sei.lpFile = exeFile.c_str();
	sei.lpParameters = L"/ELEVATE";
	sei.nShow = SW_SHOWNORMAL;
	ShellExecuteEx(&sei);

	if (!IsAboveVista()) _beginthreadex(NULL, 0, SetRunAsThread, NULL, 0, NULL);

	return (sei.hProcess) ? sei.hProcess : NULL;
}

unsigned __stdcall SetRunAsThread(void*)
{
	const UINT IDC_USRSAFER = 0x106, IDC_OTHERUSER = 0x104;

	// Wait for up to 5 seconds for the Run As dialog to appear
	for (int i = 0; i < 50; ++i)
	{
		Sleep(100);
		HWND hwnd = FindWindow(L"#32770", L"Run As...");
		if (hwnd)
		{
			SendMessage(GetDlgItem(hwnd, IDC_USRSAFER), BM_SETCHECK, BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hwnd, IDC_OTHERUSER), BM_CLICK, 0, 0);
			break;
		}
	}
	return 0;
}

unsigned __stdcall CreateInstallThread(void* pParam)
{
	return InstallComponents((RMSKIN_DATA*)pParam) ? 0 : 1;
}

/*
** Splits the string from the delimiters and trims whitespace
*/
std::vector<std::wstring> Tokenize(const std::wstring& str, const std::wstring& delimiters)
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
int CompareVersions(const std::wstring& strA, const std::wstring& strB)
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

std::wstring GetFileVersion(const std::wstring& file)
{
	DWORD bufSize = GetFileVersionInfoSize(file.c_str(), 0);
	void* versionInfo = new WCHAR[bufSize];
	void* fileVersion = 0;
	UINT valueSize;
	std::wstring result;

	if (GetFileVersionInfo(file.c_str(), 0, bufSize, versionInfo))
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

std::wstring GetDotNETVersion()
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

std::wstring GetWindowsVersion()
{
	WCHAR buffer[16];
	OSVERSIONINFOEX osvi = {sizeof(OSVERSIONINFOEX)};
	GetVersionEx((OSVERSIONINFO*)&osvi);
	_snwprintf_s(buffer, _TRUNCATE, L"%d.%d.%d", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);

	return buffer;
}

BOOL IsActiveUAC()
{
	// First check if user has a split token (that implies UAC)
	HANDLE hToken;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		TOKEN_ELEVATION_TYPE tet;
		DWORD dwReturnLength = 0;
		if (GetTokenInformation(hToken, TokenElevationType, &tet, sizeof(TOKEN_ELEVATION_TYPE), &dwReturnLength) &&
			tet != TokenElevationTypeDefault)
		{
			return TRUE;
		}

		CloseHandle(hToken);
	}

	// Check from registry
	DWORD dwValue = 0;
	DWORD dwSize = sizeof(DWORD);
	HKEY hKey;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", 0L, KEY_QUERY_VALUE, &hKey);
	RegQueryValueEx(hKey, L"EnableLUA", NULL, NULL, (LPBYTE)&dwValue, (LPDWORD)&dwSize);
	RegCloseKey(hKey);

	return (dwValue != 0) ? TRUE : FALSE;
}

BOOL IsAboveVista()
{
	static BOOL isAbove = -1;

	if (isAbove == -1)
	{
		OSVERSIONINFOEX osvi = {sizeof(OSVERSIONINFOEX)};
		GetVersionEx((OSVERSIONINFO*)&osvi);
		isAbove = (osvi.dwMajorVersion >= 6) ? TRUE : FALSE;
	}

	return isAbove;
}

BOOL IsCurrentProcessAdmin()
{
	static BOOL isAdmin = -1;

	// Based on the work of Anders Kjersem: http://nsis.sourceforge.net/UAC_plug-in
	if (isAdmin == -1)
	{
		isAdmin = FALSE;
		HANDLE hToken;

		if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
		{
			SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;
			PSID psid = 0;
			if (AllocateAndInitializeSid(&SystemSidAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &psid))
			{
				if (!CheckTokenMembership(0, psid, &isAdmin))
				{
					isAdmin = FALSE;
					DWORD cbTokenGrps;
					if (!GetTokenInformation(hToken, TokenGroups, 0, 0, &cbTokenGrps) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
					{
						TOKEN_GROUPS* ptg = (TOKEN_GROUPS*)GlobalAlloc(LPTR, cbTokenGrps);
						if (ptg)
						{
							if (GetTokenInformation(hToken, TokenGroups, ptg, cbTokenGrps, &cbTokenGrps))
							{
								for (DWORD i = 0; i < ptg->GroupCount; ++i)
								{
									if (EqualSid(ptg->Groups[i].Sid, psid))
									{
										isAdmin = TRUE;
										break;
									}
								}
							}
							GlobalFree(ptg);
						}
					}
				}

				FreeSid(psid);
			}

			// Check if UAC admin with split token check
			if (isAdmin && IsAboveVista()) 
			{
				TOKEN_ELEVATION_TYPE tet;
				DWORD dwReturnLength = 0;
				if (GetTokenInformation(hToken, TokenElevationType, &tet, sizeof(TOKEN_ELEVATION_TYPE), &dwReturnLength) &&
					tet == TokenElevationTypeLimited)
				{
					isAdmin = FALSE;
				}
			}

			CloseHandle(hToken);
		}
	}

	return isAdmin;
}
