/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Platform.h"

namespace Platform {

namespace {

bool IsWin11()
{
	// Temporary Windows 11 check
	static bool s_IsWin11 = []() -> bool
	{
		if (IsWindows10OrGreater())
		{
			WCHAR buffer[256] = { 0 };
			DWORD size = _countof(buffer);
			int buildNumber = 0;

			HKEY hKey;
			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion", 0UL, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
			{
				if (RegQueryValueEx(hKey, L"CurrentBuildNumber", nullptr, nullptr, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS)
				{
					buildNumber = _wtoi(buffer);
				}
				RegCloseKey(hKey);
			}

			// |GetTempPath2W| doesn't exist in Windows version prior to Windows 11 (as of yet)
			typedef void* (__stdcall* TempPath2)();
			TempPath2 tmpPath2 = (TempPath2)GetProcAddress(GetModuleHandle(L"kernel32"), "GetTempPath2W");

			return tmpPath2 && buildNumber >= 22000;
		}
		return false;
	} ();
	return s_IsWin11;
}

}  // namespace

LPCWSTR GetPlatformName()
{
	static std::wstring s_Name = []() -> std::wstring
	{
		const bool isServer = IsWindowsServer();
		std::wstring releaseID = GetPlatformReleaseID();

		// Note: Place newer versions at the top.
		const WCHAR* version =
			IsWin11() ? L"11" :		// Temporary hack
			IsWindows10OrGreater() ? (isServer ? (releaseID == L"1809" ? L"2019" : L"2016") : L"10") :
			IsWindows8Point1OrGreater() ? (isServer ? L"2012 R2" : L"8.1") :
			IsWindows8OrGreater() ? (isServer ? L"2012" : L"8") :
			IsWindows7OrGreater() ? (isServer ? L"2008 R2" : L"7") :
			nullptr;  // Unknown
		if (version)
		{
			std::wstring name = L"Windows ";
			name += isServer ? L"Server " : L"";
			name += version;
			return name;
		}

		return L"Unknown";
	} ();
	return s_Name.c_str();
}

std::wstring GetPlatformReleaseID()
{
	static std::wstring s_ID = []()->std::wstring
	{
		std::wstring id;

		if (IsWindows10OrGreater())
		{
			WCHAR buffer[10] = { 0 };
			DWORD size = _countof(buffer);

			HKEY hKey;
			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion", 0UL, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
			{
				if (RegQueryValueEx(hKey, L"ReleaseId", nullptr, nullptr, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS)
				{
					id = buffer;
				}
				RegCloseKey(hKey);
			}
		}
		return id;
	} ();
	return s_ID;  // Can be empty!
}

std::wstring GetPlatformFriendlyName()
{
	std::wstring name;

	WCHAR buffer[256] = { 0 };
	DWORD size = _countof(buffer);

	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion", 0UL, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hKey, L"ProductName", nullptr, nullptr, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS)
		{
			name = buffer;
			if (IsWin11())  // Temporary hack
			{
				size_t pos = name.find(L"Windows 10");
				if (pos != std::wstring::npos)
				{
					name.replace(pos, 10ULL, L"Windows 11");
				}
			}

			// For Windows 10 (and above?), use the "ReleaseId" as part of the version number.
			// (ie. 1507, 1511, 1607, 1703, 1709, 1803, 1809, 1903, 1909, 2004, 2009, ...)
			std::wstring id = GetPlatformReleaseID();
			if (!id.empty())
			{
				name += L' ';
				name += id;
			}

			name += Is64BitWindows() ? L" 64-bit" : L" 32-bit";

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

std::wstring GetPlatformUserLanguage()
{
	LANGID id = GetUserDefaultUILanguage();
	LCID lcid = MAKELCID(id, SORT_DEFAULT);
	WCHAR buffer[LOCALE_NAME_MAX_LENGTH];
	if (GetLocaleInfo(lcid, LOCALE_SENGLISHLANGUAGENAME, buffer, _countof(buffer)) == 0)
	{
		_snwprintf_s(buffer, _TRUNCATE, L"%s", L"<error>");
	}
	return std::wstring(buffer);
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
