/*
  Copyright (C) 2013 Birunthan Mohanathas

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

#include "StdAfx.h"
#include "Platform.h"

namespace Platform {

LPCWSTR GetPlatformName()
{
	const bool isServer = IsWindowsServer();

	// Note: Place newer versions at the top.

	if (IsWindows8Point1OrGreater())  return isServer ? L"Windows Server 2012 R2" : L"Windows 8.1";
	if (IsWindows8OrGreater())        return isServer ? L"Windows Server 2012" : L"Windows 8";
	if (IsWindows7OrGreater())        return isServer ? L"Windows Server 2008 R2" : L"Windows 7";
	if (IsWindowsVistaOrGreater())    return isServer ? L"Windows Server 2008" : L"Windows Vista";
	if (IsWindowsXPOrGreater())       return isServer ? L"Windows Server 2003" : L"Windows XP";

	return L"Unknown";
}

std::wstring GetPlatformFriendlyName()
{
	std::wstring name;

	WCHAR buffer[256];
	DWORD size = _countof(buffer);

	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hKey, L"ProductName", nullptr, nullptr, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS)
		{
			name += buffer;
			name += L' ';
			name += Is64BitWindows() ? L"64" : L"32";
			name += L"-bit";

			size = _countof(buffer);
			if (RegQueryValueEx(hKey, L"CurrentBuildNumber", nullptr, nullptr, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS ||
				RegQueryValueEx(hKey, L"CurrentBuild", nullptr, nullptr, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS)
			{
				name += L" (build ";
				name += buffer;
				name += L')';
			}

			size = _countof(buffer);
			if (RegQueryValueEx(hKey, L"CSDVersion", nullptr, nullptr, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS)
			{
				name += L' ';
				name += buffer;
			}
		}

		RegCloseKey(hKey);
	}
	else
	{
		name = L"Windows version unknown";
	}

	return name;
}

/*
** Returns |true| if running on 64-bit Windows.
*/
bool Is64BitWindows()
{
#if _WIN64
	return true;
#endif

	auto isWow64Process = (decltype(IsWow64Process)*)GetProcAddress(GetModuleHandle(L"kernel32"), "IsWow64Process");
	if (isWow64Process)
	{
		BOOL isWow64 = FALSE;
		return isWow64Process(GetCurrentProcess(), &isWow64) && isWow64;
	}

	return false;
}

}  // namespace Platform