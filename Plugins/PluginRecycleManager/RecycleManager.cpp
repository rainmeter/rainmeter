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
#include <process.h>
#include <cstdio>
#include <cstdlib>
#include "../../Library/Export.h"	// Rainmeter's exported functions

struct MeasureData
{
	bool count;

	MeasureData() : count(false) {}
};

DWORD WINAPI QueryRecycleBinThreadProc(void* pParam);
bool HasRecycleBinChanged();

double g_BinCount = 0;
double g_BinSize = 0;

int g_UpdateCount = 0;
int g_InstanceCount = 0;
bool g_Thread = false;
bool g_FreeInstanceInThread = false;
CRITICAL_SECTION g_CriticalSection;

bool g_IsPlatformXP = false;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		InitializeCriticalSection(&g_CriticalSection);

		// Disable DLL_THREAD_ATTACH and DLL_THREAD_DETACH notification calls
		DisableThreadLibraryCalls(hinstDLL);
		break;

	case DLL_PROCESS_DETACH:
		DeleteCriticalSection(&g_CriticalSection);
		break;
	}

	return TRUE;
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData;
	*data = measure;

	if (g_InstanceCount == 0)
	{
		OSVERSIONINFOEX osvi = {sizeof(OSVERSIONINFOEX)};
		GetVersionEx((OSVERSIONINFO*)&osvi);

		// Not checking for osvi.dwMinorVersion >= 1 because pre-XP is not supported.
		g_IsPlatformXP = (osvi.dwMajorVersion == 5);
	}

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

	if (TryEnterCriticalSection(&g_CriticalSection))
	{
		if (!g_Thread)
		{
			++g_UpdateCount;
			if (g_UpdateCount > g_InstanceCount)
			{
				if (HasRecycleBinChanged())
				{
					// Delay next check.
					g_UpdateCount = g_InstanceCount * -2;

					DWORD id;
					HANDLE thread = CreateThread(nullptr, 0, QueryRecycleBinThreadProc, nullptr, 0, &id);
					if (thread)
					{
						CloseHandle(thread);
						g_Thread = true;
					}
				}
				else
				{
					g_UpdateCount = 0;
				}
			}
		}

		LeaveCriticalSection(&g_CriticalSection);
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
		EnterCriticalSection(&g_CriticalSection);
		if (g_Thread && !g_FreeInstanceInThread)
		{
			// Increment ref count of this module so that it will not be unloaded prior to
			// thread completion.
			DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS;
			HMODULE module;
			GetModuleHandleEx(flags, (LPCWSTR)DllMain, &module);
			g_FreeInstanceInThread = true;
		}
		LeaveCriticalSection(&g_CriticalSection);
	}
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	MeasureData* measure = (MeasureData*)data;

	if (_wcsicmp(args, L"EmptyBin") == 0)
	{
		SHEmptyRecycleBin(nullptr, nullptr, 0);
	}
	else if (_wcsicmp(args, L"EmptyBinSilent") == 0)
	{
		SHEmptyRecycleBin(nullptr, nullptr, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
	}
	else if (_wcsicmp(args, L"OpenBin") == 0)
	{
		ShellExecute(nullptr, L"open", L"explorer.exe", L"/N,::{645FF040-5081-101B-9F08-00AA002F954E}", nullptr, SW_SHOW);
	}
}

DWORD WINAPI QueryRecycleBinThreadProc(void* pParam)
{
	// NOTE: Do not use CRT functions (since thread was created with CreateThread())!

	SHQUERYRBINFO rbi = {0};
	rbi.cbSize = sizeof(SHQUERYRBINFO);
	SHQueryRecycleBin(nullptr, &rbi);
	g_BinCount = (double)rbi.i64NumItems;
	g_BinSize = (double)rbi.i64Size;

	EnterCriticalSection(&g_CriticalSection);
	HMODULE module = nullptr;
	if (g_FreeInstanceInThread)
	{
		DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
		GetModuleHandleEx(flags, (LPCWSTR)DllMain, &module);
		g_FreeInstanceInThread = false;
	}

	g_Thread = false;
	LeaveCriticalSection(&g_CriticalSection);

	if (module)
	{
		// Decrement the ref count and possibly unload the module if this is
		// the last instance.
		FreeLibraryAndExitThread(module, 0);
	}

	return 0;
}

bool HasRecycleBinChanged()
{
	static DWORD s_LastVolumeCount = 0;
	static ULONGLONG s_LastWriteTime = 0;

	bool changed = false;

	// Check if items have been added to recycle bin since last check.
	HKEY volumeKey;
	const WCHAR* subKey = g_IsPlatformXP ?
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\BitBucket" :
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\BitBucket\\Volume";
	LSTATUS ls = RegOpenKeyEx(HKEY_CURRENT_USER, subKey, 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &volumeKey);
	if (ls == ERROR_SUCCESS)
	{
		DWORD volumeCount = 0;
		RegQueryInfoKey(volumeKey, nullptr, nullptr, nullptr, &volumeCount, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
		if (volumeCount != s_LastVolumeCount)
		{
			s_LastVolumeCount = volumeCount;
			changed = true;
		}

		WCHAR buffer[64];
		DWORD bufferSize = _countof(buffer);
		DWORD index = 0;

		while ((ls = RegEnumKeyEx(volumeKey, index, buffer, &bufferSize, nullptr, nullptr, nullptr, nullptr)) == ERROR_SUCCESS)
		{
			HKEY volumeSubKey;
			ls = RegOpenKeyEx(volumeKey, buffer, 0, KEY_QUERY_VALUE, &volumeSubKey);
			if (ls == ERROR_SUCCESS)
			{
				ULONGLONG lastWriteTime;
				ls = RegQueryInfoKey(volumeSubKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, (FILETIME*)&lastWriteTime);
				if (ls == ERROR_SUCCESS)
				{
					if (lastWriteTime > s_LastWriteTime)
					{
						s_LastWriteTime = lastWriteTime;
						changed = true;
					}
				}

				RegCloseKey(volumeSubKey);
			}

			bufferSize = _countof(buffer);
			++index;
		}

		RegCloseKey(volumeKey);
	}

	if (!changed)
	{
		// Check if recycle bin has been emptied.
		HKEY iconKey;
		const WCHAR* subKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\DefaultIcon";
		ls = RegOpenKeyEx(HKEY_CURRENT_USER, subKey, 0, KEY_QUERY_VALUE, &iconKey);
		if (ls == ERROR_SUCCESS)
		{
			ULONGLONG lastWriteTime;
			ls = RegQueryInfoKey(iconKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, (FILETIME*)&lastWriteTime);
			if (ls == ERROR_SUCCESS)
			{
				if (lastWriteTime > s_LastWriteTime)
				{
					s_LastWriteTime = lastWriteTime;
					changed = true;
				}
			}

			RegCloseKey(iconKey);
		}
	}

	return changed;
}
