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
  $Header: /home/cvsroot/Rainmeter/Library/MeasureUptime.cpp,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeasureUptime.cpp,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.7  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.6  2002/04/26 18:24:15  rainy
  Modified the Update method to support disabled measures.

  Revision 1.5  2002/03/31 09:58:54  rainy
  Added some comments

  Revision 1.4  2001/12/23 10:16:10  rainy
  The static variable is set to zero in destructor.

  Revision 1.3  2001/10/28 10:20:49  rainy
  GetStringValue uses consts

  Revision 1.2  2001/09/26 16:27:14  rainy
  Changed the interfaces a bit.

  Revision 1.1  2001/09/01 12:56:25  rainy
  Initial version.

*/
#pragma warning(disable: 4996)

#include "MeasureUptime.h"
#include "Rainmeter.h"
#include <time.h>

/*
** CMeasureUptime
**
** The constructor
**
*/
CMeasureUptime::CMeasureUptime(CMeterWindow* meterWindow) : CMeasure(meterWindow)
{
}

/*
** ~CMeasureUptime
**
** The destructor
**
*/
CMeasureUptime::~CMeasureUptime()
{
}

/*
** ReadConfig
**
** Reads the measure specific configs.
**
*/
void CMeasureUptime::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	CMeasure::ReadConfig(parser, section);

	m_Format = parser.ReadString(section, L"Format", L"%4!i!d %3!i!:%2!02i!");
}

/*
** Update
**
** Updates the current uptime
**
*/
bool CMeasureUptime::Update()
{
	if (!CMeasure::PreUpdate()) return false;

	DWORD ticks = GetTickCount();
	m_Value = ticks / 1000;

	return PostUpdate();
}

/*
** GetStringValue
**
** Returns the uptime as string.
**
*/
const WCHAR* CMeasureUptime::GetStringValue(bool autoScale, double scale, int decimals, bool percentual)
{
	static WCHAR buffer[MAX_LINE_LENGTH];

	size_t value = (size_t)m_Value;
	size_t time[4];

	time[0] = value % 60;
	time[1] = (value / 60) % 60;
	time[2] = (value / (60 * 60));
	time[3] =  (value / (60 * 60 * 24));

	if (m_Format.find(L"%4") != std::wstring::npos) 
	{
		time[2] %= 24;
	}

	FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, m_Format.c_str(), 0, 0, buffer, MAX_LINE_LENGTH, (char**)time);

	return CheckSubstitute(buffer);
}
