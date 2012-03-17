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
#include "Titledb.h"
#include "PerfSnap.h"
#include "PerfObj.h"
#include "PerfCntr.h"
#include "ObjList.h"
#include "ObjInst.h"
#include "../API/RainmeterAPI.h"
#include "../../Library/RawString.h"
#include "../../Library/DisableThreadLibraryCalls.h"	// contains DllMain entry point

struct MeasureData
{
	CRawString objectName;
	CRawString counterName;
	CRawString instanceName;
	ULONGLONG oldValue;
	bool difference;
	bool firstTime;
};

ULONGLONG GetPerfData(PCTSTR ObjectName, PCTSTR InstanceName, PCTSTR CounterName);

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData;
	*data = measure;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;

	measure->objectName = RmReadString(rm, L"PerfMonObject", L"");
	measure->counterName = RmReadString(rm, L"PerfMonCounter", L"");
	measure->instanceName = RmReadString(rm, L"PerfMonInstance", L"");
	measure->difference = RmReadInt(rm, L"PerfMonDifference", 1) == 1;
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	double value = 0;

	ULONGLONG longValue = GetPerfData(
		measure->objectName.c_str(),
		measure->instanceName.c_str(),
		measure->counterName.c_str());

	if (measure->difference)
	{
		// Compare with the old value
		if (!measure->firstTime)
		{
			value = (double)(longValue - measure->oldValue);
		}

		measure->oldValue = longValue;
		measure->firstTime = false;
	}
	else
	{
		value = (double)longValue;
	}

	return value;
}

PLUGIN_EXPORT void Finalize(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	delete measure;

	CPerfSnapshot::CleanUp();
}

ULONGLONG GetPerfData(LPCWSTR objectName, LPCWSTR instanceName, LPCWSTR counterName)
{
	static CPerfTitleDatabase s_CounterTitles(PERF_TITLE_COUNTER);

	BYTE data[256];
	WCHAR name[256];
	ULONGLONG value = 0;

	CPerfSnapshot snapshot(&s_CounterTitles);
	CPerfObjectList objList(&snapshot, &s_CounterTitles);

	if (snapshot.TakeSnapshot(objectName))
	{
		CPerfObject* pPerfObj = objList.GetPerfObject(objectName);

		if (pPerfObj)
		{
			for (CPerfObjectInstance* pObjInst = pPerfObj->GetFirstObjectInstance();
				pObjInst != NULL;
				pObjInst = pPerfObj->GetNextObjectInstance())
			{
				if (*instanceName)
				{
					if (pObjInst->GetObjectInstanceName(name, 256))
					{
						if (_wcsicmp(instanceName, name) != 0)
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

				CPerfCounter* pPerfCntr = pObjInst->GetCounterByName(counterName);
				if (pPerfCntr != NULL)
				{
					pPerfCntr->GetData(data, 256, NULL);

					if (pPerfCntr->GetSize() == 1)
					{
						value = *(BYTE*)data;
					}
					else if (pPerfCntr->GetSize() == 2)
					{
						value = *(WORD*)data;
					}
					else if (pPerfCntr->GetSize() == 4)
					{
						value = *(DWORD*)data;
					}
					else if (pPerfCntr->GetSize() == 8)
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
