/*
  Copyright (C) 2001 Kimmo Pekkola

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "MeasurePhysicalMemory.h"
#include "ConfigParser.h"

/*
** The constructor
**
*/
MeasurePhysicalMemory::MeasurePhysicalMemory(MeterWindow* meterWindow, const WCHAR* name) : Measure(meterWindow, name),
	m_Total(false)
{
	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&stat);
	m_MaxValue = (double)(__int64)stat.ullTotalPhys;
}

/*
** The destructor
**
*/
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

