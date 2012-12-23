/*
  Copyright (C) 2001 Kimmo Pekkola

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

#define _CRTDBG_MAP_ALLOC
#define WIN32_LEAN_AND_MEAN
#include <crtdbg.h>
#include <Windows.h>
#include <ShellAPI.h>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

typedef int (*RainmeterMainFunc)(WCHAR* args);

HINSTANCE LoadRainmeterLibrary()
{
	const int bufferSize = MAX_PATH;
	WCHAR buffer[bufferSize];
	int bufferLen = GetModuleFileName(NULL, buffer, bufferSize);
	if (bufferLen == 0 || bufferLen == bufferSize)
	{
		return NULL;
	}

	// Change extension from .exe to .dll.
	buffer[bufferLen - 3] = L'd';
	buffer[bufferLen - 2] = L'l';
	buffer[bufferLen - 1] = L'l';

	return LoadLibrary(buffer);
}

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

	HINSTANCE instance = (HINSTANCE)&__ImageBase;
	WCHAR* args = GetCommandLineArguments();

	HRSRC iconResource = FindResource(instance, MAKEINTRESOURCE(1), RT_ICON);
	if (iconResource)
	{
		// Initialize Rainmeter.
		HINSTANCE libInstance = LoadRainmeterLibrary();
		if (libInstance)
		{
			auto rainmeterMain = (RainmeterMainFunc)GetProcAddress(libInstance, MAKEINTRESOURCEA(1));
			int result = rainmeterMain(args);
			FreeLibrary(libInstance);
			return result;
		}
		else
		{
			WCHAR buffer[128];
			int arch = 32;
#ifdef _WIN64
			arch = 64;
#endif
			const WCHAR* error = L"Rainmeter.dll (%i-bit) error %i.\n\nDo you want to view help online?";
			wsprintf(buffer, error, arch, GetLastError());
			if (MessageBox(NULL, buffer, L"Rainmeter", MB_YESNO | MB_ICONERROR) == IDYES)
			{
				ShellExecute(NULL, L"open", L"http://rainmeter.net/dllerror", NULL, NULL, SW_SHOWNORMAL); 
			}
		}
	}
	else
	{
		// Stub prodecure. If icon resources have been removed, try to launch the actual Rainmeter.exe.
		HKEY hKey;
		const REGSAM desiredSam = KEY_QUERY_VALUE | KEY_WOW64_32KEY;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Rainmeter", 0, desiredSam, &hKey) == ERROR_SUCCESS)
		{
			const DWORD size = MAX_PATH;
			WCHAR buffer[size];
			DWORD type = 0;
			if (RegQueryValueEx(hKey, NULL , NULL, &type, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS &&
				type == REG_SZ)
			{
				SetCurrentDirectory(buffer);
				lstrcat(buffer, L"\\Rainmeter.exe");
				ShellExecute(NULL, L"open", buffer, args, NULL, SW_SHOWNORMAL);
			}
			RegCloseKey(hKey);
		}

		return 0;
	}

	return 1;
}

#ifndef _DEBUG
EXTERN_C int WINAPI Main()
{
	int result = wWinMain(NULL, NULL, NULL, 0);
	TerminateProcess(GetCurrentProcess(), result);
	return 0;  // Never reached.
}
#endif
