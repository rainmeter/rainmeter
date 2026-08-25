// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasureAdvancedCPU.h"
#include "ConfigParser.h"
#include "../Common/PdhUtil.h"
#include <pdh.h>

namespace {

// Keeps the processor time of every process up to date, shared by the measures reading it
class ProcessCollector
{
public:
	struct Process
	{
		std::wstring name;
		LONGLONG oldValue;
		LONGLONG newValue;
		bool found;
	};

	static ProcessCollector& GetInstance()
	{
		static ProcessCollector s_Collector;
		return s_Collector;
	}

	ProcessCollector(const ProcessCollector&) = delete;
	ProcessCollector& operator=(const ProcessCollector&) = delete;

	void AddReference();
	void ReleaseReference();

	// Reads the category again if what it holds has gone stale, and returns what it has either way
	const std::vector<Process>& UpdateProcesses();

private:
	ProcessCollector() = default;

	void Collect();

	PDH_HQUERY m_Query = nullptr;
	PDH_HCOUNTER m_Counter = nullptr;
	UINT m_References = 0;

	std::vector<Process> m_Processes;

	// Kept between collections so that reading the category does not have to go back to the heap for
	// a buffer that settles at a stable size
	std::vector<BYTE> m_Buffer;

	ULONGLONG m_OldTime = 0;
};

void ProcessCollector::AddReference()
{
	if (++m_References > 1) return;

	if (PdhOpenQuery(nullptr, 0, &m_Query) != ERROR_SUCCESS)
	{
		m_Query = nullptr;
		return;
	}

	// The English name is the only one the measure has ever accepted
	if (!PdhUtil::AddEnglishCounter(m_Query, L"Process", L"% Processor Time", L"*", &m_Counter))
	{
		PdhCloseQuery(m_Query);
		m_Query = nullptr;
		m_Counter = nullptr;
	}
}

void ProcessCollector::ReleaseReference()
{
	if (m_References == 0 || --m_References > 0) return;

	if (m_Query)
	{
		PdhCloseQuery(m_Query);   // Also closes every counter that was added to the query
		m_Query = nullptr;
		m_Counter = nullptr;
	}

	// What was collected goes with the query rather than outliving it, so that a measure loaded
	// later does not take a difference against values of unknown age
	m_Processes.clear();
	m_Processes.shrink_to_fit();
	m_Buffer.clear();
	m_Buffer.shrink_to_fit();
	m_OldTime = 0;
}

const std::vector<ProcessCollector::Process>& ProcessCollector::UpdateProcesses()
{
	// The category is read no more often than this, however fast the measures using it update
	const ULONGLONG updateRate = 500;

	const ULONGLONG time = GetTickCount64();
	if (m_OldTime == 0 || time - m_OldTime > updateRate)
	{
		Collect();
		m_OldTime = time;
	}

	return m_Processes;
}

// Reads the processor time of every process, pairing each one with what it had read the last time
// so that the measures can take the difference. A category that cannot be read leaves the list
// empty, which reports zero rather than a difference against values of unknown age.
void ProcessCollector::Collect()
{
	std::vector<Process> processes;

	DWORD count = 0;
	if (m_Query && PdhCollectQueryData(m_Query) == ERROR_SUCCESS &&
		PdhUtil::GetRawArray(m_Counter, m_Buffer, count))
	{
		const auto items = (const PDH_RAW_COUNTER_ITEM*)m_Buffer.data();
		processes.reserve(count);

		for (DWORD i = 0; i < count; ++i)
		{
			if (_wcsicmp(items[i].szName, L"_Total") == 0) continue;
			if (!PdhUtil::IsValidStatus(items[i].RawValue.CStatus)) continue;

			Process process;
			process.name = items[i].szName;
			process.oldValue = 0;
			process.newValue = items[i].RawValue.FirstValue;
			process.found = false;

			// Processes that share a name are told apart by a "#1" suffix, so a name matches at most
			// one of the values collected before it
			for (auto& previous : m_Processes)
			{
				if (!previous.found && _wcsicmp(previous.name.c_str(), process.name.c_str()) == 0)
				{
					process.oldValue = previous.newValue;
					previous.found = true;
					break;
				}
			}

			processes.push_back(std::move(process));
		}
	}

	m_Processes = std::move(processes);
}

}  // namespace

MeasureAdvancedCPU::MeasureAdvancedCPU(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Includes(),
	m_Excludes(),
	m_IncludesCache(),
	m_ExcludesCache(),
	m_TopProcess(-1),
	m_TopProcessName()
{
	ProcessCollector::GetInstance().AddReference();
}

MeasureAdvancedCPU::~MeasureAdvancedCPU()
{
	ProcessCollector::GetInstance().ReleaseReference();
}

void MeasureAdvancedCPU::ReadOptions(ConfigParser& parser, std::wstring_view section)
{
	Measure::ReadOptions(parser, section);

	std::wstring value = parser.ReadString(section, L"CPUInclude", L"");
	if (_wcsicmp(value.c_str(), m_IncludesCache.c_str()) != 0)
	{
		m_IncludesCache = value;
		ReadProcessList(value, m_Includes);
	}

	parser.ReadString(value, section, L"CPUExclude", L"");
	if (_wcsicmp(value.c_str(), m_ExcludesCache.c_str()) != 0)
	{
		m_ExcludesCache = value;
		ReadProcessList(value, m_Excludes);
	}

	m_TopProcess = parser.ReadInt(section, L"TopProcess", 0);

	if (!parser.IsValueDefined(section, L"MaxValue"))
	{
		m_MaxValue = 10000000.0;
		m_LogMaxValue = false;
	}
}

void MeasureAdvancedCPU::UpdateValue()
{
	LONGLONG newValue = 0;

	for (const auto& process : ProcessCollector::GetInstance().UpdateProcesses())
	{
		if (CheckProcess(process.name.c_str()) && process.oldValue != 0)
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

	size_t start = 0;
	while (start < value.length())
	{
		const size_t end = value.find(L';', start);
		const size_t length = (end == std::wstring::npos) ? std::wstring::npos : end - start;

		if (length != 0)
		{
			processList.emplace_back(value.substr(start, length));
		}

		if (end == std::wstring::npos)
		{
			break;
		}

		start = end + 1;
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
