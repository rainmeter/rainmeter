// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include <Windows.h>
#include <Shlwapi.h>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

EXTERN_C int SkinInstallerMain(LPWSTR lpCmdLine);

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

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	// Prevent system error message boxes.
	UINT oldMode = SetErrorMode(0);
	SetErrorMode(oldMode | SEM_FAILCRITICALERRORS);

	WCHAR* args = GetCommandLineArguments();
	return SkinInstallerMain(args);
}

#ifndef _DEBUG
EXTERN_C int WINAPI Main()
{
	int result = wWinMain(nullptr, nullptr, nullptr, 0);
	ExitProcess(result);
	return 0;  // Never reached.
}
#endif
