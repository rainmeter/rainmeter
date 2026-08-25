// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasurePerfMon.h"
#include "ConfigParser.h"
#include "../Common/PdhUtil.h"

MeasurePerfMon::MeasurePerfMon(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_ObjectName(),
	m_CounterName(),
	m_InstanceName(),
	m_Query(nullptr),
	m_Counter(nullptr),
	m_FirstInstance(false),
	m_Buffer(),
	m_OldValue(),
	m_Difference(false),
	m_FirstTime(true)
{
}

MeasurePerfMon::~MeasurePerfMon()
{
	CloseQuery();
}

// Opens a query for the counter the measure was configured with. Whether an object has instances at
// all only shows in whether the path resolves, so an object that was not given an instance is tried
// both ways.
bool MeasurePerfMon::OpenQuery()
{
	if (PdhOpenQuery(nullptr, 0, &m_Query) != ERROR_SUCCESS)
	{
		m_Query = nullptr;
		return false;
	}

	if (m_InstanceName.empty())
	{
		if (PdhUtil::AddEnglishCounter(m_Query, m_ObjectName, m_CounterName, nullptr, &m_Counter)) return true;

		// Reading the first instance is what the measure has always done when it was not told which
		// one to read
		if (PdhUtil::AddEnglishCounter(m_Query, m_ObjectName, m_CounterName, L"*", &m_Counter))
		{
			m_FirstInstance = true;
			return true;
		}
	}
	else if (PdhUtil::AddEnglishCounter(m_Query, m_ObjectName, m_CounterName, m_InstanceName.c_str(), &m_Counter))
	{
		return true;
	}

	CloseQuery();
	return false;
}

void MeasurePerfMon::CloseQuery()
{
	if (m_Query)
	{
		PdhCloseQuery(m_Query);   // Also closes every counter that was added to the query
		m_Query = nullptr;
	}

	m_Counter = nullptr;
	m_FirstInstance = false;
}

bool MeasurePerfMon::GetRawValue(ULONGLONG& value)
{
	if (PdhCollectQueryData(m_Query) != ERROR_SUCCESS) return false;

	if (m_FirstInstance)
	{
		DWORD count = 0;
		if (!PdhUtil::GetRawArray(m_Counter, m_Buffer, count) || count == 0) return false;

		const auto items = (const PDH_RAW_COUNTER_ITEM*)m_Buffer.data();
		if (!PdhUtil::IsValidStatus(items[0].RawValue.CStatus)) return false;

		value = (ULONGLONG)items[0].RawValue.FirstValue;
		return true;
	}

	PDH_RAW_COUNTER raw = { 0 };
	if (PdhGetRawCounterValue(m_Counter, nullptr, &raw) != ERROR_SUCCESS) return false;
	if (!PdhUtil::IsValidStatus(raw.CStatus)) return false;

	value = (ULONGLONG)raw.FirstValue;
	return true;
}

void MeasurePerfMon::ReadOptions(ConfigParser& parser, std::wstring_view section)
{
	Measure::ReadOptions(parser, section);

	bool changed = false;

	std::wstring value = parser.ReadString(section, L"PerfMonObject", L"");
	if (_wcsicmp(value.c_str(), m_ObjectName.c_str()) != 0)
	{
		m_ObjectName = value;
		changed = true;
	}

	parser.ReadString(value, section, L"PerfMonCounter", L"");
	if (_wcsicmp(value.c_str(), m_CounterName.c_str()) != 0)
	{
		m_CounterName = value;
		changed = true;
	}

	parser.ReadString(value, section, L"PerfMonInstance", L"");
	if (_wcsicmp(value.c_str(), m_InstanceName.c_str()) != 0)
	{
		m_InstanceName = value;
		changed = true;
	}

	const bool difference = parser.ReadBool(section, L"PerfMonDifference", true);
	if (difference != m_Difference)
	{
		m_Difference = difference;
		m_OldValue = 0;
		m_FirstTime = true;
	}

	if (!changed) return;

	// Which counter is being read is what a query is built around, so it has to be built again
	CloseQuery();
	m_OldValue = 0;
	m_FirstTime = true;

	if (m_ObjectName.empty() || m_CounterName.empty()) return;

	OpenQuery();
}

void MeasurePerfMon::UpdateValue()
{
	// A counter that cannot be read measures zero, as it always has
	ULONGLONG value = 0;
	if (m_Query && !GetRawValue(value))
	{
		value = 0;
	}

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
