// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasureVirtualMemory.h"
#include "ConfigParser.h"

MeasureVirtualMemory::MeasureVirtualMemory(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Total(false)
{
	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&stat);
	m_MaxValue = (double)(__int64)stat.ullTotalPageFile;
}

MeasureVirtualMemory::~MeasureVirtualMemory()
{
}

void MeasureVirtualMemory::UpdateValue()
{
	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&stat);
	m_MaxValue = (double)(__int64)stat.ullTotalPageFile;

	if (m_Total)
	{
		m_Value = m_MaxValue;
	}
	else
	{
		m_Value = (double)(__int64)(stat.ullTotalPageFile - stat.ullAvailPageFile);
	}
}

void MeasureVirtualMemory::ReadOptions(ConfigParser::OptionReader& reader)
{
	double oldMaxValue = m_MaxValue;
	Measure::ReadOptions(reader);
	m_MaxValue = oldMaxValue;

	m_Total = reader.ReadBool<"Total">(false);
}

