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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <windows.h>
#include <TlHelp32.h>
#include <algorithm>
#include <vector>
#include "../../Common/RawString.h"
#include "../../Library/Export.h"	// Rainmeter's exported functions

struct MeasureData
{
	RawString processName;
	bool isRunning;

	MeasureData() : isRunning(false) {}
};

static UINT g_UpdateCount = 0;
static std::vector<MeasureData*> g_Measures;

void CheckProcesses();

PLUGIN_EXPORT void Initialize(void** data, void* rm)
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

	return measure->isRunning ? 1.0 : -1.0;
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

	HANDLE thSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (thSnapshot != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 processEntry = {sizeof(processEntry)};
		if (Process32First(thSnapshot, &processEntry))
		{
			do
			{
				for (MeasureData* data : g_Measures)
				{
					if (_wcsicmp(processEntry.szExeFile, data->processName.c_str()) == 0)
					{
						data->isRunning = true;
					}
				}
			}
			while (Process32Next(thSnapshot, &processEntry));
		}

		CloseHandle(thSnapshot);
	}
}
