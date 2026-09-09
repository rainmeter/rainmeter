// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasureMemory.h"
#include "ConfigParser.h"

MeasureMemory::MeasureMemory(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Total(false)
{
	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&stat);
	m_MaxValue = (double)(__int64)(stat.ullTotalPageFile + stat.ullTotalPhys);
}

MeasureMemory::~MeasureMemory()
{
}

void MeasureMemory::UpdateValue()
{
	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&stat);
	m_MaxValue = (double)(__int64)(stat.ullTotalPageFile + stat.ullTotalPhys);

	if (m_Total)
	{
		m_Value = m_MaxValue;
	}
	else
	{
		m_Value = (double)(__int64)(stat.ullTotalPageFile + stat.ullTotalPhys - stat.ullAvailPageFile - stat.ullAvailPhys);
	}
}

void MeasureMemory::ReadOptions(ConfigParser::OptionReader& reader)
{
	double oldMaxValue = m_MaxValue;
	Measure::ReadOptions(reader);
	m_MaxValue = oldMaxValue;

	m_Total = reader.ReadBool<"Total">(false);
}
