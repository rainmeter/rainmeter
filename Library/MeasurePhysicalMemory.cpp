// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasurePhysicalMemory.h"
#include "ConfigParser.h"

MeasurePhysicalMemory::MeasurePhysicalMemory(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Total(false)
{
	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&stat);
	m_MaxValue = (double)(__int64)stat.ullTotalPhys;
}

MeasurePhysicalMemory::~MeasurePhysicalMemory()
{
}

void MeasurePhysicalMemory::UpdateValue()
{
	if (!m_Total)
	{
		MEMORYSTATUSEX stat;
		stat.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&stat);

		m_Value = (double)(__int64)(stat.ullTotalPhys - stat.ullAvailPhys);
	}
}

void MeasurePhysicalMemory::ReadOptions(ConfigParser& parser, std::wstring_view section)
{
	double oldMaxValue = m_MaxValue;
	Measure::ReadOptions(parser, section);
	m_MaxValue = oldMaxValue;

	m_Total = parser.ReadBool(section, L"Total", false);
	if (m_Total)
	{
		m_Value = m_MaxValue;
	}
}

