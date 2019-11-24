//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include "menuCmdID.h"

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE /*hModule*/)
{
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{

    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );

    // use getFuncsArray instead to follow iso c++11 guidelines (literal to char*)
    //setCommand(0, TEXT("Refresh skin"), RefreshSkin, nullptr, false);
    //setCommand(1, TEXT("Refresh all"), RefreshAll, nullptr, false);
    //setCommand(2, TEXT("About..."), About, nullptr, false);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
    // Don't forget to deallocate your shortcut here
}

//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR* cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey* sk, bool check0nInit)
{
    if (index >= nbFunc)
    {
        return false;
    }

    if (pFunc == nullptr)
    {
        return false;
    }

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//

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

const int WM_QUERY_RAINMETER = WM_APP + 1000;
const int RAINMETER_QUERY_ID_SKINS_PATH = 4101;

HWND g_RainmeterWindow = nullptr;
WCHAR g_SkinsPath[MAX_PATH] = { 0 };

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
        HWND trayWindow = FindWindowW(L"RainmeterTrayClass", nullptr);
        HWND meterWindow = FindWindowW(L"RainmeterMeterWindow", nullptr);
        if (trayWindow != nullptr && meterWindow != nullptr)
        {
            // Create window to recieve WM_COPYDATA from Rainmeter
            HWND wnd = CreateWindowW(
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
                SetWindowLongPtrW(wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc));

                SendMessageW(trayWindow, WM_QUERY_RAINMETER, RAINMETER_QUERY_ID_SKINS_PATH, reinterpret_cast<LPARAM>(wnd));
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

    WCHAR currentPath[MAX_PATH];
    auto ret = static_cast<BOOL>(SendMessageW(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, MAX_PATH, reinterpret_cast<LPARAM>(&currentPath)));

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
                WCHAR buffer[512];
                const int len = _snwprintf(
                    buffer, _countof(buffer), L"!Refresh \"%s\"", relativePath);
                buffer[_countof(buffer) - 1] = L'\0';

                COPYDATASTRUCT cds;
                cds.dwData = 1;
                cds.cbData = static_cast<DWORD>(len + 1) * sizeof(WCHAR);
                cds.lpData = static_cast<void*>(buffer);
                SendMessageW(g_RainmeterWindow, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds));
            }
        }
    }
}

void RefreshAll()
{
    if (GetRainmeter())
    {
        wchar_t command[] = L"!Refresh *";
        COPYDATASTRUCT cds;
        cds.dwData = 1;
        cds.cbData = sizeof(command);
        cds.lpData = command;
        SendMessageW(g_RainmeterWindow, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds));
    }
}

void About()
{
    MessageBoxW(
        nppData._nppHandle,
        L"By Birunthan Mohanathas.\n"
        L"poiru.github.com/rainlexer",
        RAINLEXER_TITLE,
        MB_OK);
}