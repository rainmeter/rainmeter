/*
  Copyright (C) 2010-2012 Birunthan Mohanathas <http://poiru.net>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "StdAfx.h"
#include "PluginInterface.h"
#include "Version.h"

NppData nppData;
const TCHAR NPP_PLUGIN_NAME[] = TEXT("RainLexer");
const int nbFunc = 3;

namespace RainLexer {

const int WM_QUERY_RAINMETER = WM_APP + 1000;
const int RAINMETER_QUERY_ID_SKINS_PATH = 4101;

HWND g_RainmeterWindow = nullptr;
//HWND g_NppWindow = nullptr;
WCHAR g_SkinsPath[MAX_PATH]{};

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COPYDATA)
	{
		auto cds = reinterpret_cast<COPYDATASTRUCT*>(lParam);
		if (cds->dwData == RAINMETER_QUERY_ID_SKINS_PATH)
		{
			wcsncpy(g_SkinsPath, static_cast<const WCHAR*>(cds->lpData), _countof(g_SkinsPath));
			g_SkinsPath[_countof(g_SkinsPath) - 1] = L'\0';
		}

		return TRUE;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

bool GetRainmeter()
{
	if (g_RainmeterWindow == nullptr || IsWindow(g_RainmeterWindow) == 0)
	{
		HWND trayWindow = FindWindow(L"RainmeterTrayClass", nullptr);
		HWND meterWindow = FindWindow(L"RainmeterMeterWindow", nullptr);
		if (trayWindow != nullptr && meterWindow != nullptr)
		{
			// Create window to recieve WM_COPYDATA from Rainmeter
			HWND wnd = CreateWindow(
				L"STATIC",
				L"",
				WS_DISABLED,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				nullptr,
				nullptr,
				nullptr,
				nullptr);

			if (wnd != nullptr)
			{
				SetWindowLongPtr(wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc));

				SendMessage(trayWindow, WM_QUERY_RAINMETER, RAINMETER_QUERY_ID_SKINS_PATH, reinterpret_cast<LPARAM>(wnd));
				DestroyWindow(wnd);

				if (*g_SkinsPath != NULL)
				{
					g_RainmeterWindow = meterWindow;
					return true;
				}
			}
		}
	}
	else
	{
		return true;
	}

	return false;
}

void RefreshSkin()
{
	if (!GetRainmeter())
	{
		return;
	}

	WCHAR currentPath[MAX_PATH]{};
	auto ret = static_cast<BOOL>(SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, MAX_PATH, reinterpret_cast<LPARAM>(&currentPath)));

	if (ret > 0)
	{
		const size_t skinsPathLen = wcslen(g_SkinsPath);
		const size_t currentPathLen = wcslen(currentPath);

		// Make sure the file is in the skins folder and that extension is .ini
		if (wcsncmp(g_SkinsPath, currentPath, skinsPathLen) == 0 &&
			_wcsicmp(&currentPath[currentPathLen - 4], L".ini") == 0)
		{
			WCHAR* relativePath = &currentPath[skinsPathLen];
			WCHAR* pos = wcsrchr(relativePath, L'\\');
			if (pos != nullptr)
			{
				relativePath[pos - relativePath] = L'\0';
				WCHAR buffer[512]{};
				const int len = _snwprintf(
					buffer, _countof(buffer), L"!Refresh \"%s\"", relativePath);
				buffer[_countof(buffer) - 1] = L'\0';

				COPYDATASTRUCT cds{};
				cds.dwData = 1;
				cds.cbData = static_cast<DWORD>(len + 1) * sizeof(WCHAR);
				cds.lpData = static_cast<void*>(buffer);
				SendMessage(g_RainmeterWindow, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds));
			}
		}
	}
}

void RefreshAll()
{
	if (!GetRainmeter())
	{
		return;
	}

	COPYDATASTRUCT cds{};
	cds.dwData = 1;
	cds.cbData = sizeof(L"!Refresh *");
	cds.lpData = (PVOID)L"!Refresh *";
	SendMessage(g_RainmeterWindow, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds));
}

void About()
{
	MessageBox(
		nppData._nppHandle,
		L"By Birunthan Mohanathas.\n"
		L"https://github.com/rainmeter/rainlexer",
		RAINLEXER_TITLE,
		MB_OK);
}

FuncItem funcItem[nbFunc] =
{
	{ L"Refresh skin", RefreshSkin, 0, false, nullptr },
	{ L"Refresh all", RefreshAll, 0, false, nullptr },
	{ L"&About...", About, 0, false, nullptr }
};

HMODULE GetCurrentModule()
{
	HMODULE hModule = nullptr;
	GetModuleHandleEx(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
		reinterpret_cast<LPCTSTR>(GetCurrentModule),
		&hModule);

	return hModule;
}

void addToolbarIcon()
{
	UINT style = (LR_SHARED | LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS);

	auto dpi = GetDeviceCaps(GetWindowDC(nppData._nppHandle), LOGPIXELSX);
	int size = 16 * dpi / 96;

	auto hInstance = static_cast<HINSTANCE>(GetCurrentModule());

	toolbarIconsWithDarkMode tbRefresh{};
	tbRefresh.hToolbarIcon = static_cast<HICON>(::LoadImage(hInstance, MAKEINTRESOURCE(IDI_RAINMETER), IMAGE_ICON, size, size, style));
	tbRefresh.hToolbarIconDarkMode = tbRefresh.hToolbarIcon;

	ICONINFO iconinfo{};
	GetIconInfo(tbRefresh.hToolbarIcon, &iconinfo);
	tbRefresh.hToolbarBmp = iconinfo.hbmColor;

	::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE, funcItem[0]._cmdID, reinterpret_cast<LPARAM>(&tbRefresh));
}

//
// Notepad++ exports
//

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int* nbF)
{
	*nbF = nbFunc;
	return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification * notifyCode)
{
	switch (notifyCode->nmhdr.code)
	{
	case NPPN_SHUTDOWN:
	{
		//commandMenuCleanUp();
		break;
	}
	case NPPN_TBMODIFICATION: {
		addToolbarIcon();
		break;
	}

	default:
		return;
	}
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT /*Message*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{/*
	if (Message == WM_MOVE)
	{
		::MessageBox(NULL, "move", "", MB_OK);
	}
*/
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
	return TRUE;
}
#endif //UNICODE

}	// namespace RainLexer
