/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasurePerfMon.h"
#include "ConfigParser.h"
#include "PerfCounter/Titledb.h"
#include "PerfCounter/PerfSnap.h"
#include "PerfCounter/PerfObj.h"
#include "PerfCounter/PerfCntr.h"
#include "PerfCounter/ObjList.h"
#include "PerfCounter/ObjInst.h"

MeasurePerfMon::MeasurePerfMon(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_ObjectName(),
	m_CounterName(),
	m_InstanceName(),
	m_OldValue(),
	m_Difference(false),
	m_FirstTime(true)
{
}

MeasurePerfMon::~MeasurePerfMon()
{
	CPerfSnapshot::CleanUp();
}

void MeasurePerfMon::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	bool changed = false;

	std::wstring value = parser.ReadString(section, L"PerfMonObject", L"");
	if (_wcsicmp(value.c_str(), m_ObjectName.c_str()) != 0)
	{
		m_ObjectName = value;
		changed = true;
	}

	value = parser.ReadString(section, L"PerfMonCounter", L"");
	if (_wcsicmp(value.c_str(), m_CounterName.c_str()) != 0)
	{
		m_CounterName = value;
		changed = true;
	}

	value = parser.ReadString(section, L"PerfMonInstance", L"");
	if (_wcsicmp(value.c_str(), m_InstanceName.c_str()) != 0)
	{
		m_InstanceName = value;
		changed = true;
	}

	const bool difference = parser.ReadBool(section, L"PerfMonDifference", true);
	if (difference != m_Difference)
	{
		m_Difference = difference;
		changed = true;
	}

	if (changed)
	{
		m_OldValue = 0ULL;
		m_FirstTime = true;
	}
}

void MeasurePerfMon::UpdateValue()
{
	const ULONGLONG value = GetPerfData(
		m_ObjectName.c_str(),
		m_InstanceName.c_str(),
		m_CounterName.c_str());

	if (m_Difference)
	{
		m_Value = m_FirstTime ? 0.0 : (double)(value - m_OldValue);
		m_OldValue = value;
		m_FirstTime = false;
	}
	else
	{
		m_Value = (double)value;
	}
}

ULONGLONG MeasurePerfMon::GetPerfData(const WCHAR* objectName, const WCHAR* instanceName, const WCHAR* counterName)
{
	static CPerfTitleDatabase s_TitleCounter(PERF_TITLE_COUNTER);

	BYTE data[256];
	WCHAR name[256];
	ULONGLONG value = 0ULL;

	CPerfSnapshot snapshot(&s_TitleCounter);
	CPerfObjectList objList(&snapshot, &s_TitleCounter);

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
							pObjInst = nullptr;
							continue;
						}
					}
					else
					{
						delete pObjInst;
						pObjInst = nullptr;
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
					pPerfCntr = nullptr;
					delete pObjInst;
					pObjInst = nullptr;
					break;
				}

				if (pObjInst)
				{
					delete pObjInst;
					pObjInst = nullptr;
				}
			}

			delete pPerfObj;
			pPerfObj = nullptr;
		}
	}

	return value;
}
