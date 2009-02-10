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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
/*
  $Header: /home/cvsroot/Rainmeter/Library/MeasureMemory.cpp,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeasureMemory.cpp,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.8  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.7  2002/04/26 18:24:16  rainy
  Modified the Update method to support disabled measures.

  Revision 1.6  2002/03/31 09:58:54  rainy
  Added some comments

  Revision 1.5  2001/09/26 16:27:15  rainy
  Changed the interfaces a bit.

  Revision 1.4  2001/09/01 13:00:10  rainy
  Slight changes in the interface. The value is now measured only once if possible.

  Revision 1.3  2001/08/19 09:15:21  rainy
  Added support for value invert.

  Revision 1.2  2001/08/12 15:45:45  Rainy
  Adjusted Update()'s interface.

  Revision 1.1.1.1  2001/08/11 10:58:19  Rainy
  Added to CVS.

*/
#pragma warning(disable: 4996)

#include "MeasureMemory.h"

/*
** CMeasureMemory
**
** The constructor
**
*/
CMeasureMemory::CMeasureMemory(CMeterWindow* meterWindow) : CMeasure(meterWindow)
{
	m_Total = false;
}

/*
** ~CMeasureMemory
**
** The destructor
**
*/
CMeasureMemory::~CMeasureMemory()
{
}

/*
** Update
**
** Updates the current total memory value. 
**
*/
bool CMeasureMemory::Update()
{
	if (!CMeasure::PreUpdate()) return false;

	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&stat);		// Doesn't measure values > 4GB. Should use GlobalMemoryStatusEx instead, but that requires Win2k.
	if (m_Total)
	{
		m_Value = (double)(__int64)(stat.ullTotalPageFile + stat.ullTotalPhys);
	}
	else
	{
		m_Value = (double)(__int64)(stat.ullTotalPageFile + stat.ullTotalPhys - stat.ullAvailPageFile - stat.ullAvailPhys);
	}

	return PostUpdate();
}

/*
** ReadConfig
**
** Reads the measure specific configs.
**
*/
void CMeasureMemory::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	CMeasure::ReadConfig(parser, section);

	m_Total = (1 == parser.ReadInt(section, L"Total", 0));

	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&stat);
	m_MaxValue = (double)(__int64)(stat.ullTotalPageFile + stat.ullTotalPhys);
}

