/*
  Copyright (C) 2004 Kimmo Pekkola

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
#include <vector>
#include "../PluginPerfMon/Titledb.h"
#include "../PluginPerfMon/PerfSnap.h"
#include "../PluginPerfMon/PerfObj.h"
#include "../PluginPerfMon/PerfCntr.h"
#include "../PluginPerfMon/ObjList.h"
#include "../PluginPerfMon/ObjInst.h"
#include "../API/RainmeterAPI.h"
#include "../../Common/RawString.h"

struct MeasureData
{
	std::vector<RawString> includes;
	std::vector<RawString> excludes;
	RawString includesCache;
	RawString excludesCache;
	int topProcess;
	RawString topProcessName;
	LONGLONG topProcessValue;

	MeasureData() :
		topProcess(-1),
		topProcessValue()
	{
	}
};

struct ProcessValues
{
	RawString name;
	LONGLONG oldValue;
	LONGLONG newValue;
	bool found;
};

static CPerfTitleDatabase g_CounterTitles(PERF_TITLE_COUNTER);
std::vector<ProcessValues> g_Processes;

void UpdateProcesses();

void SplitName(WCHAR* names, std::vector<RawString>& splittedNames)
{
	WCHAR* token = wcstok(names, L";");
	while (token != nullptr)
	{
		splittedNames.push_back(token);
		token = wcstok(nullptr, L";");
	}
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData;
	*data = measure;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;
	bool changed = false;

	LPCWSTR value = RmReadString(rm, L"CPUInclude", L"");
	if (_wcsicmp(value, measure->includesCache.c_str()) != 0)
	{
		measure->includesCache = value;
		measure->includes.clear();

		WCHAR* buffer = _wcsdup(value);
		SplitName(buffer, measure->includes);
		delete buffer;
		changed = true;
	}

	value = RmReadString(rm, L"CPUExclude", L"");
	if (_wcsicmp(value, measure->excludesCache.c_str()) != 0)
	{
		measure->excludesCache = value;
		measure->excludes.clear();

		WCHAR* buffer = _wcsdup(value);
		SplitName(buffer, measure->excludes);
		delete buffer;
		changed = true;
	}

	int topProcess = RmReadInt(rm, L"TopProcess", 0);
	if (topProcess != measure->topProcess)
	{
		measure->topProcess = topProcess;
		changed = true;
	}

	if (changed)
	{
		*maxValue = 10000000;	// The values are 100 * 100000
	}
}

bool CheckProcess(MeasureData* measure, const WCHAR* name)
{
	if (measure->includes.empty())
	{
		for (size_t i = 0; i < measure->excludes.size(); ++i)
		{
			if (_wcsicmp(measure->excludes[i].c_str(), name) == 0)
			{
				return false;		// Exclude
			}
		}
		return true;	// Include
	}
	else
	{
		for (size_t i = 0; i < measure->includes.size(); ++i)
		{
			if (_wcsicmp(measure->includes[i].c_str(), name) == 0)
			{
				return true;	// Include
			}
		}
	}
	return false;
}

ULONGLONG _GetTickCount64()
{
	typedef ULONGLONG (WINAPI * FPGETTICKCOUNT64)();
	static FPGETTICKCOUNT64 c_GetTickCount64 = (FPGETTICKCOUNT64)GetProcAddress(GetModuleHandle(L"kernel32"), "GetTickCount64");

	if (c_GetTickCount64)
	{
		return c_GetTickCount64();
	}
	else
	{
		static ULONGLONG lastTicks = 0;
		ULONGLONG ticks = GetTickCount();
		while (ticks < lastTicks) ticks += 0x100000000;
		lastTicks = ticks;
		return ticks;
	}
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	static ULONGLONG oldTime = 0;

	// Only update twice per second
	ULONGLONG time = _GetTickCount64();
	if (oldTime == 0 || time - oldTime > 500)
	{
		UpdateProcesses();
		oldTime = time;
	}

	LONGLONG newValue = 0;

	for (size_t i = 0; i < g_Processes.size(); ++i)
	{
		// Check process include/exclude
		if (CheckProcess(measure, g_Processes[i].name.c_str()))
		{
			if (g_Processes[i].oldValue != 0)
			{
				LONGLONG value = g_Processes[i].newValue - g_Processes[i].oldValue;

				if (measure->topProcess == 0)
				{
					// Add all values together
					newValue += value;
				}
				else
				{
					// Find the top process
					if (newValue < value)
					{
						newValue = value;
						measure->topProcessName = g_Processes[i].name;
						measure->topProcessValue = newValue;
					}
				}
			}
		}
	}

	return (double)newValue;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	MeasureData* measure = (MeasureData*)data;

	if (measure->topProcess == 2)
	{
		return measure->topProcessName.c_str();
	}

	return nullptr;
}

PLUGIN_EXPORT void Finalize(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	delete measure;

	CPerfSnapshot::CleanUp();
}

/*
  This updates the process status
*/
void UpdateProcesses()
{
	BYTE data[256];
	WCHAR name[256];

	std::vector<ProcessValues> newProcesses;
	newProcesses.reserve(g_Processes.size());

	CPerfSnapshot snapshot(&g_CounterTitles);
	CPerfObjectList objList(&snapshot, &g_CounterTitles);

	if (snapshot.TakeSnapshot(L"Process"))
	{
		CPerfObject* pPerfObj = objList.GetPerfObject(L"Process");

		if (pPerfObj)
		{
			for (CPerfObjectInstance* pObjInst = pPerfObj->GetFirstObjectInstance();
				pObjInst != nullptr;
				pObjInst = pPerfObj->GetNextObjectInstance())
			{
				if (pObjInst->GetObjectInstanceName(name, 256))
				{
					if (_wcsicmp(name, L"_Total") == 0)
					{
						delete pObjInst;
						continue;
					}

					CPerfCounter* pPerfCntr = pObjInst->GetCounterByName(L"% Processor Time");
					if (pPerfCntr != nullptr)
					{
						pPerfCntr->GetData(data, 256, nullptr);

						if (pPerfCntr->GetSize() == 8)
						{
							ProcessValues values;
							values.name = name;
							values.oldValue = 0;
							values.newValue = *(ULONGLONG*)data;
							values.found = false;

							// Check if we can find the old value
							for (size_t i = 0; i < g_Processes.size(); ++i)
							{
								if (!g_Processes[i].found && _wcsicmp(g_Processes[i].name.c_str(), name) == 0)
								{
									values.oldValue = g_Processes[i].newValue;
									g_Processes[i].found = true;
									break;
								}
							}

							newProcesses.push_back(std::move(values));
						}

						delete pPerfCntr;
					}
				}

				delete pObjInst;
			}

			delete pPerfObj;
		}
	}

	g_Processes = std::move(newProcesses);
}
