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
#include "Util.h"

namespace Util {

//
// System functions.
//

bool IsSystem64Bit()
{
#ifdef _WIN64
	return true;
#else
	BOOL system64 = FALSE;

	typedef BOOL (WINAPI * FPIsWow64Process)(HANDLE hProcess, BOOL* Wow64Process);
	auto isWow64Process = (FPIsWow64Process)GetProcAddress(
		GetModuleHandle(L"kernel32"), "IsWow64Process");
	if (isWow64Process)
	{
		isWow64Process(GetCurrentProcess(), &system64);
	}

	return system64;
#endif
}

//
// Process functions.
//

bool IsProcessUserAdmin()
{
	BOOL runningAsAdmin = FALSE;

	// Allocate and initialize a SID of the administrators group.
	PSID adminGroupSid = nullptr;
	SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
	if (AllocateAndInitializeSid(
		&NtAuthority, 
		2, 
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&adminGroupSid))
	{
		// Check if the primary access token of the process has the admin group SID.
		if (!CheckTokenMembership(nullptr, adminGroupSid, &runningAsAdmin))
		{
			runningAsAdmin = TRUE;
		}

		FreeSid(adminGroupSid);
		adminGroupSid = nullptr;
	}

	return runningAsAdmin;
}

bool CanProcessUserElevate()
{
	OSVERSIONINFO osvi = {sizeof(osvi)};
	GetVersionEx(&osvi);
	if (osvi.dwMajorVersion >= 6)
	{
		// Check if UAC is enabled with Vista and above.
		BOOL uacEnabled = FALSE;

		// First try with undocumented CheckElevationEnabled function.
		// See: http://blog.airesoft.co.uk/2011/03/uaceen-nothing-yet/
		typedef DWORD (WINAPI * FPCheckElevationEnabled)(BOOL* pResult);
		auto checkElevationEnabled = (FPCheckElevationEnabled)GetProcAddress(
			GetModuleHandle(L"kernel32"), "CheckElevationEnabled");
		if (checkElevationEnabled && checkElevationEnabled(&uacEnabled) == ERROR_SUCCESS)
		{
			return uacEnabled;
		}
		else
		{
			// Try checking registry.
			const WCHAR* subKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System";
			DWORD data;
			if (GetRegistryDword(HKEY_LOCAL_MACHINE, subKey, L"EnableLUA", &data) == ERROR_SUCCESS &&
				data != 0)
			{
				return true;
			}
		}
	}

	return false;
}

//
// File and directory functions.
//

bool CopyDirectory(const WCHAR* fromPath, const WCHAR* toPath)
{
	int fromPathLength = wcslen(fromPath);
	int toPathLength = wcslen(toPath);

	// SHFileOperation expects double null terminated strings.
	WCHAR* from = (WCHAR*)malloc((fromPathLength + 2) * sizeof(WCHAR));
	wcscpy(from, fromPath);
	from[fromPathLength + 2] = L'\0';

	WCHAR* to = (WCHAR*)malloc((toPathLength + 2) * sizeof(WCHAR));
	wcscpy(to, toPath);
	to[toPathLength + 2] = L'\0';

	SHFILEOPSTRUCT fo =
	{
		nullptr,
		FO_COPY,
		from,
		to,
		FOF_SILENT | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION
	};

	BOOL copied = (SHFileOperation(&fo) == 0);

	free(from);
	free(to);

	return copied;
}

bool CreateShortcutFile(const WCHAR* filePath, const WCHAR* targetPath)
{
	IShellLink* psl;
	HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&psl);
	if (SUCCEEDED(hr))
	{
		IPersistFile* ppf;
		hr = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
		if (SUCCEEDED(hr))
		{
			psl->SetPath(filePath);
			hr = ppf->Save(targetPath, TRUE);

			ppf->Release();
		}

		psl->Release();
	}

	return SUCCEEDED(hr);
}

bool IsDirectoryWritable()
{
	return TRUE;
}

//
// Registry functions.
//

bool GetRegistryDword(HKEY rootKey, const WCHAR* subKey, const WCHAR* value, DWORD* data)
{
	DWORD type;
	DWORD dataSize = sizeof(DWORD);
	return (SHGetValue(rootKey, subKey, value, &type, data, &dataSize) == ERROR_SUCCESS &&
		type == REG_DWORD);
}

bool GetRegistryString(HKEY rootKey, const WCHAR* subKey, const WCHAR* value, WCHAR* data, DWORD dataSize)
{
	DWORD type;
	return (SHGetValue(rootKey, subKey, value, &type, data, &dataSize) == ERROR_SUCCESS &&
		type == REG_SZ);
}

bool SetRegistryData(DWORD type, HKEY rootKey, const WCHAR* subKey, const WCHAR* value, BYTE* data, DWORD dataSize)
{
	BOOL result = FALSE;
	HKEY regKey;
	if (RegCreateKeyEx(rootKey, subKey, 0, 0, 0, KEY_SET_VALUE, nullptr, &regKey, nullptr) == ERROR_SUCCESS)
	{
		if (RegSetValueEx(regKey, value, 0, type, data, dataSize) == ERROR_SUCCESS)
		{
			result = TRUE;
		}

		RegCloseKey(regKey);
	}

	return result;
}

bool SetRegistryDword(HKEY rootKey, const WCHAR* subKey, const WCHAR* value, DWORD data)
{
	return SetRegistryData(REG_DWORD, rootKey, subKey, value, (BYTE*)&data, sizeof(DWORD));
}

bool SetRegistryString(HKEY rootKey, const WCHAR* subKey, const WCHAR* value, const WCHAR* data)
{
	DWORD dataSize = (wcslen(data) + 1) * sizeof(WCHAR);
	return SetRegistryData(REG_SZ, rootKey, subKey, value, (BYTE*)data, dataSize);
}

//
// Misc. functions.
//

bool DownloadFile(const WCHAR* url, const WCHAR* file)
{
	bool result = false;
	HINTERNET hNet = InternetOpen(L"Mozilla/5.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	if (hNet)
	{
		HANDLE hFile = CreateFile(file, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile)
		{
			HINTERNET hUrl = InternetOpenUrl(hNet, url, nullptr, 0, INTERNET_FLAG_RESYNCHRONIZE, 0);
			if (hUrl)
			{
				const DWORD bufferSize = 8192;
				BYTE* buffer = (BYTE*)malloc(bufferSize);
				if (buffer)
				{
					DWORD readSize;
					while (InternetReadFile(hUrl, buffer, bufferSize, &readSize))
					{
						if (readSize == 0)
						{
							// All data read.
							result = true;
							break;
						}

						DWORD writeSize;
						if (!WriteFile(hFile, buffer, readSize, &writeSize, nullptr) ||
							readSize != writeSize)
						{
							break;
						}
					}

					free(buffer);
				}

				 InternetCloseHandle(hUrl);
			}

			CloseHandle(hFile);

			if (!result)
			{
				DeleteFile(file);
			}
		}

		InternetCloseHandle(hNet);
	}

	return result;
}

}  // namespace Util