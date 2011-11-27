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
	if (wcscmp(lpCmdLine, L"/BACKUP") != 0)
	{
		// Temporary solution until Rainstaller rewrite
		return Rainstaller(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	}

	// Check whether Rainstaller.exe is already running and bring it to front if so
	HANDLE hMutex;
	if (IsRunning(L"RmSkinInstallerMutex", &hMutex))
	{
		HWND hwnd = FindWindow(L"#32770", L"Backup Rainmeter");
		SetForegroundWindow(hwnd);
		return 0;
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

	CDialogBackup::Create(hInstance, lpCmdLine);

	ReleaseMutex(hMutex);

	return 0;
}

/*
** Checks whether Rainstaller.exe is running (modified from Application.cpp)
*/
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