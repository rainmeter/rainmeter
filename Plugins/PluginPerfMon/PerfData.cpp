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
#include "PerfData.h"
#include <string>
#include <map>
#include <crtdbg.h>
#include "..\..\Library\Export.h"	// Rainmeter's exported functions

ULONGLONG GetPerfData(PCTSTR ObjectName, PCTSTR InstanceName, PCTSTR CounterName);

struct PerfMeasure
{
	std::wstring ObjectName;
	std::wstring CounterName;
	std::wstring InstanceName;
	bool        Difference;
	ULONGLONG   OldValue;
	bool        FirstTime;
};

static CPerfTitleDatabase g_CounterTitles( PERF_TITLE_COUNTER );
static std::map<UINT, PerfMeasure*> g_Measures;

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
	PerfMeasure* measure = new PerfMeasure;
	measure->Difference = false;
	measure->FirstTime = true;
	measure->OldValue = 0;

	LPCTSTR buffer = ReadConfigString(section, L"PerfMonObject", L"");
	if (buffer)
	{
 		measure->ObjectName = buffer;
	}

	buffer = ReadConfigString(section, L"PerfMonCounter", L"");
	if (buffer)
	{
 		measure->CounterName = buffer;
	}

	buffer = ReadConfigString(section, L"PerfMonInstance", L"");
	if (buffer)
	{
 		measure->InstanceName = buffer;
	}

	buffer = ReadConfigString(section, L"PerfMonDifference", L"1");
	if (buffer)
	{
 		measure->Difference = 1==_wtoi(buffer);
	}

	// Store the measure
	g_Measures[id] = measure;

	int maxVal = 0;
	buffer = ReadConfigString(section, L"PerfMonMaxValue", L"0");
	if (buffer)
	{
 		maxVal = _wtoi(buffer);
	}

	return maxVal;
}

/*
  This function is called when new value should be measured.
  The function returns the new value.
*/
double Update2(UINT id)
{
	double value = 0;

	std::map<UINT, PerfMeasure*>::iterator i = g_Measures.find(id);
	if(i != g_Measures.end())
	{
		PerfMeasure* measure = (*i).second;

		if(measure)
		{
			// Check the platform
			OSVERSIONINFO osvi;
			ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
			osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			if(GetVersionEx((OSVERSIONINFO*)&osvi) && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion > 4)
			{
				ULONGLONG longvalue = 0;
				longvalue = GetPerfData(measure->ObjectName.c_str(), 
										measure->InstanceName.c_str(), 
										measure->CounterName.c_str());

				if(measure->Difference)
				{
					// Compare with the old value
					if(!measure->FirstTime) 
					{
						value = (double)(longvalue - measure->OldValue);
					}
					measure->OldValue = longvalue;
					measure->FirstTime = false;
				}
				else
				{
					value = (double)longvalue;
				}
			}
			else
			{
				LSLog(LOG_DEBUG, L"Rainmeter", L"PerfMon plugin works only in Win2K and later.");
			}
		}
	}

	return value;
}

/*
  If the measure needs to free resources before quitting.
  The plugin can export Finalize function, which is called
  when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	// delete the measure
	std::map<UINT, PerfMeasure*>::iterator i = g_Measures.find(id);
	if(i != g_Measures.end())
	{
		delete (*i).second;
		g_Measures.erase(i);
	}

	CPerfSnapshot::CleanUp();
}

/*
  This method gets value of the given perfmon counter.
*/
ULONGLONG GetPerfData(PCTSTR ObjectName, PCTSTR InstanceName, PCTSTR CounterName)
{
	CPerfObject* pPerfObj;
	CPerfObjectInstance* pObjInst;
	CPerfCounter* pPerfCntr;
	BYTE data[256];
	WCHAR name[256];
	ULONGLONG value = 0;

	if(ObjectName == NULL || CounterName == NULL || wcslen(ObjectName) == 0 || wcslen(CounterName) == 0)
	{
		// Unable to continue
		return 0;
	}

	CPerfSnapshot snapshot(&g_CounterTitles);
	CPerfObjectList objList(&snapshot, &g_CounterTitles);

	if(snapshot.TakeSnapshot(ObjectName))
	{
		pPerfObj = objList.GetPerfObject(ObjectName);

		if(pPerfObj)
		{
			for(pObjInst = pPerfObj->GetFirstObjectInstance();
				pObjInst != NULL;
				pObjInst = pPerfObj->GetNextObjectInstance())
			{
				if (InstanceName != NULL && wcslen(InstanceName) > 0)
				{
					if(pObjInst->GetObjectInstanceName(name, 256))
					{
						if(_wcsicmp(InstanceName, name) != 0) 
						{
							delete pObjInst;
							continue;
						}
					}
					else
					{
						delete pObjInst;
						continue;
					}
				}

				pPerfCntr = pObjInst->GetCounterByName(CounterName);
				if(pPerfCntr != NULL)
				{
					pPerfCntr->GetData(data, 256, NULL);
					
					if(pPerfCntr->GetSize() == 1)
					{
						value = *(BYTE*)data;
					} 
					else if(pPerfCntr->GetSize() == 2)
					{
						value = *(WORD*)data;
					}
					else if(pPerfCntr->GetSize() == 4)
					{
						value = *(DWORD*)data;
					}
					else if(pPerfCntr->GetSize() == 8)
					{
						value = *(ULONGLONG*)data;
					}

					delete pPerfCntr;
					delete pObjInst;
					break;	// No need to continue
				}
				delete pObjInst;
			}
			delete pPerfObj;
		}
	}

	return value;
}

UINT GetPluginVersion()
{
	return 1002;
}

LPCTSTR GetPluginAuthor()
{
	return L"Rainy (rainy@iki.fi)";
}


