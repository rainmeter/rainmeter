/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __RAINMETER_SYSTEM_H__
#define __RAINMETER_SYSTEM_H__

#include <windows.h>
#include <queue>
#include <vector>

class System
{
public:
	System(const System& other) = delete;
	System& operator=(System other) = delete;

	static void Initialize(HINSTANCE instance);
	static void Finalize();

	static UINT GetDpiForWindow(HWND window);
	static UINT GetSystemDpi();

	static bool GetShowDesktop() { return c_ShowDesktop; }

	static HWND GetWindow() { return c_Window; }
	static HWND GetBackmostTopWindow();

	static HWND GetHelperWindow() { return c_HelperWindow; }
	static void PrepareHelperWindow(HWND desktopIconsHostWindow = GetDesktopIconsHostWindow());

	static POINT GetCursorPosition();

	static bool IsFileWritable(LPCWSTR file);

	static HMODULE RmLoadLibrary(LPCWSTR lpLibFileName, DWORD* dwError = nullptr);
	static void ResetWorkingDirectory();
	static void InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection);

	static void SetClipboardText(const std::wstring& text);
	static void SetWallpaper(const std::wstring& wallpaper, const std::wstring& style);

	static bool CopyFiles(std::wstring from, std::wstring to, bool bMove = false);
	static bool CopyFilesWithNoCollisions(std::wstring from, const std::wstring& to);
	static bool RemoveFile(const std::wstring& file);
	static bool RemoveFolder(std::wstring folder);

	static void UpdateIniFileMappingList();
	static std::wstring GetTemporaryFile(const std::wstring& iniFile);

	static bool IsProcessRunningCached(const std::wstring& lowercaseName);

private:
	static void CALLBACK MyWinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static HWND GetDefaultShellWindow();
	static HWND GetDesktopIconsHostWindow();
	static void ChangeZPosInOrder();

	static bool CheckDesktopState(HWND desktopIconsHostWindow);
	static bool BelongToSameProcess(HWND hwndA, HWND hwndB);

	static HWND c_Window;
	static HWND c_HelperWindow;

	static HWINEVENTHOOK c_WinEventHook;

	static bool c_ShowDesktop;

	static std::wstring c_WorkingDirectory;

	static std::vector<std::wstring> c_IniFileMappings;
};

#endif
