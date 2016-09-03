/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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

/*
** Updates the current physical memory value.
**
*/
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

/*
** Read the options specified in the ini file.
**
*/
void MeasurePhysicalMemory::ReadOptions(ConfigParser& parser, const WCHAR* section)
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

