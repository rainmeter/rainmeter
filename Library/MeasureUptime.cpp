/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureUptime.h"
#include "Rainmeter.h"
#include "System.h"

MeasureUptime::MeasureUptime(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_AddDaysToHours(false),
	m_Seconds(0.0),
	m_SecondsDefined(false)
{
}

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

	// Don't allow negative seconds
	m_SecondsDefined = false;
	std::wstring seconds = parser.ReadString(section, L"SecondsValue", L"");
	if (!seconds.empty())
	{
		m_SecondsDefined = true;
		m_Seconds = (double)abs((__int64)parser.ReadFloat(section, L"SecondsValue", 0.0));
	}
}

/*
** Updates the current uptime
**
*/
void MeasureUptime::UpdateValue()
{
	if (!m_SecondsDefined)
	{
		ULONGLONG ticks = System::GetTickCount64();
		m_Value = (double)(__int64)(ticks / 1000);
	}
	else
	{
		m_Value = m_Seconds;
	}
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
