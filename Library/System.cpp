/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "System.h"
#include "MonitorUtil.h"
#include "Util.h"
#include "Rainmeter.h"
#include "Skin.h"
#include "MeasureNet.h"
#include "../Common/PathUtil.h"
#include <TlHelp32.h>
#include <WtsApi32.h>

bool ConvertImageToBmpFile(const std::wstring& source, const std::wstring& target);

#define DEBUG_VERBOSE  (0)  // Set 1 if you need verbose logging.

#define ZPOS_FLAGS	(SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING)

enum TIMER
{
	TIMER_SHOWDESKTOP   = 1,
	TIMER_RESUME        = 2
};
enum INTERVAL
{
	INTERVAL_SHOWDESKTOP    = 250,
	INTERVAL_RESTOREWINDOWS = 100,
	INTERVAL_RESUME         = 1000
};

HWND System::c_Window = nullptr;
HWND System::c_HelperWindow = nullptr;

HWINEVENTHOOK System::c_WinEventHook = nullptr;

bool System::c_ShowDesktop = false;

std::wstring System::c_WorkingDirectory;

std::vector<std::wstring> System::c_IniFileMappings;

/*
** Creates a helper window to detect changes in the system.
**
*/
void System::Initialize(HINSTANCE instance)
{
	// Update the CRT timezone variables.
	_tzset();

	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.hInstance = instance;
	wc.lpszClassName = L"RainmeterSystem";
	ATOM className = RegisterClass(&wc);

	c_Window = CreateWindowEx(
		WS_EX_TOOLWINDOW,
		MAKEINTATOM(className),
		L"System",
		WS_POPUP | WS_DISABLED,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		nullptr,
		nullptr,
		instance,
		nullptr);

	c_HelperWindow = CreateWindowEx(
		WS_EX_TOOLWINDOW,
		MAKEINTATOM(className),
		L"PositioningHelper",
		WS_POPUP | WS_DISABLED,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		nullptr,
		nullptr,
		instance,
		nullptr);

	WTSRegisterSessionNotification(c_Window, NOTIFY_FOR_THIS_SESSION);

	SetWindowPos(c_Window, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);
	SetWindowPos(c_HelperWindow, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);

	MonitorUtil::InitializeMultiMonitorInfo();

	WCHAR directory[MAX_PATH];
	DWORD len = GetCurrentDirectory(MAX_PATH, directory);
	c_WorkingDirectory.assign(directory, len <= MAX_PATH ? len : 0);

	c_WinEventHook = SetWinEventHook(
		EVENT_SYSTEM_FOREGROUND,
		EVENT_SYSTEM_FOREGROUND,
		nullptr,
		MyWinEventProc,
		0,
		0,
		WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

	SetTimer(c_Window, TIMER_SHOWDESKTOP, INTERVAL_SHOWDESKTOP, nullptr);
}

/*
** Destroys a window.
**
*/
void System::Finalize()
{
	KillTimer(c_Window, TIMER_SHOWDESKTOP);
	KillTimer(c_Window, TIMER_RESUME);

	if (c_WinEventHook)
	{
		UnhookWinEvent(c_WinEventHook);
		c_WinEventHook = nullptr;
	}

	if (c_HelperWindow)
	{
		DestroyWindow(c_HelperWindow);
		c_HelperWindow = nullptr;
	}

	if (c_Window)
	{
		WTSUnRegisterSessionNotification(c_Window);
		DestroyWindow(c_Window);
		c_Window = nullptr;
	}
}

UINT System::GetSystemDpi()
{
	typedef UINT(WINAPI* GetDpiForSystemProc)();
	static auto s_GetDpiForSystemProc = (GetDpiForSystemProc)GetProcAddress(GetModuleHandle(L"user32"), "GetDpiForSystem");
	if (s_GetDpiForSystemProc) return s_GetDpiForSystemProc();

	HDC dc = GetDC(nullptr);
	if (dc)
	{
		const int dpi = GetDeviceCaps(dc, LOGPIXELSX);
		ReleaseDC(nullptr, dc);
		if (dpi > 0) return (UINT)dpi;
	}

	return USER_DEFAULT_SCREEN_DPI;
}

UINT System::GetDpiForWindow(HWND window)
{
	typedef UINT(WINAPI* GetDpiForWindowProc)(HWND);
	static auto s_GetDpiForWindow = (GetDpiForWindowProc)GetProcAddress(GetModuleHandle(L"user32"), "GetDpiForWindow");

	if (window && s_GetDpiForWindow)
	{
		const UINT dpi = s_GetDpiForWindow(window);
		if (dpi > 0) return dpi;
	}

	return GetSystemDpi();
}

/*
** Finds the Default Shell's window.
**
*/
HWND System::GetDefaultShellWindow()
{
	static HWND c_ShellW = nullptr;  // cache
	HWND shellW = GetShellWindow();

	if (shellW)
	{
		if (shellW == c_ShellW)
		{
			return shellW;
		}
		else
		{
			const int classLen = _countof(L"Progman") + 1;
			WCHAR className[classLen];
			if (!(GetClassName(shellW, className, classLen) > 0 &&
				wcscmp(className, L"Progman") == 0))
			{
				shellW = nullptr;
			}
		}
	}

	c_ShellW = shellW;
	return shellW;
}

// Windows 11 24H2 reordered the desktop shell window hierarchy.
//
// Spy++ output before Windows 11 24H2:
//
//   0x00010190 "" WorkerW
//     ...
//     0x000100EE "" SHELLDLL_DefView
//       0x000100F0 "FolderView" SysListView32
//   0x00100B8A "" WorkerW
//   0x000100EC "Program Manager" Progman
//
// Spy++ output after Windows 11 24H2:
//
//   0x000100EC "Program Manager" Progman
//     0x000100EE "" SHELLDLL_DefView
//       0x000100F0 "FolderView" SysListView32
//     0x00100B8A "" WorkerW
//
// So if we're on 24H2+, we should be using the shell window (Progman) instead of WorkerW.
bool ShouldUseShellWindowAsDesktopIconsHost() {
	// Check for the existence of GetCurrentMonitorTopologyId, which should be present only
	// on Windows 11 build 10.0.26100.2454.
	static bool result = GetProcAddress(GetModuleHandle(L"user32"), "GetCurrentMonitorTopologyId") != nullptr;
	return result;
}

// We position our windows relative to the parent of SHELLDLL_DefView, which is what
// contains the desktop icons.
HWND System::GetDesktopIconsHostWindow()
{
	static HWND c_DefView = nullptr;  // cache
	HWND shellW = GetDefaultShellWindow();
	if (!shellW) return nullptr;  // Default Shell (Explorer) not running

	if (ShouldUseShellWindowAsDesktopIconsHost()) {
		if (FindWindowEx(shellW, nullptr, L"SHELLDLL_DefView", L""))
		{
			return shellW;
		}

		return nullptr;
	}

	if (c_DefView && IsWindow(c_DefView))
	{
		HWND parent = GetAncestor(c_DefView, GA_PARENT);
		if (parent)
		{
			if (parent == shellW)
			{
				return nullptr;
			}
			else
			{
				const int classLen = _countof(L"WorkerW") + 1;
				WCHAR className[classLen];
				if (GetClassName(parent, className, classLen) > 0 &&
					wcscmp(className, L"WorkerW") == 0)
				{
					return parent;
				}
			}
		}
	}

	HWND workerW = nullptr;
	HWND defView = FindWindowEx(shellW, nullptr, L"SHELLDLL_DefView", L"");
	if (defView == nullptr)
	{
		while (workerW = FindWindowEx(nullptr, workerW, L"WorkerW", L""))
		{
			if (IsWindowVisible(workerW) &&
				BelongToSameProcess(shellW, workerW) &&
				(defView = FindWindowEx(workerW, nullptr, L"SHELLDLL_DefView", L"")))
			{
				break;
			}
		}
	}

	c_DefView = defView;
	return workerW;
}

/*
** Returns the first window whose position is not ZPOSITION_ONDESKTOP,
** ZPOSITION_BOTTOM, or ZPOSITION_NORMAL.
**
*/
HWND System::GetBackmostTopWindow()
{
	HWND winPos = c_HelperWindow;

	// Skip all ZPOSITION_ONDESKTOP, ZPOSITION_BOTTOM, and ZPOSITION_NORMAL windows
	while (winPos = ::GetNextWindow(winPos, GW_HWNDPREV))
	{
		Skin* wnd = GetRainmeter().GetSkin(winPos);
		if (!wnd ||
			(wnd->GetWindowZPosition() != ZPOSITION_NORMAL &&
			wnd->GetWindowZPosition() != ZPOSITION_ONDESKTOP &&
			wnd->GetWindowZPosition() != ZPOSITION_ONBOTTOM))
		{
			break;
		}
	}

	return winPos;
}

/*
** Checks whether the given windows belong to the same process.
**
*/
bool System::BelongToSameProcess(HWND hwndA, HWND hwndB)
{
	DWORD procAId = 0, procBId = 0;

	GetWindowThreadProcessId(hwndA, &procAId);
	GetWindowThreadProcessId(hwndB, &procBId);

	return (procAId == procBId);
}

/*
** Retrieves the Rainmeter's meter windows in Z-order.
**
*/
BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	bool logging = GetRainmeter().GetDebug() && DEBUG_VERBOSE;
	const int classLen = _countof(METERWINDOW_CLASS_NAME) + (DEBUG_VERBOSE ? 32 : 1);
	WCHAR className[classLen];
	Skin* Window;
	WCHAR flag;

	if (GetClassName(hwnd, className, classLen) > 0 &&
		wcscmp(className, METERWINDOW_CLASS_NAME) == 0 &&
		(Window = GetRainmeter().GetSkin(hwnd)))
	{
		ZPOSITION zPos = Window->GetWindowZPosition();
		if (zPos == ZPOSITION_ONDESKTOP ||
			(zPos == ZPOSITION_NORMAL && GetRainmeter().IsNormalStayDesktop()) ||
			zPos == ZPOSITION_ONBOTTOM)
		{
			if (lParam)
			{
				((std::vector<Skin*>*)lParam)->push_back(Window);
			}

			if (logging) flag = L'+';
		}
		else
		{
			if (logging) flag = L'-';
		}

		if (logging)
		{
			LogDebugF(L"%c [%c] 0x%p : %s (Name: \"%s\", zPos=%i)",
				flag, IsWindowVisible(hwnd) ? L'V' : L'H', hwnd, className, Window->GetFolderPath().c_str(), (int)zPos);
		}
	}
	else
	{
		if (logging)
		{
			flag = (hwnd == System::GetHelperWindow()) ? L'o' : ' ';
			LogDebugF(L"%c [%c] 0x%p : %s", flag, IsWindowVisible(hwnd) ? L'V' : L'H', hwnd, className);
		}
	}

	return TRUE;
}

/*
** Arranges the meter window in Z-order.
**
*/
void System::ChangeZPosInOrder()
{
	bool logging = GetRainmeter().GetDebug() && DEBUG_VERBOSE;
	std::vector<Skin*> windowsInZOrder;

	if (logging) LogDebug(L"1: ----- BEFORE -----");

	// Retrieve the Rainmeter's meter windows in Z-order
	EnumWindows(MyEnumWindowsProc, (LPARAM)(&windowsInZOrder));

	auto resetZPos = [&](ZPOSITION zpos)
	{
		// Reset ZPos in Z-order (Bottom)
		std::vector<Skin*>::const_iterator iter = windowsInZOrder.begin();
		for ( ; iter != windowsInZOrder.end(); ++iter)
		{
			if ((*iter)->GetWindowZPosition() == zpos)
			{
				(*iter)->ChangeZPos(zpos);  // reset
			}
		}
	};

	if (GetRainmeter().IsNormalStayDesktop())
	{
		resetZPos(ZPOSITION_NORMAL);
	}

	if (!c_ShowDesktop)
	{
		resetZPos(ZPOSITION_ONBOTTOM);
	}

	resetZPos(ZPOSITION_ONDESKTOP);

	if (logging)
	{
		LogDebug(L"2: ----- AFTER -----");

		// Log all windows in Z-order
		EnumWindows(MyEnumWindowsProc, (LPARAM)nullptr);
	}
}

/*
** Moves the helper window to the reference position.
**
*/
void System::PrepareHelperWindow(HWND desktopIconsHostWindow)
{
	bool logging = GetRainmeter().GetDebug() && DEBUG_VERBOSE;

	SetWindowPos(c_Window, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);  // always on bottom

	if (c_ShowDesktop && desktopIconsHostWindow)
	{
		// Set WS_EX_TOPMOST flag
		SetWindowPos(c_HelperWindow, HWND_TOPMOST, 0, 0, 0, 0, ZPOS_FLAGS);

		// Find the "backmost" topmost window
		HWND hwnd = desktopIconsHostWindow;
		while (hwnd = ::GetNextWindow(hwnd, GW_HWNDPREV))
		{
			if (GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
			{
				WCHAR className[64], windowText[64];

				if (logging)
				{
					GetClassName(hwnd, className, 64);
					GetWindowText(hwnd, windowText, 64);

					SetLastError(ERROR_SUCCESS);
				}

				// Insert the helper window after the found window
				if (0 != SetWindowPos(c_HelperWindow, hwnd, 0, 0, 0, 0, ZPOS_FLAGS))
				{
					if (logging)
					{
						LogDebugF(L"System: HelperWindow: hwnd=0x%p (desktopIconsHostWindow=0x%p), hwndInsertAfter=0x%p (\"%s\" %s) - %s",
							c_HelperWindow, desktopIconsHostWindow, hwnd, windowText, className, (GetWindowLongPtr(c_HelperWindow, GWL_EXSTYLE) & WS_EX_TOPMOST) ? L"TOPMOST" : L"NORMAL");
					}
					return;
				}

				if (logging)
				{
					DWORD err = GetLastError();
					LogDebugF(L"System: HelperWindow: hwnd=0x%p (desktopIconsHostWindow=0x%p), hwndInsertAfter=0x%p (\"%s\" %s) - FAILED (ErrorCode=0x%08X)",
						c_HelperWindow, desktopIconsHostWindow, hwnd, windowText, className, err);
				}
			}
		}

		if (logging)
		{
			LogDebugF(L"System: HelperWindow: hwnd=0x%p (desktopIconsHostWindow=0x%p), hwndInsertAfter=HWND_TOPMOST - %s",
				c_HelperWindow, desktopIconsHostWindow, (GetWindowLongPtr(c_HelperWindow, GWL_EXSTYLE) & WS_EX_TOPMOST) ? L"TOPMOST" : L"NORMAL");
		}
	}
	else
	{
		// Insert the helper window to the bottom
		SetWindowPos(c_HelperWindow, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);

		if (logging)
		{
			LogDebugF(L"System: HelperWindow: hwnd=0x%p (desktopIconsHostWindow=0x%p), hwndInsertAfter=HWND_BOTTOM - %s",
				c_HelperWindow, desktopIconsHostWindow, (GetWindowLongPtr(c_HelperWindow, GWL_EXSTYLE) & WS_EX_TOPMOST) ? L"TOPMOST" : L"NORMAL");
		}
	}
}

/*
** Changes the "Show Desktop" state.
**
*/
bool System::CheckDesktopState(HWND desktopIconsHostWindow)
{
	HWND hwnd = nullptr;

	if (desktopIconsHostWindow && IsWindowVisible(desktopIconsHostWindow))
	{
		hwnd = FindWindowEx(nullptr, desktopIconsHostWindow, L"RainmeterSystem", L"System");
	}

	bool stateChanged = (hwnd && !c_ShowDesktop) || (!hwnd && c_ShowDesktop);

	if (stateChanged)
	{
		c_ShowDesktop = !c_ShowDesktop;

		if (GetRainmeter().GetDebug())
		{
			LogDebugF(L"System: \"Show %s\" has been detected.",
				c_ShowDesktop ? L"desktop" : L"open windows");
		}

		PrepareHelperWindow(desktopIconsHostWindow);

		ChangeZPosInOrder();

		if (c_ShowDesktop)
		{
			SetTimer(c_Window, TIMER_SHOWDESKTOP, INTERVAL_RESTOREWINDOWS, nullptr);
		}
		else
		{
			SetTimer(c_Window, TIMER_SHOWDESKTOP, INTERVAL_SHOWDESKTOP, nullptr);
		}
	}

	return stateChanged;
}

/*
** The event hook procedure
**
*/
void CALLBACK System::MyWinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
	if (event == EVENT_SYSTEM_FOREGROUND)
	{
		if (!c_ShowDesktop)
		{
			if (ShouldUseShellWindowAsDesktopIconsHost())
			{
				if (hwnd == GetDefaultShellWindow())
				{
					const int max = 5;
					int loop = 0;
					while (loop < max && !CheckDesktopState(hwnd))
					{
						Sleep(2);  // Wait for 2-16 ms before retrying
						++loop;
					}
				}

				return;
			}

			const int classLen = _countof(L"WorkerW") + 1;
			WCHAR className[classLen];
			if (GetClassName(hwnd, className, classLen) > 0 &&
				wcscmp(className, L"WorkerW") == 0 &&
				BelongToSameProcess(GetDefaultShellWindow(), hwnd))
			{
				const int max = 5;
				int loop = 0;
				while (loop < max && FindWindowEx(hwnd, nullptr, L"SHELLDLL_DefView", L"") == nullptr)
				{
					Sleep(2);  // Wait for 2-16 ms before retrying
					++loop;
				}

				if (loop < max)
				{
					loop = 0;
					while (loop < max && !CheckDesktopState(hwnd))
					{
						Sleep(2);  // Wait for 2-16 ms before retrying
						++loop;
					}
				}
			}
		}
	}
}

/*
** The window procedure
**
*/
LRESULT CALLBACK System::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (hWnd != c_Window)
	{
		if (uMsg == WM_WINDOWPOSCHANGING)
		{
			((LPWINDOWPOS)lParam)->flags |= SWP_NOZORDER;
			return 0;
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
	case WM_WINDOWPOSCHANGING:
		((LPWINDOWPOS)lParam)->flags |= SWP_NOZORDER;
		break;

	case WM_TIMER:
		switch (wParam)
		{
		case TIMER_SHOWDESKTOP:
			if (wParam == TIMER_SHOWDESKTOP)
			{
				CheckDesktopState(GetDesktopIconsHostWindow());
			}
			break;

		case TIMER_RESUME:
			KillTimer(hWnd, TIMER_RESUME);
			if (GetRainmeter().IsRedrawable())
			{
				auto iter = GetRainmeter().GetAllSkins().begin();
				for ( ; iter != GetRainmeter().GetAllSkins().end(); ++iter)
				{
					(*iter).second->UpdateWindowContents();
				}
			}
			break;
		}
		break;

	case WM_DISPLAYCHANGE:
		LogNotice(L"System: Display settings changed");
		MonitorUtil::ClearMultiMonitorInfo();
	case WM_SETTINGCHANGE:
		if (uMsg == WM_DISPLAYCHANGE || (/*uMsg == WM_SETTINGCHANGE &&*/ wParam == SPI_SETWORKAREA))
		{
			if (uMsg == WM_SETTINGCHANGE)  // SPI_SETWORKAREA
			{
				LogNotice(L"System: Work area changed");
				MonitorUtil::UpdateWorkareaInfo();
			}

			// Deliver WM_DISPLAYCHANGE / WM_SETTINGCHANGE message to all meter windows
			auto iter = GetRainmeter().GetAllSkins().begin();
			for ( ; iter != GetRainmeter().GetAllSkins().end(); ++iter)
			{
				PostMessage((*iter).second->GetWindow(), WM_METERWINDOW_DELAYED_MOVE, (WPARAM)uMsg, (LPARAM)0);
			}
		}
		break;

	case WM_TIMECHANGE:
		// Update the CRT timezone variables.
		_tzset();
		break;

	case WM_POWERBROADCAST:
		if (wParam == PBT_APMRESUMESUSPEND)
		{
			// Deliver PBT_APMRESUMESUSPEND event to all meter windows
			SetTimer(hWnd, TIMER_RESUME, INTERVAL_RESUME, nullptr);
		}
		return TRUE;

	case WM_WTSSESSION_CHANGE:
		LogDebugF(L"System: User session change detected! Session ID: 0x%08X Type: 0x%08X", lParam, wParam);
		if (GetRainmeter().IsRedrawable())
		{
			auto iter = GetRainmeter().GetAllSkins().begin();
			for (; iter != GetRainmeter().GetAllSkins().end(); ++iter)
			{
				(*iter).second->UpdateWindowContents();
			}
		}
		break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

/*
** Gets the cursor position in last message retrieved by GetMessage().
**
*/
POINT System::GetCursorPosition()
{
	DWORD pos = GetMessagePos();
	POINT pt = { GET_X_LPARAM(pos), GET_Y_LPARAM(pos) };
	return pt;
}

/*
** Checks if file is writable.
**
*/
bool System::IsFileWritable(LPCWSTR file)
{
	HANDLE hFile = CreateFile(file, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	CloseHandle(hFile);
	return true;
}

/*
** This function is a wrapper function for LoadLibrary().
**
** Avoids loading a DLL from current directory.
**
*/
HMODULE System::RmLoadLibrary(LPCWSTR lpLibFileName, DWORD* dwError)
{
	// Remove current directory from DLL search path
	SetDllDirectory(L"");

	SetLastError(ERROR_SUCCESS);
	HMODULE hLib = LoadLibrary(lpLibFileName);

	if (dwError)
	{
		*dwError = GetLastError();
	}

	return hLib;
}

/*
** Resets working directory to default.
**
*/
void System::ResetWorkingDirectory()
{
	WCHAR directory[MAX_PATH] = { 0 };
	GetCurrentDirectory(MAX_PATH, directory);

	const WCHAR* workDir = c_WorkingDirectory.c_str();
	if (_wcsicmp(directory, workDir) != 0)
	{
		SetCurrentDirectory(workDir);
	}
}

/*
** Initializes a critical section object by using InitializeCriticalSectionEx function with CRITICAL_SECTION_NO_DEBUG_INFO flag.
** For more details: http://stackoverflow.com/questions/804848/critical-sections-leaking-memory-on-vista-win2008/
**
*/
void System::InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	if (InitializeCriticalSectionEx(lpCriticalSection, 0UL, CRITICAL_SECTION_NO_DEBUG_INFO) == TRUE)
	{
		return;
	}

	// The following should "always succeed" according to:
	// https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializecriticalsectionandspincount
	if (InitializeCriticalSectionAndSpinCount(lpCriticalSection, 0UL) == TRUE)
	{
		return;
	}

	// error?
}

/*
** Sets clipboard text to given string.
**
*/
void System::SetClipboardText(const std::wstring& text)
{
	if (OpenClipboard(nullptr))
	{
		// Include terminating null char
		size_t len = text.length() + 1ULL;

		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(WCHAR));
		if (hMem)
		{
			LPVOID data = GlobalLock(hMem);
			if (data)
			{
				memcpy(data, text.c_str(), len * sizeof(WCHAR));
				GlobalUnlock(hMem);

				EmptyClipboard();
				if (!SetClipboardData(CF_UNICODETEXT, hMem))
				{
					GlobalFree(hMem);
				}
			}
		}

		CloseClipboard();
	}
}

/*
** Sets the system wallpapar.
**
*/
void System::SetWallpaper(const std::wstring& wallpaper, const std::wstring& style)
{
	if (!wallpaper.empty())
	{
		if (_waccess_s(wallpaper.c_str(), 0) != 0)
		{
			LogErrorF(L"!SetWallpaper: Unable to read file: %s", wallpaper.c_str());
			return;
		}

		std::wstring file = GetRainmeter().GetSettingsPath() + L"Wallpaper.bmp";
		if (ConvertImageToBmpFile(wallpaper, file))
		{
			if (!style.empty())
			{
				HKEY hKey;
				if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
				{
					const WCHAR* wallStyle = nullptr;
					const WCHAR* wallTile = L"0";

					const WCHAR* option = style.c_str();
					if (_wcsicmp(option, L"CENTER") == 0)
					{
						wallStyle = L"0";
					}
					else if (_wcsicmp(option, L"TILE") == 0)
					{
						wallStyle = L"0";
						wallTile = L"1";
					}
					else if (_wcsicmp(option, L"STRETCH") == 0)
					{
						wallStyle = L"2";
					}
					if (_wcsicmp(option, L"FIT") == 0)
					{
						wallStyle = L"6";
					}
					else if (_wcsicmp(option, L"FILL") == 0)
					{
						wallStyle = L"10";
					}
					else if (_wcsicmp(option, L"SPAN") == 0)
					{
						wallStyle = L"22";
					}

					if (wallStyle)
					{
						RegSetValueEx(hKey, L"WallpaperStyle", 0, REG_SZ, (const BYTE*)wallStyle, sizeof(WCHAR) * 2);
						RegSetValueEx(hKey, L"TileWallpaper", 0, REG_SZ, (const BYTE*)wallTile, sizeof(WCHAR) * 2);
					}
					else
					{
						LogError(L"!SetWallpaper: Invalid style");
					}

					RegCloseKey(hKey);
				}
			}

			SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (void*)file.c_str(), SPIF_UPDATEINIFILE);
		}
	}
}

/*
** Copies files and folders from one location to another.
**
*/
bool System::CopyFiles(std::wstring from, std::wstring to, bool bMove)
{
	// If given "from" path ends with path separator, remove it (Workaround for XP: error code 1026)
	size_t len;
	while (len = from.size(), len > 0 && PathUtil::IsSeparator(from[len - 1]))
	{
		from.resize(len - 1);
	}

	// The strings must end with double \0
	from.append(1, L'\0');
	to.append(1, L'\0');

	SHFILEOPSTRUCT fo =
	{
		nullptr,
		(UINT)(bMove ? FO_MOVE : FO_COPY),
		from.c_str(),
		to.c_str(),
		FOF_NO_UI | FOF_NOCONFIRMATION | FOF_ALLOWUNDO
	};

	int result = SHFileOperation(&fo);
	if (result != 0)
	{
		LogErrorF(L"Copy error: From %s to %s (%i)", from.c_str(), to.c_str(), result);
		return false;
	}
	return true;
}

/*
** Copies files and folders from one location to another ONLY if the destination does not exist.
**
*/
bool System::CopyFilesWithNoCollisions(std::wstring from, const std::wstring& to)
{
	auto checkDir = [](LPCWSTR str) -> bool
	{
		if (PathIsDirectory(str) == FALSE)
		{
			LogErrorF(L"Copy error: \"%s\" muse be a folder.", str);
			return false;
		}
		return true;
	};
	if (!checkDir(from.c_str())) return false;
	if (!checkDir(to.c_str())) return false;

	PathUtil::AppendBackslashIfMissing(from);
	const std::wstring::size_type len = from.length();

	std::queue<std::wstring> dirs;
	dirs.push(from);

	while (!dirs.empty())
	{
		from = dirs.front();
		dirs.pop();

		PathUtil::AppendBackslashIfMissing(from);

		std::wstring spec = from;
		spec.append(1, L'*');

		WIN32_FIND_DATA fd = { 0 };
		HANDLE find = FindFirstFileEx(spec.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
		if (find != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;

				std::wstring fromFile = from;
				fromFile.append(fd.cFileName);

				std::wstring toFile = to;
				PathUtil::AppendBackslashIfMissing(toFile);
				toFile.append(from.substr(len));
				toFile.append(fd.cFileName);

				if (_waccess_s(toFile.c_str(), 0) != 0)
				{
					System::CopyFiles(fromFile, toFile);
				}

				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					dirs.push(fromFile);
				}

			} while (FindNextFile(find, &fd));

			FindClose(find);
			find = INVALID_HANDLE_VALUE;
		}
	}

	return true;
}

/*
** Removes a file even if a file is read-only.
**
*/
bool System::RemoveFile(const std::wstring& file)
{
	DWORD attr = GetFileAttributes(file.c_str());
	if (attr == -1 || (attr & FILE_ATTRIBUTE_READONLY))
	{
		// Unset read-only
		SetFileAttributes(file.c_str(), (attr == -1) ? FILE_ATTRIBUTE_NORMAL : attr - FILE_ATTRIBUTE_READONLY);
	}

	return (DeleteFile(file.c_str()) != 0);
}

/*
** Recursively removes folder.
**
*/
bool System::RemoveFolder(std::wstring folder)
{
	// The strings must end with double nul
	folder.append(1, L'\0');

	SHFILEOPSTRUCT fo =
	{
		nullptr,
		FO_DELETE,
		folder.c_str(),
		nullptr,
		FOF_NO_UI | FOF_NOCONFIRMATION | FOF_ALLOWUNDO
	};

	int result = SHFileOperation(&fo);
	if (result != 0)
	{
		LogErrorF(L"Unable to delete folder %s (%i)", folder.c_str(), result);
		return false;
	}
	return true;
}

/*
** Retrieves the "IniFileMapping" entries from Registry.
**
*/
void System::UpdateIniFileMappingList()
{
	static ULONGLONG s_LastWriteTime = 0ULL;

	HKEY hKey = nullptr;
	LONG ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping", 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hKey);
	if (ret == ERROR_SUCCESS)
	{
		DWORD numSubKeys = 0UL;
		ULONGLONG ftLastWriteTime = 0ULL;
		bool changed = false;

		ret = RegQueryInfoKey(hKey, nullptr, nullptr, nullptr, &numSubKeys, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, (LPFILETIME)&ftLastWriteTime);
		if (ret == ERROR_SUCCESS)
		{
			//LogDebugF(L"IniFileMapping: numSubKeys=%u, ftLastWriteTime=%llu", numSubKeys, ftLastWriteTime);

			if (ftLastWriteTime != s_LastWriteTime ||
				numSubKeys != c_IniFileMappings.size())
			{
				s_LastWriteTime = ftLastWriteTime;
				if (numSubKeys > c_IniFileMappings.capacity())
				{
					c_IniFileMappings.reserve(numSubKeys);
				}
				changed = true;
			}
		}
		else
		{
			s_LastWriteTime = 0;
			changed = true;
		}

		if (changed)
		{
			if (!c_IniFileMappings.empty())
			{
				c_IniFileMappings.clear();
			}

			WCHAR* buffer = new WCHAR[MAX_PATH];
			DWORD index = 0, cch = MAX_PATH;

			while ((ret = RegEnumKeyEx(hKey, index++, buffer, &cch, nullptr, nullptr, nullptr, nullptr)) != ERROR_NO_MORE_ITEMS)
			{
				if (ret == ERROR_SUCCESS)
				{
					c_IniFileMappings.push_back(buffer);
				}
				cch = MAX_PATH;
			}

			delete [] buffer;
			buffer = nullptr;
		}

		RegCloseKey(hKey);
		hKey = nullptr;
	}
}

/*
** Prepares a temporary file if iniFile is included in the "IniFileMapping" entries.
** If iniFile is not included, returns a empty string. If error occurred, returns "?".
** Note that a temporary file must be deleted by caller.
**
*/
std::wstring System::GetTemporaryFile(const std::wstring& iniFile)
{
	std::wstring temporary;

	if (!c_IniFileMappings.empty())
	{
		std::wstring::size_type pos = iniFile.find_last_of(L"\\/");
		const WCHAR* filename = iniFile.c_str() + ((pos != std::wstring::npos) ? pos + 1 : 0);

		std::vector<std::wstring>::const_iterator iter = c_IniFileMappings.begin();
		for ( ; iter != c_IniFileMappings.end(); ++iter)
		{
			if (_wcsicmp((*iter).c_str(), filename) == 0)
			{
				WCHAR* buffer = new WCHAR[MAX_PATH];

				if (GetTempPath(MAX_PATH, buffer) != 0 &&
					GetTempFileName(buffer, L"cfg", 0, buffer) != 0)
				{
					temporary = buffer;

					std::wstring tmp = GetTemporaryFile(temporary);
					if (!tmp.empty() || !CopyFiles(iniFile, temporary))  // temporary is reserved or failed
					{
						RemoveFile(temporary);

						if (tmp.empty())
						{
							temporary = L"?";
						}
						else
						{
							temporary.swap(tmp);
						}
					}
				}
				else  // failed
				{
					LogErrorF(L"Unable to create temporary file to: %s", temporary.c_str());
					temporary = L"?";
				}

				delete [] buffer;
				buffer = nullptr;
				break;
			}
		}
	}

	return temporary;
}

bool System::IsProcessRunningCached(const std::wstring& lowercaseName)
{
	static ankerl::unordered_dense::set<std::wstring> s_Processes;
	static ULONGLONG s_LastUpdateTickCount = 0ULL;
	const ULONGLONG updateInterval = 250ULL; // ms

	ULONGLONG tickCount = GetTickCount64();
	if (tickCount >= (s_LastUpdateTickCount + updateInterval))
	{
		s_LastUpdateTickCount = tickCount;

		s_Processes = {};
		HANDLE thSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (thSnapshot != INVALID_HANDLE_VALUE)
		{
			PROCESSENTRY32 processEntry = { sizeof(processEntry) };
			if (Process32First(thSnapshot, &processEntry))
			{
				do
				{
					std::wstring name = processEntry.szExeFile;
					StringUtil::ToLowerCase(name);
					s_Processes.insert(name);
				} while (Process32Next(thSnapshot, &processEntry));
			}
			CloseHandle(thSnapshot);
		}
	}

	return s_Processes.count(lowercaseName) != 0;
}
