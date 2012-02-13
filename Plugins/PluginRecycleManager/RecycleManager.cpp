/*
  Copyright (C) 2005 Kimmo Pekkola, 2009 Greg Schoppe

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
#include <ShlObj.h>
#include <process.h>
#include <vector>
#include "../../Library/Export.h"	// Rainmeter's exported functions
#include "../../Library/DisableThreadLibraryCalls.h"	// contains DllMain entry point

struct MeasureData
{
	bool count;

	MeasureData() : count(false) {}
};

struct BinData
{
	FILETIME lastAccess;
	HANDLE directory;
	WCHAR drive;
};

unsigned int __stdcall QueryRecycleBinThreadProc(void* pParam);
HRESULT GetFolderCLSID(LPCWSTR pszPath, CLSID* pathCLSID);
HANDLE GetRecycleBinHandle(WCHAR drive);

std::vector<BinData> g_BinData;
double g_BinCount = 0;
double g_BinSize = 0;

UINT g_InstanceCount = 0;
HANDLE g_Thread = NULL;

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData;
	*data = measure;

	++g_InstanceCount;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;

	LPCWSTR value = RmReadString(rm, L"RecycleType", L"COUNT");
	if (_wcsicmp(L"COUNT", value) == 0)
	{
		measure->count = true;
	}
	else if (_wcsicmp(L"SIZE", value) == 0)
	{
		measure->count = false;
	}
	else
	{
		WCHAR buffer[256];
		_snwprintf_s(buffer, _TRUNCATE, L"RecycleManager.dll: RecycleType=%s is not valid in [%s]", value, RmGetMeasureName(rm));
		RmLog(LOG_ERROR, buffer);
	}
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;

	if (!g_Thread)
	{
		WCHAR buffer[128];
		DWORD len = GetLogicalDriveStrings(128, buffer);

		// Convert "A:\\\0B:\\\0" -> "AB\0"
		int index = 0;
		for (int i = 0; i < len; i += 4)
		{
			buffer[index] = buffer[i];
			++index;
		}
		buffer[index] = L'\0';

		const WCHAR DRIVE_HANDLED = 1;
		bool changed = false;
		auto iter = g_BinData.begin();
		while (iter != g_BinData.end())
		{
			BinData& data = (*iter);

			WCHAR* pos = wcschr(buffer, data.drive);
			if (pos != NULL)
			{
				*pos = DRIVE_HANDLED;
				++iter;

				if (data.lastAccess.dwHighDateTime != 0xFFFFFFFF &&
					data.lastAccess.dwLowDateTime != 0xFFFFFFFF)
				{
					FILETIME lastAccess;
					GetFileTime(data.directory, NULL, &lastAccess, NULL);
					if (data.lastAccess.dwHighDateTime != lastAccess.dwHighDateTime ||
						data.lastAccess.dwLowDateTime != lastAccess.dwLowDateTime)
					{
						data.lastAccess.dwHighDateTime = lastAccess.dwHighDateTime;
						data.lastAccess.dwLowDateTime = lastAccess.dwLowDateTime;
						changed = true;
					}
				}
			}
			else
			{
				// Drive removed
				if (data.directory)
				{
					CloseHandle(data.directory);
				}

				iter = g_BinData.erase(iter);
			}
		}

		for (int i = 0; i < index; ++i)
		{
			if (buffer[i] != DRIVE_HANDLED)
			{
				// New drive
				BinData data = {0};
				data.directory = GetRecycleBinHandle(buffer[i]);
				data.drive = buffer[i];
				data.lastAccess.dwHighDateTime =
					data.lastAccess.dwLowDateTime = data.directory ? 0 : 0xFFFFFFFF;
			
				g_BinData.push_back(data);
			}
		}

		if (changed)
		{
			g_Thread = (HANDLE)_beginthreadex(NULL, 64 * 1024, QueryRecycleBinThreadProc, NULL, 0, NULL);
		}

	}

	return measure->count ? g_BinCount : g_BinSize;
}

PLUGIN_EXPORT void Finalize(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	delete measure;

	--g_InstanceCount;
	if (g_InstanceCount == 0)
	{
		for (auto iter = g_BinData.cbegin(); iter != g_BinData.cend(); ++iter)
		{
			if ((*iter).directory)
			{
				CloseHandle((*iter).directory);
			}
		}

		WaitForSingleObject(g_Thread, INFINITE);
	}
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	MeasureData* measure = (MeasureData*)data;

	if (_wcsicmp(args, L"EmptyBin") == 0)
	{
		SHEmptyRecycleBin(NULL, NULL, 0);
	}
	else if (_wcsicmp(args, L"EmptyBinSilent") == 0)
	{
		SHEmptyRecycleBin(NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
	}
	else if (_wcsicmp(args, L"OpenBin") == 0)
	{
		ShellExecute(NULL, L"open", L"explorer.exe", L"/N,::{645FF040-5081-101B-9F08-00AA002F954E}", NULL, SW_SHOW);
	}
}

unsigned int __stdcall QueryRecycleBinThreadProc(void* pParam)
{
	SHQUERYRBINFO rbi = {0};
	rbi.cbSize = sizeof(SHQUERYRBINFO);
	SHQueryRecycleBin(NULL, &rbi);
	g_BinCount = (double)rbi.i64NumItems;
	g_BinSize = (double)rbi.i64Size;

	g_Thread = NULL;

	_endthreadex(0);
	return 0;
}

HRESULT GetFolderCLSID(LPCWSTR path, CLSID* clsid)
{
	LPITEMIDLIST pidl;
	HRESULT hr = SHParseDisplayName(path, NULL, &pidl, 0, NULL);
	if (SUCCEEDED(hr))
	{
		IShellFolder* sf;
		LPCITEMIDLIST pidlLast;
		hr = SHBindToParent(pidl, IID_IShellFolder, (void**)&sf, &pidlLast);
		if (SUCCEEDED(hr))
		{
			SHDESCRIPTIONID did;
			hr = SHGetDataFromIDList(sf, pidlLast, SHGDFIL_DESCRIPTIONID, &did, sizeof(did));
			*clsid = did.clsid;

			sf->Release();
		}

		CoTaskMemFree(pidl);
	}
	return hr;
}

HANDLE GetRecycleBinHandle(WCHAR drive)
{
	WCHAR search[] = L"\0:\\*";
	search[0] = drive;

	WIN32_FIND_DATA fd;
	HANDLE hSearch = FindFirstFile(search, &fd);
	if (hSearch != INVALID_HANDLE_VALUE)
	{
		bool found = false;
		std::wstring path;

		do
		{
			if ((fd.dwFileAttributes &
				(FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_DIRECTORY)) == 0)
			{
				continue;
			}

			WCHAR buffer[MAX_PATH];
			int len = _snwprintf_s(buffer, _TRUNCATE, L"%c:\\%s\\*", drive, fd.cFileName);

			// Find the SID-specific child directory
			HANDLE hChildSearch = FindFirstFile(buffer, &fd);
			if (hChildSearch != INVALID_HANDLE_VALUE)
			{
				do
				{
					if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || (fd.cFileName[0] == L'.'))
					{
						continue;
					}

					path.assign(buffer, len - 1);
					path += fd.cFileName;

					CLSID id;
					HRESULT hr = GetFolderCLSID(path.c_str(), &id);
					if (SUCCEEDED(hr) && IsEqualGUID(CLSID_RecycleBin, id))
					{
						found = true;
						break;
					}
				}
				while (FindNextFile(hChildSearch, &fd));

				FindClose(hChildSearch);

				if (found)
				{
					// Break out of main loop
					break;
				}
			}
		}
		while (FindNextFile(hSearch, &fd));

		FindClose(hSearch);

		if (found)
		{
			HANDLE hDir = CreateFile(
				path.c_str(),
				GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				FILE_FLAG_BACKUP_SEMANTICS,
				NULL);

			return hDir;
		}
	}

	return NULL;
}
