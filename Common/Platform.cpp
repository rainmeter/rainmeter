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
#include <string>

namespace Platform {

LPCWSTR GetPlatformName()
{
	bool isServer = IsWindowsServer();

	// Note: Place newer versions at the top.

	if (IsWindows8Point1OrGreater())	return isServer ? L"Windows Server 2012 R2" : L"Windows 8.1";
	if (IsWindows8OrGreater())			return isServer ? L"Windows Server 2012" : L"Windows 8";
	if (IsWindows7OrGreater())			return isServer ? L"Windows Server 2008 R2" : L"Windows 7";
	if (IsWindowsVistaOrGreater())		return isServer ? L"Windows Server 2008" : L"Windows Vista";
	if (IsWindowsXPOrGreater())			return isServer ? L"Windows Server 2003" : L"Windows XP";

	return L"Unknown";
}

LPCWSTR GetPlatformFriendlyName()
{
	static std::wstring name;
	
	WCHAR* buffer = new WCHAR[MAX_LINE_LENGTH];
	DWORD size = MAX_LINE_LENGTH;

	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hKey, L"ProductName", nullptr, nullptr, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS)
		{
			bool is64Bit = false;
			bool has64Bit = GetPlatformBit(is64Bit);

			name += buffer;
			name += L' ';
			name += (has64Bit) ? (is64Bit) ? L" 64" : L" 32" : L" ???";
			name += L"-bit";

			size = MAX_LINE_LENGTH;
			if (RegQueryValueEx(hKey, L"CurrentBuildNumber", nullptr, nullptr, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS)
			{
				name += L" (build ";
				name += buffer;
				name += L')';
			}
			else if (RegQueryValueEx(hKey, L"CurrentBuild", nullptr, nullptr, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS)
			{
				name += L" (build ";
				name += buffer;
				name += L')';
			}

			size = MAX_LINE_LENGTH;
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

	delete [] buffer;

	return name.c_str();
}

/*
** Returns |true| if the OS architecture can be found (either 32-bit or 64-bit),
**  or |false| if it cannot be determined.
**
** Note: IsWow64Process was introduced with Windows XP SP2.
*/
bool GetPlatformBit(bool& is64Bit)
{
#if _WIN64

	is64Bit = true;
	return true;

#elif _WIN32

	BOOL isWow64 = FALSE;

	LPFN_ISWOW64PROCESS fnIsWow64Process = 
		(LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(L"kernel32"), "IsWow64Process");

	if (fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &isWow64))
		{
			return false;
		}

		if (isWow64)
		{
			is64Bit = true;
		}
		else
		{
			is64Bit = false;
		}

		return true;
	}
	else
	{
		return false;
	}

#else

	return false;

#endif
}

}  // namespace Platform