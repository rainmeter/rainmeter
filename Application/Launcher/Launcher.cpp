/*
  Copyright (C) 2012 Birunthan Mohanathas

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

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShellAPI.h>
#include "../../Library/Rainmeter.h"

/*
** Entry point.
**
*/
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Rainmeter", 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS)
	{
		const DWORD size = MAX_PATH;
		WCHAR buffer[size];
		DWORD type = 0;
		if (RegQueryValueEx(hKey, NULL , NULL, &type, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS && type == REG_SZ)
		{
			SetCurrentDirectory(buffer);
			wcscat(buffer, L"\\Rainmeter.exe");
			ShellExecute(NULL, L"open", buffer, lpCmdLine, NULL, SW_SHOWNORMAL);
		}
		RegCloseKey(hKey);
	}

	return 0;
}
