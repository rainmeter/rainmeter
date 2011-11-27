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
#include "DialogBackup.h"
#include "resource.h"
#include "Application.h"
#include "Rainstaller.h"

GLOBALDATA g_Data;

/*
** Entry point
*/
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	bool loadTheme = (_wcsnicmp(lpCmdLine, L"/LoadTheme ", 11) == 0);
	if (wcscmp(lpCmdLine, L"/BACKUP") != 0 && !loadTheme)
	{
		// Temporary solution until Rainstaller rewrite
		return Rainstaller(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	}

	// Avoid loading a dll from current directory
	SetDllDirectory(L"");

	WCHAR buffer[MAX_PATH];
	GetModuleFileName(hInstance, buffer, MAX_PATH);

	// Remove the module's name from the path
	WCHAR* pos = wcsrchr(buffer, L'\\');
	if (pos)
	{
		*(pos + 1) = L'\0';
	}

	g_Data.programPath = g_Data.settingsPath = buffer;
	wcscat(buffer, L"Rainmeter.ini");

	// Find the settings file and read skins path off it
	if (_waccess(buffer, 0) == 0)
	{
		g_Data.iniFile = buffer;
		if (GetPrivateProfileString(L"Rainmeter", L"SkinPath", L"", buffer, MAX_LINE_LENGTH, buffer) > 0)
		{
			g_Data.skinsPath = buffer;
		}
		else
		{
			g_Data.skinsPath = g_Data.programPath;
			g_Data.skinsPath += L"Skins\\";
		}
	}
	else
	{
		HRESULT hr = SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buffer);
		wcscat(buffer, L"\\Rainmeter\\");
		g_Data.settingsPath = buffer;
		wcscat(buffer, L"Rainmeter.ini");
		g_Data.iniFile = buffer;
		if (SUCCEEDED(hr) && _waccess(buffer, 0) == 0)
		{
			if (GetPrivateProfileString(L"Rainmeter", L"SkinPath", L"", buffer, MAX_LINE_LENGTH, buffer) > 0)
			{
				g_Data.skinsPath = buffer;
			}
			else
			{
				std::wstring error = L"SkinPath not found.\nMake sure that Rainmeter has been run at least once.";
				MessageBox(NULL, error.c_str(), L"Backup Rainmeter", MB_ERROR);
				return 1;
			}
		}
		else
		{
			std::wstring error = L"Rainmeter.ini not found.\nMake sure that Rainmeter has been run at least once.";
			MessageBox(NULL, error.c_str(), L"Backup Rainmeter", MB_ERROR);
			return 1;
		}
	}

	if (loadTheme)
	{
		// Skip "/LOADTHEME "
		lpCmdLine += 11;

		if (CloseRainmeterIfActive() && *lpCmdLine)
		{
			LoadTheme(lpCmdLine);

			std::wstring file = g_Data.programPath + L"Rainmeter.exe";
			SHELLEXECUTEINFO sei = {0};
			sei.cbSize = sizeof(SHELLEXECUTEINFO);
			sei.fMask = SEE_MASK_UNICODE;
			sei.lpFile = file.c_str();
			sei.nShow = SW_SHOWNORMAL;
			ShellExecuteEx(&sei);
		}

		return 0;
	}

	// Check whether Rainstaller.exe is already running and bring it to front if so
	HANDLE hMutex;
	if (IsRunning(L"RmSkinInstallerMutex", &hMutex))
	{
		HWND hwnd = FindWindow(L"#32770", L"Backup Rainmeter");
		SetForegroundWindow(hwnd);
		return 0;
	}

	CDialogBackup::Create(hInstance, lpCmdLine);

	ReleaseMutex(hMutex);
	return 0;
}

bool CloseRainmeterIfActive()
{
	// Close Rainmeter.exe
	HWND hwnd = FindWindow(L"DummyRainWClass", L"Rainmeter control window");
	if (hwnd)
	{
		DWORD pID, exitCode;
		GetWindowThreadProcessId(hwnd, &pID);
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pID);
		PostMessage(hwnd, WM_DESTROY, 0, 0);

		// Wait up to 5 seconds for Rainmeter to close
		WaitForSingleObject(hProcess, 5000);
		GetExitCodeProcess(hProcess, &exitCode);
		CloseHandle(hProcess);

		if (exitCode == STILL_ACTIVE)
		{
			return false;
		}
	}

	return true;
}

void LoadTheme(const WCHAR* name)
{
	std::wstring backup = g_Data.settingsPath + L"Themes\\Backup";
	CreateDirectory(backup.c_str(), NULL);
	backup += L"\\Rainmeter.thm";

	// Make a copy of current Rainmeter.ini
	CopyFiles(g_Data.iniFile, backup);

	// Replace Rainmeter.ini with theme
	std::wstring theme = g_Data.settingsPath + L"Themes\\";
	theme += name;
	std::wstring wallpaper = theme + L"\\RainThemes.bmp";
	theme += L"\\Rainmeter.thm";
	if (CopyFiles(theme, g_Data.iniFile))
	{
		PreserveSetting(backup, L"SkinPath");
		PreserveSetting(backup, L"ConfigEditor");
		PreserveSetting(backup, L"LogViewer");
		PreserveSetting(backup, L"Logging");
		PreserveSetting(backup, L"DisableVersionCheck");
		PreserveSetting(backup, L"Language");
		PreserveSetting(backup, L"NormalStayDesktop");
		PreserveSetting(backup, L"TrayExecuteL", false);
		PreserveSetting(backup, L"TrayExecuteM", false);
		PreserveSetting(backup, L"TrayExecuteR", false);
		PreserveSetting(backup, L"TrayExecuteDM", false);
		PreserveSetting(backup, L"TrayExecuteDR", false);

		// Set wallpaper if it exists
		if (_waccess(wallpaper.c_str(), 0) != -1)
		{
			SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (void*)wallpaper.c_str(), SPIF_UPDATEINIFILE);
		}
	}
}

void PreserveSetting(const std::wstring& from, LPCTSTR key, bool replace)
{
	WCHAR* buffer = new WCHAR[MAX_LINE_LENGTH];

	if ((replace || GetPrivateProfileString(L"Rainmeter", key, L"", buffer, 4, g_Data.iniFile.c_str()) == 0) &&
		GetPrivateProfileString(L"Rainmeter", key, L"", buffer, MAX_LINE_LENGTH, from.c_str()) > 0)
	{
		WritePrivateProfileString(L"Rainmeter", key, buffer, g_Data.iniFile.c_str());
	}

	delete buffer;
}

// -----------------------------------------------------------------------------------------------
// Stolen functions from Rainmeter Litestep.cpp, System.cpp, and Application.cpp
// -----------------------------------------------------------------------------------------------

bool IsRunning(const WCHAR* name, HANDLE* hMutex)
{
	// Create mutex
	HANDLE hMutexTmp = CreateMutex(NULL, FALSE, name);
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		*hMutex = NULL;
		return true;
	}
	else
	{
		*hMutex = hMutexTmp;
		return false;
	}
}

bool CopyFiles(const std::wstring& strFrom, const std::wstring& strTo, bool bMove)
{
	std::wstring tmpFrom(strFrom), tmpTo(strTo);

	// The strings must end with double nul
	tmpFrom.append(1, L'\0');
	tmpTo.append(1, L'\0');

	SHFILEOPSTRUCT fo =
	{
		NULL,
		bMove ? FO_MOVE : FO_COPY,
		tmpFrom.c_str(),
		tmpTo.c_str(),
		FOF_NO_UI | FOF_NOCONFIRMATION | FOF_ALLOWUNDO
	};

	return SHFileOperation(&fo) != 0;
}

std::string ConvertToAscii(LPCTSTR str)
{
	std::string szAscii;

	if (str && *str)
	{
		int strLen = (int)wcslen(str);
		int bufLen = WideCharToMultiByte(CP_ACP, 0, str, strLen, NULL, 0, NULL, NULL);
		if (bufLen > 0)
		{
			szAscii.resize(bufLen);
			WideCharToMultiByte(CP_ACP, 0, str, strLen, &szAscii[0], bufLen, NULL, NULL);
		}
	}
	return szAscii;
}

std::wstring ConvertToWide(LPCSTR str)
{
	std::wstring szWide;

	if (str && *str)
	{
		int strLen = (int)strlen(str);
		int bufLen = MultiByteToWideChar(CP_ACP, 0, str, strLen, NULL, 0);
		if (bufLen > 0)
		{
			szWide.resize(bufLen);
			MultiByteToWideChar(CP_ACP, 0, str, strLen, &szWide[0], bufLen);
		}
	}
	return szWide;
}