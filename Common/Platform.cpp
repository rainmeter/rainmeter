/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Platform.h"

namespace
{

std::wstring& GetBuildNumberFromRegistry()
{
	static std::wstring s_BuildNumber = []() -> std::wstring
	{
		std::wstring buildNumber = L"0";

		HKEY hkey = nullptr;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion", 0UL, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
		{
			WCHAR buffer[10] = { 0 };
			DWORD size = _countof(buffer);

			if (RegQueryValueEx(hkey, L"CurrentBuildNumber", nullptr, nullptr, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS ||
				RegQueryValueEx(hkey, L"CurrentBuild", nullptr, nullptr, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS)
			{
				buildNumber = buffer;
			}
			RegCloseKey(hkey);
			hkey = nullptr;
		}

		return buildNumber;
	} ();

	return s_BuildNumber;
}

};

inline bool IsWindows11OrGreater()
{
	static bool s_IsWindows11OrGreater = []() -> bool
	{
		if (!IsWindows10OrGreater()) return false;

		// |GetTempPath2W| doesn't exist in Windows version prior to Windows 11 (as of yet)
		typedef void* (__stdcall* TempPath2)();
		HMODULE hmod = GetModuleHandle(L"kernel32");
		if (!hmod) return false;

		TempPath2 tmpPath2 = (TempPath2)GetProcAddress(hmod, "GetTempPath2W");

		int buildNumber = _wtoi(GetBuildNumberFromRegistry().c_str());
		return tmpPath2 && (buildNumber >= 22000);
	} ();

	return s_IsWindows11OrGreater;
}

Platform::Platform()
{
	Initialize();
}

Platform::~Platform()
{
}

Platform& Platform::GetInstance()
{
	static Platform s_Platform;
	return s_Platform;
}

void Platform::Initialize()
{
	// Is Windows 64 bit?
	m_Is64BitWindows = [&]() -> bool
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
	} ();

	std::wstring ubrStr;
	std::wstring servicePack;

	HKEY hkey = nullptr;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion", 0UL, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
	{
		WCHAR buffer[256] = { 0 };
		DWORD size = _countof(buffer);

		// Get Windows 10/11 specific values
		if (IsWindows10OrGreater())
		{
			// Prefer "DisplayVersion" over "ReleaseId"
			if ((RegQueryValueEx(hkey, L"DisplayVersion", nullptr, nullptr, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS) ||
				(RegQueryValueEx(hkey, L"ReleaseId", nullptr, nullptr, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS))
			{
				m_DisplayVersion = buffer;
			}
		}

		size = _countof(buffer);
		if (RegQueryValueEx(hkey, L"ProductName", nullptr, nullptr, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS)
		{
			m_ProductName = buffer;

			if (IsWindows11OrGreater() && !m_DisplayVersion.empty())
			{
				size_t pos = m_ProductName.find(L"Windows 10");
				if (_wcsnicmp(L"Windows 10", m_ProductName.c_str(), 10ULL) == 0)
				{
					m_ProductName.replace(pos, 10ULL, L"Windows 11");
				}
			}
		}

		DWORD ubr = 0UL;
		size = sizeof(DWORD);
		if (RegQueryValueEx(hkey, L"UBR", nullptr, nullptr, (LPBYTE)&ubr, &size) == ERROR_SUCCESS && ubr > 0UL)
		{
			ubrStr = L'.';
			ubrStr += std::to_wstring(ubr);
		}

		size = _countof(buffer);
		if (RegQueryValueEx(hkey, L"CSDVersion", nullptr, nullptr, (LPBYTE)buffer, (LPDWORD)&size) == ERROR_SUCCESS)
		{
			servicePack = buffer;
		}

		RegCloseKey(hkey);
		hkey = nullptr;
	}

	// Retrieve build number
	m_BuildNumber = GetBuildNumberFromRegistry();

	// Set Name
	const bool isServer = IsWindowsServer();
	m_Name = isServer ? L"Windows Server " : L"Windows ";
	m_Name += [&]() -> LPCWSTR
	{
		return
			IsWindows11OrGreater() ? L"11" :
			IsWindows10OrGreater() ? (isServer ?
				(m_DisplayVersion == L"21H2" ? L"2022" :
				(m_DisplayVersion == L"1809" ? L"2019" : L"2016")) : L"10") :
			IsWindows8Point1OrGreater() ? (isServer ? L"2012 R2" : L"8.1") :
			IsWindows8OrGreater() ? (isServer ? L"2012" : L"8") :
			IsWindows7OrGreater() ? (isServer ? L"2008 R2" : L"7") :
			L"Unknown";
	} ();

	// Set "friendly" name
	m_FriendlyName = m_ProductName;
	if (!m_DisplayVersion.empty())
	{
		m_FriendlyName += L' ';
		m_FriendlyName += m_DisplayVersion;
	}
	if (!m_BuildNumber.empty())
	{
		m_FriendlyName += L" (build ";
		m_FriendlyName += m_BuildNumber;
		m_FriendlyName += ubrStr;

		if (!servicePack.empty())
		{
			m_FriendlyName += L": ";
			m_FriendlyName += servicePack;
		}

		m_FriendlyName += L')';
	}
	m_FriendlyName += m_Is64BitWindows ? L" 64-bit" : L" 32-bit";

	// Retrieve user language LCID
	LANGID id = GetUserDefaultUILanguage();
	LCID lcid = MAKELCID(id, SORT_DEFAULT);
	WCHAR buffer[LOCALE_NAME_MAX_LENGTH];
	if (GetLocaleInfo(lcid, LOCALE_SENGLISHLANGUAGENAME, buffer, _countof(buffer)) == 0)
	{
		_snwprintf_s(buffer, _TRUNCATE, L"%s", L"<error>");
	}
	m_UserLanguage = buffer;
}
