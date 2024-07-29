/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <Windows.h>
#include <ShellAPI.h>
#include <Shlwapi.h>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

typedef int (*RainmeterMainFunc)(LPWSTR cmdLine);

WCHAR* GetCommandLineArguments()
{
	WCHAR* args = GetCommandLine();

	// Skip past (quoted) application path in cmdLine.
	if (*args == L'"')
	{
		++args;  // Skip leading quote.
		while (*args && *args != L'"')
		{
			++args;
		}
		++args;  // Skip trailing quote.
	}
	else
	{
		while (*args && *args != L' ')
		{
			++args;
		}
	}

	// Skip leading whitespace (similar to CRT implementation).
	while (*args && *args <= L' ')
	{
		++args;
	}

	return args;
}

/*
** Entry point. In Release builds, the entry point is Main() since the CRT is not used.
**
*/
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(000);

	WCHAR path[MAX_PATH];
	SecureZeroMemory(path, _countof(path));

	DWORD num = GetModuleFileName(nullptr, path, _countof(path));
	if (path && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
		WCHAR* file = PathFindFileName(path);
		if (file && lstrcmpi(file, L"Rainmeter.exe") != 0)
		{
			WCHAR message[MAX_PATH];
			SecureZeroMemory(message, _countof(message));

			wsprintf(message, L"Please rename \"%s\" to Rainmeter.exe", file);
			MessageBox(nullptr, message, L"Rainmeter", MB_OK | MB_ICONERROR);
			return 1;
		}
	}

	// Prevent system error message boxes.
	UINT oldMode = SetErrorMode(0U);
	SetErrorMode(oldMode | SEM_FAILCRITICALERRORS);

	HINSTANCE instance = (HINSTANCE)&__ImageBase;
	WCHAR* args = GetCommandLineArguments();

	HRSRC iconResource = FindResource(instance, MAKEINTRESOURCE(1), RT_ICON);
	if (iconResource)
	{
		HINSTANCE rmDll = LoadLibrary(L"Rainmeter.dll");
		if (rmDll)
		{
			auto rainmeterMain = (RainmeterMainFunc)GetProcAddress(rmDll, MAKEINTRESOURCEA(1));
			if (rainmeterMain)
			{
				return rainmeterMain(args);
			}
		}

		WCHAR message[128];
		wsprintf(
			message,
			L"Rainmeter.dll load error %ld.",
			GetLastError());
		MessageBox(nullptr, message, L"Rainmeter", MB_OK | MB_ICONERROR);
	}
	else
	{
		// Stub prodecure. If icon resources have been removed, try to launch the actual Rainmeter.exe.
		HKEY hKey = nullptr;
		const REGSAM desiredSam = KEY_QUERY_VALUE | KEY_WOW64_32KEY;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Rainmeter", 0, desiredSam, &hKey) == ERROR_SUCCESS)
		{
			const DWORD size = MAX_PATH;
			WCHAR buffer[size];
			DWORD type = 0UL;
			if (RegQueryValueEx(hKey, nullptr , nullptr, &type, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS &&
				type == REG_SZ)
			{
				SetCurrentDirectory(buffer);
				lstrcat(buffer, L"\\Rainmeter.exe");
				ShellExecute(nullptr, L"open", buffer, args, nullptr, SW_SHOWNORMAL);
			}
			RegCloseKey(hKey);
			hKey = nullptr;
		}

		return 0;
	}

	return 1;
}

#ifndef _DEBUG
EXTERN_C int WINAPI Main()
{
	int result = wWinMain(nullptr, nullptr, nullptr, 0);
	ExitProcess(result);
	return 0;  // Never reached.
}
#endif
