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

#include "StdAfx.h"
#include "MeasureUptime.h"
#include "Rainmeter.h"

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

	__try
	{
		FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, m_Format.c_str(), 0, 0, buffer, MAX_LINE_LENGTH, (char**)time);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DebugLog(L"Uptime: Invalid Format: Measure=[%s], Format=\"%s\"", m_Name.c_str(), m_Format.c_str());
		buffer[0] = 0;
	}

	return CheckSubstitute(buffer);
}
