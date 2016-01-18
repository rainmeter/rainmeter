/* Copyright (C) 2016 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureRecycleManager.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "System.h"

namespace {

bool g_Thread = false;

double g_BinCount = 0;
double g_BinSize = 0;

int g_UpdateCount = 0;
int g_InstanceCount = 0;
CRITICAL_SECTION g_CriticalSection;

DWORD WINAPI QueryRecycleBinThreadProc(void* pParam)
{
	// NOTE: Do not use CRT functions (since thread was created with CreateThread())!

	SHQUERYRBINFO rbi = { 0 };
	rbi.cbSize = sizeof(SHQUERYRBINFO);
	SHQueryRecycleBin(nullptr, &rbi);
	g_BinCount = (double)rbi.i64NumItems;
	g_BinSize = (double)rbi.i64Size;

	g_Thread = false;

	return 0;
}

bool HasRecycleBinChanged()
{
	static DWORD s_LastVolumeCount = 0;
	static ULONGLONG s_LastWriteTime = 0;

	bool changed = false;

	// Check if items have been added to recycle bin since last check.
	HKEY volumeKey;
	const WCHAR* subKey =
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

}

enum class MeasureRecycleManager::Type
{
	None,
	Count,
	Size
};

MeasureRecycleManager::MeasureRecycleManager(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Type(Type::None)
{
	static bool s_Init = [] {
		System::InitializeCriticalSection(&g_CriticalSection);
		return true;
	} ();
}

MeasureRecycleManager::~MeasureRecycleManager()
{
}

void MeasureRecycleManager::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	const WCHAR* type = parser.ReadString(section, L"RecycleType", L"COUNT").c_str();
	if (_wcsicmp(L"COUNT", type) == 0)
	{
		m_Type = Type::Count;
	}
	else if (_wcsicmp(L"SIZE", type) == 0)
	{
		m_Type = Type::Size;
	}
	else
	{
		m_Type = Type::None;
		LogErrorF(this, L"Invalid RecycleType=%s");
	}
}

void MeasureRecycleManager::UpdateValue()
{
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

	switch (m_Type)
	{
		case Type::Size:
			m_Value = g_BinSize;
			break;

		case Type::Count:
			m_Value = g_BinCount;
			break;
	}
}

void MeasureRecycleManager::Command(const std::wstring& command)
{
	const WCHAR* args = command.c_str();
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
