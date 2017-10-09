/* Copyright (C) 2004 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

/*
  Requires Win2k or later.  Sorry to anyone still using 9x or ME.
  All my results seem to agree with Task Manager (it gets tedious adding up
  the different process values).  Already proved useful in identifying
  resource leaks in itself (all fixed).  There's irony for you!
*/

#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include "../../Common/RawString.h"
#include "../../Library/Export.h"	// Rainmeter's exported functions

// system resources that can be counted
enum MEASURETYPE
{
	GDI_COUNT,
	USER_COUNT,
	HANDLE_COUNT,
	WINDOW_COUNT
};

struct MeasureData
{
	MEASURETYPE type;
	RawString process;

	MeasureData() : type(GDI_COUNT) {}
};

// used to track the number of existing windows
UINT g_WindowCount = 0;

// count the windows
BOOL CALLBACK EnumWindowProc(HWND hWnd, LPARAM lParam)
{
	++g_WindowCount;
	return TRUE;
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData;
	*data = measure;
}


PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;

	LPCWSTR type = RmReadString(rm, L"ResCountType", L"GDI");
	if (_wcsicmp(L"GDI", type) == 0)
	{
		measure->type = GDI_COUNT;
	}
	else if (_wcsicmp(L"USER", type) == 0)
	{
		measure->type = USER_COUNT;
	}
	else if (_wcsicmp(L"HANDLE", type) == 0)
	{
		measure->type = HANDLE_COUNT;
	}
	else if (_wcsicmp(L"WINDOW", type) == 0)
	{
		measure->type = WINDOW_COUNT;
	}
	else
	{
		RmLogF(rm, LOG_ERROR, L"ResMon.dll: GDICountType=%s is not valid", type);
	}

	measure->process = RmReadString(rm, L"ProcessName", L"");
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;

	// count the existing window objects
	if (measure->type == WINDOW_COUNT)
	{
		g_WindowCount = 0;
		EnumChildWindows(nullptr, EnumWindowProc, 0);
		return g_WindowCount;
	}

	const WCHAR* processName = measure->process.c_str();
	bool name = !measure->process.empty();

	DWORD aProcesses[1024];
	DWORD bytesNeeded;
	WCHAR buffer[1024];
	HMODULE hMod[1024];
	DWORD cbNeeded;

	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &bytesNeeded))
	{
		return 0.0;
	}

	// step through the running processes
	DWORD flags = PROCESS_QUERY_INFORMATION;

	if (name)
	{
		flags |= PROCESS_VM_READ;
	}

	UINT resourceCount = 0;
	for (UINT i = 0, isize = bytesNeeded / sizeof(DWORD); i < isize; ++i)
	{
		HANDLE hProcess = OpenProcess(flags, true, aProcesses[i]);
		if (hProcess != nullptr)
		{
			if (name)
			{
				if (EnumProcessModules(hProcess, hMod, sizeof(hMod), &cbNeeded))
				{
					if (GetModuleBaseName(hProcess, hMod[0], buffer, sizeof(buffer)))
					{
						if (_wcsicmp(buffer, processName) != 0)
						{
							CloseHandle(hProcess);
							continue;
						}
					}
					else
					{
						CloseHandle(hProcess);
						continue;
					}
				}
				else
				{
					CloseHandle(hProcess);
					continue;
				}
			}

			if (measure->type == GDI_COUNT)
			{
				resourceCount += GetGuiResources(hProcess, GR_GDIOBJECTS);
			}
			else if (measure->type == USER_COUNT)
			{
				resourceCount += GetGuiResources(hProcess, GR_USEROBJECTS);
			}
			else if (measure->type == HANDLE_COUNT)
			{
				DWORD tempHandleCount = 0;
				GetProcessHandleCount(hProcess, &tempHandleCount);
				resourceCount += tempHandleCount;
			}
		}
		CloseHandle(hProcess);
	}

	return resourceCount;
}

PLUGIN_EXPORT void Finalize(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	delete measure;
}
