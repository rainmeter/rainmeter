/*
  Copyright (C) 2011 Birunthan Mohanathas

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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <windows.h>
#include <Psapi.h>
#include <string>
#include <map>
#include "../../Library/Export.h"	// Rainmeter's exported functions

#include "../../Library/DisableThreadLibraryCalls.h"	// contains DllMain entry point

/* The exported functions */
extern "C"
{
__declspec(dllexport) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
__declspec(dllexport) void Finalize(HMODULE instance, UINT id);
__declspec(dllexport) UINT Update(UINT id);
__declspec(dllexport) UINT GetPluginVersion();
__declspec(dllexport) LPCTSTR GetPluginAuthor();
}

struct MeasureData
{
	std::wstring processName;
	bool isRunning;

	MeasureData() : isRunning(false) {}
};

static std::map<UINT, MeasureData> g_Values;
static UINT g_UpdateCount = 0;

void CheckProcesses();

/*
  This function is called when the measure is initialized.
  The function must return the maximum value that can be measured.
  The return value can also be 0, which means that Rainmeter will
  track the maximum value automatically. The parameters for this
  function are:

  instance  The instance of this DLL
  iniFile   The name of the ini-file (usually Rainmeter.ini)
  section   The name of the section in the ini-file for this measure
  id        The identifier for the measure. This is used to identify the measures that use the same plugin.
*/
UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id)
{
	MeasureData data;

	const WCHAR* value = ReadConfigString(section, L"ProcessName", L"");
	if (*value)
	{
		data.processName = value;
		g_Values[id] = std::move(data);
	}

	return 1;
}

/*
  This function is called when new value should be measured.
  The function returns the new value.
*/
UINT Update(UINT id)
{
	UINT result = 0;

	std::map<UINT, MeasureData>::const_iterator iter = g_Values.find(id);
	if (iter != g_Values.end())
	{
		// Updates the measure only once per combined updates of all measures
		++g_UpdateCount;
		if (g_UpdateCount >= g_Values.size())
		{
			CheckProcesses();
			g_UpdateCount = 0;
		}

		result = (UINT)(*iter).second.isRunning ? 1 : -1;
	}

	return result;
}

/*
  If the measure needs to free resources before quitting.
  The plugin can export Finalize function, which is called
  when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	std::map<UINT, MeasureData>::iterator iter = g_Values.find(id);
	if (iter != g_Values.end())
	{
		g_Values.erase(iter);
	}
}

UINT GetPluginVersion()
{
	return 1000;
}

LPCTSTR GetPluginAuthor()
{
	return L"Birunthan Mohanathas (poiru.net)";
}

void CheckProcesses()
{
	// Set everything to false
	std::map<UINT, MeasureData>::iterator iter = g_Values.begin();
	for ( ; iter != g_Values.end(); ++iter)
	{
		(*iter).second.isRunning = false;
	}

	int bufSize = 256;
	DWORD* pids = new DWORD[bufSize];
	DWORD bytesReturned = 0;
	while (!EnumProcesses(pids, bufSize * sizeof(DWORD), &bytesReturned) &&
			bytesReturned == bufSize * sizeof(DWORD))
	{
		delete [] pids;
		bufSize *= 2;
		pids = new DWORD[bufSize];
	}

	for (UINT i = 0, isize = bytesReturned / sizeof(DWORD); i < isize; ++i)
	{
		const DWORD flags = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ;
		HANDLE hProcess = OpenProcess(flags, FALSE, pids[i]);
		if (hProcess)
		{
			WCHAR buffer[MAX_PATH];
			if (GetModuleBaseName(hProcess, NULL, buffer, _countof(buffer)))
			{
				iter = g_Values.begin();
				for ( ; iter != g_Values.end(); ++iter)
				{
					if (_wcsicmp(buffer, (*iter).second.processName.c_str()) == 0)
					{
						(*iter).second.isRunning = true;
					}
				}
			}

			CloseHandle(hProcess);
		}
	}

	delete [] pids;
}
