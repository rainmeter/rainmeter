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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#pragma warning(disable: 4996)

#include <windows.h>
#include "AdvancedCPU.h"
#include <string>
#include <vector>
#include <map>
#include "..\..\Library\Export.h"	// Rainmeter's exported functions

//ULONGLONG GetPerfData(PCTSTR ObjectName, PCTSTR InstanceName, PCTSTR CounterName);
void UpdateProcesses();

struct CPUMeasure
{
	std::vector< std::wstring > includes;
	std::vector< std::wstring > excludes;
	int topProcess;
	std::wstring topProcessName;
	LONGLONG topProcessValue;
};

struct ProcessValues
{
	std::wstring name;
	LONGLONG oldValue;
	LONGLONG newValue;
	bool found;
};

static CPerfTitleDatabase g_CounterTitles( PERF_TITLE_COUNTER );
std::vector< ProcessValues > g_Processes;
static std::map<UINT, CPUMeasure*> g_CPUMeasures;

void SplitName(WCHAR* names, std::vector< std::wstring >& splittedNames)
{
	WCHAR* token;
	
	token = wcstok(names, L";");
	while(token != NULL)
	{
		splittedNames.push_back(token);
		token = wcstok(NULL, L";");
	}
}

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
	WCHAR buffer[4096];
	CPUMeasure* measure = new CPUMeasure;
	measure->topProcess = 0;
	measure->topProcessValue = 0;

	LPCTSTR data = ReadConfigString(section, L"CPUInclude", L"");
	if (data)
	{
		wcsncpy(buffer, data, 4096);
		buffer[4095] = 0;
		SplitName(buffer, measure->includes);
	}

	data = ReadConfigString(section, L"CPUExclude", L"");
	if (data)
	{
		wcsncpy(buffer, data, 4096);
		buffer[4095] = 0;
		SplitName(buffer, measure->excludes);
	}

	measure->topProcess = 0;
	data = ReadConfigString(section, L"TopProcess", L"0");
	if (data)
	{
		measure->topProcess = _wtoi(data);
	}

	g_CPUMeasures[id] = measure;

	return 10000000;	// The values are 100 * 100000
}

bool CheckProcess(CPUMeasure* measure, const std::wstring& name)
{
	if (measure->includes.empty()) 
	{
		for (size_t i = 0; i < measure->excludes.size(); i++)
		{
			if (_wcsicmp(measure->excludes[i].c_str(), name.c_str()) == 0)
			{
				return false;		// Exclude
			}
		}
		return true;	// Include
	}
	else
	{
		for (size_t i = 0; i < measure->includes.size(); i++)
		{
			if (_wcsicmp(measure->includes[i].c_str(), name.c_str()) == 0)
			{
				return true;	// Include
			}
		}
	}
	return false;
}

/*
  This function is called when new value should be measured.
  The function returns the new value.
*/
double Update2(UINT id)
{
	static DWORD oldTime = 0;
	
	// Only update twice per second
	DWORD time = GetTickCount();
	if (oldTime == 0 || time - oldTime > 500)
	{
		UpdateProcesses();
		oldTime = time;
	}

	LONGLONG newValue = 0;

	std::map<UINT, CPUMeasure*>::iterator i = g_CPUMeasures.find(id);
	if(i != g_CPUMeasures.end())
	{
		CPUMeasure* measure = (*i).second;

		if(measure)
		{
			for (size_t i = 0; i < g_Processes.size(); i++) 
			{
				// Check process include/exclude
				if (CheckProcess(measure, g_Processes[i].name)) 
				{
					if (g_Processes[i].oldValue != 0) 
					{
						if (measure->topProcess == 0) 
						{
							// Add all values together
							newValue += g_Processes[i].newValue - g_Processes[i].oldValue;
						}
						else 
						{
							// Find the top process
							if (newValue < g_Processes[i].newValue - g_Processes[i].oldValue)
							{
								newValue = g_Processes[i].newValue - g_Processes[i].oldValue;
								measure->topProcessName = g_Processes[i].name;
								measure->topProcessValue = newValue;
							}
						}
					}
				}
			}
		}

//				LONGLONG newValue = 0;
//				ULONGLONG longvalue = 0;
//
//				if (measure->includes.empty())
//				{
//					// First get the total CPU value
//					longvalue = GetPerfData(L"Processor", L"_Total", L"% Processor Time");
//					newValue = longvalue;
//
//					// Then substract the excluded processes
//					std::vector< std::wstring >::iterator j = measure->excludes.begin();
//					for( ; j != measure->excludes.end(); j++)
//					{
//						longvalue = GetPerfData(L"Process", (*j).c_str(), L"% Processor Time");
//						newValue += longvalue;		// Adding means actually substraction
//					}
//
//					// Compare with the old value
//					if(measure->oldValue != 0) 
//					{
//						int val = 10000000 - (UINT)(newValue - measure->oldValue);
//						if (val < 0) val = 0;
//						value = val;
//					}
//					measure->oldValue = newValue;
//				}
//				else
//				{
//					// Add the included processes
//					std::vector< std::wstring >::iterator j = measure->includes.begin();
//					for( ; j != measure->includes.end(); j++)
//					{
//						longvalue = GetPerfData(L"Process", (*j).c_str(), L"% Processor Time");
//						newValue += longvalue;
//					}
//
//					// Compare with the old value
//					if(measure->oldValue != 0) 
//					{
//						value = (UINT)(newValue - measure->oldValue);
//					}
//					measure->oldValue = newValue;
//
//				}
//
//		}
	}

	return (double)newValue;
}

/*
  This function is called when the value should be
  returned as a string.
*/
LPCTSTR GetString(UINT id, UINT flags) 
{
	std::map<UINT, CPUMeasure*>::iterator i = g_CPUMeasures.find(id);
	if(i != g_CPUMeasures.end())
	{
		CPUMeasure* measure = (*i).second;

		if (measure->topProcess == 2) 
		{
			return measure->topProcessName.c_str();
		}
	}

	return NULL;
}

/*
  If the measure needs to free resources before quitting.
  The plugin can export Finalize function, which is called
  when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	// delete the measure
	std::map<UINT, CPUMeasure*>::iterator i = g_CPUMeasures.find(id);
	if(i != g_CPUMeasures.end())
	{
		delete (*i).second;
		g_CPUMeasures.erase(i);
	}

	CPerfSnapshot::CleanUp();
}

/*
  This updates the process status
*/
void UpdateProcesses()
{
	CPerfObject* pPerfObj;
	CPerfObjectInstance* pObjInst;
	CPerfCounter* pPerfCntr;
	BYTE data[256];
	WCHAR name[256];

	std::vector< ProcessValues > newProcesses;

	CPerfSnapshot snapshot(&g_CounterTitles);
	CPerfObjectList objList(&snapshot, &g_CounterTitles);

	if(snapshot.TakeSnapshot(L"Process"))
	{
		pPerfObj = objList.GetPerfObject(L"Process");

		if(pPerfObj)
		{
			for(pObjInst = pPerfObj->GetFirstObjectInstance();
				pObjInst != NULL;
				pObjInst = pPerfObj->GetNextObjectInstance())
			{
				if(pObjInst->GetObjectInstanceName(name, 256))
				{
					if (_wcsicmp(name, L"_Total") == 0)
					{
						continue;
					}

					pPerfCntr = pObjInst->GetCounterByName(L"% Processor Time");
					if(pPerfCntr != NULL)
					{
						pPerfCntr->GetData(data, 256, NULL);
						
						if(pPerfCntr->GetSize() == 8)
						{
							ProcessValues values;
							values.name = name;
							values.oldValue = 0;

							// Check if we can find the old value
							for (size_t i = 0; i < g_Processes.size(); i++) 
							{
								if (!g_Processes[i].found && g_Processes[i].name == name) 
								{
									values.oldValue = g_Processes[i].newValue;
									g_Processes[i].found = true;
									break;
								}
							}

							values.newValue = *(ULONGLONG*)data;
							values.found = false;
							newProcesses.push_back(values);

//							LSLog(LOG_DEBUG, L"Rainmeter", name);		// DEBUG
						}

						delete pPerfCntr;
					}
				}
				delete pObjInst;
			}
			delete pPerfObj;
		}
	}

	g_Processes = newProcesses;
}


UINT GetPluginVersion()
{
	return 1005;
}

LPCTSTR GetPluginAuthor()
{
	return L"Rainy (rainy@iki.fi)";
}

