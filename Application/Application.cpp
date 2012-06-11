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
#include "../Library/Rainmeter.h"

#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

/*
** Entry point.
**
*/
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(000);

	HRSRC iconResource = FindResource(hInstance, MAKEINTRESOURCE(1), RT_ICON);
	if (iconResource)
	{
		// Call RainmeterMain from Rainmeter.dll. Since Rainmeter.dll is delay-loaded, this will cause
		// crash if Rainmeter.dll is not found.
		return RainmeterMain(lpCmdLine);
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
				wcscat(buffer, L"\\Rainmeter.exe");
				ShellExecute(NULL, L"open", buffer, lpCmdLine, NULL, SW_SHOWNORMAL);
			}
			RegCloseKey(hKey);
		}

		return 0;
	}
}
