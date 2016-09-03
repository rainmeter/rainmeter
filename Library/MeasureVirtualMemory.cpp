/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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

/*
** Updates the current virtual memory value.
**
*/
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

/*
** Read the options specified in the ini file.
**
*/
void MeasureVirtualMemory::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	double oldMaxValue = m_MaxValue;
	Measure::ReadOptions(parser, section);
	m_MaxValue = oldMaxValue;

	m_Total = parser.ReadBool(section, L"Total", false);
}

