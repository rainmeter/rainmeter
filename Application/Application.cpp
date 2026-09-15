// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <Windows.h>
#include <ShellAPI.h>
#include <Shlwapi.h>
#include <stdarg.h>

#include "Uninstall.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

typedef int (*RainmeterMainFunc)(LPWSTR cmdLine);

void FormatString(WCHAR* destination, size_t count, const WCHAR* format, ...)
{
	if (!count) return;

	va_list arguments;
	va_start(arguments, format);
	DWORD length = FormatMessage(FORMAT_MESSAGE_FROM_STRING, format, 0, 0, destination, static_cast<DWORD>(count), &arguments);
	va_end(arguments);
	if (!length) destination[0] = 0;
}

static bool HasCommandLineArgument(const WCHAR* expected, bool prefix = false)
{
	int argc = 0;
	WCHAR** argv = CommandLineToArgvW(GetCommandLine(), &argc);
	bool found = false;
	int length = lstrlen(expected);
	for (int index = 1; index < argc; ++index)
	{
		bool matches = prefix ? lstrlen(argv[index]) >= length && CompareStringOrdinal(argv[index], length, expected, length, TRUE) == CSTR_EQUAL
		                      : lstrcmpi(argv[index], expected) == 0;
		if (matches)
		{
			found = true;
			break;
		}
	}

	LocalFree(argv);
	return found;
}

static int LaunchUninstaller()
{
	WCHAR executable[MAX_PATH];
	if (!GetModuleFileName(nullptr, executable, _countof(executable))) return 1;

	WCHAR directory[MAX_PATH];
	lstrcpyn(directory, executable, _countof(directory));
	PathRemoveFileSpec(directory);

	WCHAR temporaryDirectory[MAX_PATH];
	if (!GetTempPath(_countof(temporaryDirectory), temporaryDirectory)) return 1;

	WCHAR temporaryExecutable[MAX_PATH];
	FormatString(temporaryExecutable, _countof(temporaryExecutable), L"%1!s!Rainmeter-uninst-%2!u!-%3!u!.exe", temporaryDirectory, GetCurrentProcessId(), GetTickCount());

	if (!CopyFile(executable, temporaryExecutable, FALSE)) return 1;

	WCHAR parameters[MAX_PATH * 2];
	FormatString(parameters, _countof(parameters), L"/Uninstall /PARENT=%1!u! /D=\"%2!s!\"%3!s!%4!s!", GetCurrentProcessId(), directory,
	             HasCommandLineArgument(L"/S") ? L" /S" : L"", HasCommandLineArgument(L"/DELETEALL=1") ? L" /DELETEALL=1" : L"");

	SHELLEXECUTEINFO info;
	SecureZeroMemory(&info, sizeof(info));
	info.cbSize = sizeof(info);
	info.fMask = SEE_MASK_NOCLOSEPROCESS;
	info.lpVerb = L"open";
	info.lpFile = temporaryExecutable;
	info.lpParameters = parameters;
	info.lpDirectory = temporaryDirectory;
	info.nShow = SW_SHOWNORMAL;
	if (!ShellExecuteEx(&info))
	{
		DeleteFile(temporaryExecutable);
		return 1;
	}

	WaitForSingleObject(info.hProcess, INFINITE);
	DWORD exitCode = 1;
	GetExitCodeProcess(info.hProcess, &exitCode);
	CloseHandle(info.hProcess);
	return (int)exitCode;
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

// Entry point. In Release builds, the entry point is Main() since the CRT is not used.
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(000);

	if (HasCommandLineArgument(L"/Uninstall")) return HasCommandLineArgument(L"/D=", true) ? Uninstall() : LaunchUninstaller();

	WCHAR path[MAX_PATH];
	path[0] = L'\0';
	DWORD num = GetModuleFileName(nullptr, path, _countof(path));
	if (num > 0 && *path && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
		WCHAR* file = PathFindFileName(path);
		if (file && lstrcmpi(file, L"Rainmeter.exe") != 0)
		{
			WCHAR message[1024];
			wsprintf(message, L"Please rename \"%s\" to Rainmeter.exe", file);
			MessageBox(nullptr, message, L"Rainmeter", MB_OK | MB_ICONERROR);
			return 1;
		}
	}

	// Prevent system error message boxes.
	UINT oldMode = SetErrorMode(0);
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

		WCHAR message[1024];
		wsprintf(message, L"Rainmeter.dll load error %ld.", GetLastError());
		MessageBox(nullptr, message, L"Rainmeter", MB_OK | MB_ICONERROR);
	}
	else
	{
		// Stub prodecure. If icon resources have been removed, try to launch the actual Rainmeter.exe.
		WCHAR buffer[MAX_PATH];
		DWORD bufferSize = sizeof(buffer);
		DWORD type = 0;
		if (RegGetValue(HKEY_LOCAL_MACHINE, L"Software\\Rainmeter", nullptr, RRF_RT_REG_SZ | RRF_SUBKEY_WOW6432KEY, &type, buffer, &bufferSize) == ERROR_SUCCESS)
		{
			SetCurrentDirectory(buffer);
			lstrcat(buffer, L"\\Rainmeter.exe");
			ShellExecute(nullptr, L"open", buffer, args, nullptr, SW_SHOWNORMAL);
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
