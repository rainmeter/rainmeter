/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureAdvancedCPU.h"
#include "ConfigParser.h"
#include "PerfCounter/Titledb.h"
#include "PerfCounter/PerfSnap.h"
#include "PerfCounter/PerfObj.h"
#include "PerfCounter/PerfCntr.h"
#include "PerfCounter/ObjList.h"
#include "PerfCounter/ObjInst.h"

struct ProcessValues
{
	std::wstring name;
	LONGLONG oldValue;
	LONGLONG newValue;
	bool found;
};

static void UpdateProcesses(std::vector<ProcessValues>& processes);

MeasureAdvancedCPU::MeasureAdvancedCPU(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Includes(),
	m_Excludes(),
	m_IncludesCache(),
	m_ExcludesCache(),
	m_TopProcess(-1),
	m_TopProcessName()
{
}

MeasureAdvancedCPU::~MeasureAdvancedCPU()
{
	CPerfSnapshot::CleanUp();
}

void MeasureAdvancedCPU::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	std::wstring value = parser.ReadString(section, L"CPUInclude", L"");
	if (_wcsicmp(value.c_str(), m_IncludesCache.c_str()) != 0)
	{
		m_IncludesCache = value;
		ReadProcessList(value, m_Includes);
	}

	value = parser.ReadString(section, L"CPUExclude", L"");
	if (_wcsicmp(value.c_str(), m_ExcludesCache.c_str()) != 0)
	{
		m_ExcludesCache = value;
		ReadProcessList(value, m_Excludes);
	}

	m_TopProcess = parser.ReadInt(section, L"TopProcess", 0);

	if (!m_MaxValueDefined)
	{
		m_MaxValue = 10000000.0;
		m_LogMaxValue = false;
	}
}

void MeasureAdvancedCPU::UpdateValue()
{
	static std::vector<ProcessValues> s_Processes;
	static ULONGLONG s_OldTime = 0ULL;

	const ULONGLONG time = GetTickCount64();
	if (s_OldTime == 0ULL || time - s_OldTime > 500ULL)
	{
		UpdateProcesses(s_Processes);
		s_OldTime = time;
	}

	LONGLONG newValue = 0LL;

	for (const auto& process : s_Processes)
	{
		if (CheckProcess(process.name.c_str()) && process.oldValue != 0LL)
		{
			const LONGLONG value = process.newValue - process.oldValue;

			if (m_TopProcess == 0)
			{
				newValue += value;
			}
			else if (newValue < value)
			{
				newValue = value;
				m_TopProcessName = process.name;
			}
		}
	}

	m_Value = (double)newValue;
}

const WCHAR* MeasureAdvancedCPU::GetStringValue()
{
	return m_TopProcess == 2 ? CheckSubstitute(m_TopProcessName.c_str()) : nullptr;
}

void MeasureAdvancedCPU::ReadProcessList(const std::wstring& value, std::vector<std::wstring>& processList)
{
	processList.clear();

	size_t start = 0U;
	while (start < value.length())
	{
		const size_t end = value.find(L';', start);
		const size_t length = (end == std::wstring::npos) ? std::wstring::npos : end - start;

		if (length != 0U)
		{
			processList.emplace_back(value.substr(start, length));
		}

		if (end == std::wstring::npos)
		{
			break;
		}

		start = end + 1U;
	}
}

bool MeasureAdvancedCPU::CheckProcess(const WCHAR* name)
{
	if (m_Includes.empty())
	{
		for (const auto& exclude : m_Excludes)
		{
			if (_wcsicmp(exclude.c_str(), name) == 0)
			{
				return false;
			}
		}

		return true;
	}

	for (const auto& include : m_Includes)
	{
		if (_wcsicmp(include.c_str(), name) == 0)
		{
			return true;
		}
	}

	return false;
}

static void UpdateProcesses(std::vector<ProcessValues>& processes)
{
	static CPerfTitleDatabase s_CounterTitles(PERF_TITLE_COUNTER);
	BYTE data[256] = { 0 };
	WCHAR name[256] = { 0 };

	std::vector<ProcessValues> newProcesses;
	newProcesses.reserve(processes.size());

	CPerfSnapshot snapshot(&s_CounterTitles);
	CPerfObjectList objList(&snapshot, &s_CounterTitles);

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
						pObjInst = nullptr;
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
							values.oldValue = 0LL;
							values.newValue = *(ULONGLONG*)data;
							values.found = false;

							for (auto& process : processes)
							{
								if (!process.found && _wcsicmp(process.name.c_str(), name) == 0)
								{
									values.oldValue = process.newValue;
									process.found = true;
									break;
								}
							}

							newProcesses.push_back(std::move(values));
						}

						delete pPerfCntr;
						pPerfCntr = nullptr;
					}
				}

				delete pObjInst;
				pObjInst = nullptr;
			}

			delete pPerfObj;
			pPerfObj = nullptr;
		}
	}

	processes = std::move(newProcesses);
}
