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
#include "MeasureVirtualMemory.h"
#include "ConfigParser.h"

/*
** The constructor
**
*/
CMeasureVirtualMemory::CMeasureVirtualMemory(CMeterWindow* meterWindow, const WCHAR* name) : CMeasure(meterWindow, name),
	m_Total(false)
{
}

/*
** The destructor
**
*/
CMeasureVirtualMemory::~CMeasureVirtualMemory()
{
}

/*
** Updates the current virtual memory value.
**
*/
void CMeasureVirtualMemory::UpdateValue()
{
	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&stat);
	if (m_Total)
	{
		m_Value = (double)(__int64)stat.ullTotalPageFile;
	}
	else
	{
		m_Value = (double)(__int64)(stat.ullTotalPageFile - stat.ullAvailPageFile);
	}
}

/*
** Reads the measure specific configs.
**
*/
void CMeasureVirtualMemory::ReadOptions(CConfigParser& parser, const WCHAR* section)
{
	CMeasure::ReadOptions(parser, section);

	m_Total = (1 == parser.ReadInt(section, L"Total", 0));

	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&stat);
	m_MaxValue = (double)(__int64)stat.ullTotalPageFile;
}

