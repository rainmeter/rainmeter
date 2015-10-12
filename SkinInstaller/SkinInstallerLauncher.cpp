/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include <Windows.h>
#include <Shlwapi.h>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

typedef int (*SkinInstallerMainFunc)(LPWSTR cmdLine);

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
** Attempts to load SkinInstaller.dll. If it fails, retries after loading our own copies of the
** CRT DLLs in the Runtime directory.
*/
HINSTANCE LoadSkinInstallerLibrary()
{
	HINSTANCE rmDll = LoadLibrary(L"SkinInstaller.dll");
	if (!rmDll)
	{
		WCHAR path[MAX_PATH];
		if (GetModuleFileName(nullptr, path, MAX_PATH) > 0)
		{
			PathRemoveFileSpec(path);
			PathAppend(path, L"Runtime");
			SetDllDirectory(path);
			PathAppend(path, L"msvcp120.dll");

			// Loading msvcpNNN.dll will load msvcrNNN.dll as well.
			HINSTANCE msvcrDll = LoadLibrary(path);
			SetDllDirectory(L"");

			if (msvcrDll)
			{
				rmDll = LoadLibrary(L"SkinInstaller.dll");
				FreeLibrary(msvcrDll);
			}
		}
	}

	return rmDll;
}

/*
** Entry point. In Release builds, the entry point is Main() since the CRT is not used.
**
*/
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	// Prevent system error message boxes.
	UINT oldMode = SetErrorMode(0);
	SetErrorMode(oldMode | SEM_FAILCRITICALERRORS);

	WCHAR* args = GetCommandLineArguments();

	HINSTANCE skinInstallerDll = LoadSkinInstallerLibrary();
	if (skinInstallerDll)
	{
		auto skinInstallerMain =
			(SkinInstallerMainFunc)GetProcAddress(skinInstallerDll, MAKEINTRESOURCEA(1));
		if (skinInstallerMain)
		{
			return skinInstallerMain(args);
		}
	}

	WCHAR message[128];
	wsprintf(
		message,
		L"SkinInstaller.dll load error %ld.",
		GetLastError());
	MessageBox(nullptr, message, L"Skin Installer", MB_OK | MB_ICONERROR);

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
