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
#include "MeasureUptime.h"
#include "Rainmeter.h"
#include "System.h"

/*
** The constructor
**
*/
MeasureUptime::MeasureUptime(MeterWindow* meterWindow, const WCHAR* name) : Measure(meterWindow, name),
	m_AddDaysToHours(false)
{
}

/*
** The destructor
**
*/
MeasureUptime::~MeasureUptime()
{
}

/*
** Read the options specified in the ini file.
**
*/
void MeasureUptime::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	m_Format = parser.ReadString(section, L"Format", L"%4!i!d %3!i!:%2!02i!");

	if (m_Format.find(L"%4") == std::wstring::npos)
	{
		m_AddDaysToHours = parser.ReadBool(section, L"AddDaysToHours", true);
	}
	else
	{
		m_AddDaysToHours = false;
	}
}

/*
** Updates the current uptime
**
*/
void MeasureUptime::UpdateValue()
{
	ULONGLONG ticks = System::GetTickCount64();
	m_Value = (double)(__int64)(ticks / 1000);
}

/*
** Returns the uptime as string.
**
*/
const WCHAR* MeasureUptime::GetStringValue()
{
	static WCHAR buffer[MAX_LINE_LENGTH];

	size_t value = (size_t)m_Value;
	size_t time[4];

	time[0] = value % 60;
	time[1] = (value / 60) % 60;
	time[2] = (value / (60 * 60));
	time[3] = (value / (60 * 60 * 24));

	if (!m_AddDaysToHours)
	{
		time[2] %= 24;
	}

	__try
	{
		FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, m_Format.c_str(), 0, 0, buffer, MAX_LINE_LENGTH, (char**)time);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		LogErrorF(this, L"Uptime: \"Format=%s\" invalid", m_Format.c_str());
		buffer[0] = 0;
	}

	return CheckSubstitute(buffer);
}
