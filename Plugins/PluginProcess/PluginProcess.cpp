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
#include <algorithm>
#include <vector>
#include "../../Library/RawString.h"
#include "../../Library/Export.h"	// Rainmeter's exported functions
#include "../../Library/DisableThreadLibraryCalls.h"	// contains DllMain entry point

struct MeasureData
{
	CRawString processName;
	bool isRunning;

	MeasureData() : isRunning(false) {}
};

static UINT g_UpdateCount = 0;
static std::vector<MeasureData*> g_Measures;

void CheckProcesses();

PLUGIN_EXPORT void Initialize(void** data)
{
	MeasureData* measure = new MeasureData;
	g_Measures.push_back(measure);

	*data = measure;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;

	LPCWSTR value = RmReadString(rm, L"ProcessName", L"");
	measure->processName = value;
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;

	// Updates the measure only once per combined updates of all measures
	++g_UpdateCount;
	if (g_UpdateCount >= g_Measures.size())
	{
		CheckProcesses();
		g_UpdateCount = 0;
	}

	return (double)measure->isRunning;
}

PLUGIN_EXPORT void Finalize(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	std::vector<MeasureData*>::iterator iter = std::find(g_Measures.begin(), g_Measures.end(), measure);
	g_Measures.erase(iter);

	delete measure;
}

void CheckProcesses()
{
	// Set everything to false
	std::vector<MeasureData*>::iterator iter = g_Measures.begin();
	for ( ; iter != g_Measures.end(); ++iter)
	{
		(*iter)->isRunning = false;
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
				iter = g_Measures.begin();
				for ( ; iter != g_Measures.end(); ++iter)
				{
					if (_wcsicmp(buffer, (*iter)->processName.c_str()) == 0)
					{
						(*iter)->isRunning = true;
					}
				}
			}

			CloseHandle(hProcess);
		}
	}

	delete [] pids;
}
