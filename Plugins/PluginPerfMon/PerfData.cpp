/* Copyright (C) 2004 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include <windows.h>
#include <vector>
#include "Titledb.h"
#include "PerfSnap.h"
#include "PerfObj.h"
#include "PerfCntr.h"
#include "ObjList.h"
#include "ObjInst.h"
#include "../API/RainmeterAPI.h"
#include "../../Common/RawString.h"

struct MeasureData
{
	RawString objectName;
	RawString counterName;
	RawString instanceName;
	ULONGLONG oldValue;
	bool difference;
	bool firstTime;

	MeasureData() :
		oldValue(),
		difference(false),
		firstTime(true)
	{
	}
};

static CPerfTitleDatabase g_TitleCounter(PERF_TITLE_COUNTER);

ULONGLONG GetPerfData(PCTSTR ObjectName, PCTSTR InstanceName, PCTSTR CounterName);

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData;
	*data = measure;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;
	bool changed = false;

	LPCWSTR value = RmReadString(rm, L"PerfMonObject", L"");
	if (_wcsicmp(value, measure->objectName.c_str()) != 0)
	{
		measure->objectName = value;
		changed = true;
	}

	value = RmReadString(rm, L"PerfMonCounter", L"");
	if (_wcsicmp(value, measure->counterName.c_str()) != 0)
	{
		measure->counterName = value;
		changed = true;
	}

	value = RmReadString(rm, L"PerfMonInstance", L"");
	if (_wcsicmp(value, measure->instanceName.c_str()) != 0)
	{
		measure->instanceName = value;
		changed = true;
	}

	bool diff = RmReadInt(rm, L"PerfMonDifference", 1) == 1;
	if (diff != measure->difference)
	{
		measure->difference = diff;
		changed = true;
	}

	if (changed)
	{
		measure->oldValue = 0;
		measure->firstTime = true;
		*maxValue = 0.0;
	}
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
	BYTE data[256];
	WCHAR name[256];
	ULONGLONG value = 0;

	CPerfSnapshot snapshot(&g_TitleCounter);
	CPerfObjectList objList(&snapshot, &g_TitleCounter);

	if (snapshot.TakeSnapshot(objectName))
	{
		CPerfObject* pPerfObj = objList.GetPerfObject(objectName);

		if (pPerfObj)
		{
			for (CPerfObjectInstance* pObjInst = pPerfObj->GetFirstObjectInstance();
				pObjInst != nullptr;
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
				if (pPerfCntr != nullptr)
				{
					pPerfCntr->GetData(data, 256, nullptr);

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
