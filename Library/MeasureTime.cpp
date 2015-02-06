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
#include "MeasureTime.h"
#include "Rainmeter.h"

const double LOCAL_TIMEZONE = DBL_MIN;

int GetYearDay(int year, int month, int day)
{
	static const int dates[] = {  0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
	int yearDay = dates[month - 1] + day;

	if (month > 2 && ((((year % 4) == 0) && ((year % 100) != 0)) || (year % 400) == 0))
	{
		++yearDay;
	}

	return yearDay - 1;
}

// GetTimeZoneChangeDate by: Henrik Haftmann
// Via: https://msdn.microsoft.com/en-us/library/windows/desktop/ms724421%28v=vs.85%29.aspx#1
bool GetTimeZoneChangeDate(SYSTEMTIME* st)
{
	FILETIME ft;
	int nWeek = st->wDay - 1;	//wDay is 1-5, convert to 0 based
	if (!st->wMonth || !st->wYear || nWeek >= 8) return false;

	int day = st->wDayOfWeek;
	if (nWeek >= 4)
	{
		nWeek = 3 - nWeek;

		// calculate last day and day-of-week of given month
		for (st->wDay = 31; !SystemTimeToFileTime(st, &ft); st->wDay--)
		{
			if (st->wDay == 28) return false;
		}

		FileTimeToSystemTime(&ft, st);
		int last = st->wDayOfWeek;
		st->wDayOfWeek = (WORD)day;
		day -= last;

		if (day <= 0) nWeek++;
	}
	else
	{
		st->wDay = 1;
		if (!SystemTimeToFileTime(st, &ft)) return false;

		FileTimeToSystemTime(&ft, st);
		int first = st->wDayOfWeek;
		st->wDayOfWeek = (WORD)day;
		day -= first;

		if (day < 0) nWeek++;
	}

	st->wDay += day + nWeek * 7;
	return true;
}

MeasureTime::MeasureTime(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Delta(),
	m_Time(),
	m_TimeStamp(-1),
	m_TimeStampType(INVALID),
	m_TimeZone(LOCAL_TIMEZONE),
	m_DaylightSavingTime(true)
{
}

MeasureTime::~MeasureTime()
{
}

/*
** Converts given time to string.
** This function is a wrapper function for wcsftime.
**
*/
void MeasureTime::TimeToString(WCHAR* buf, size_t bufLen, const WCHAR* format, const struct tm* time)
{
	if (bufLen > 0)
	{
		_invalid_parameter_handler oldHandler = _set_invalid_parameter_handler(RmNullCRTInvalidParameterHandler);
		_CrtSetReportMode(_CRT_ASSERT, 0);

		errno = 0;
		wcsftime(buf, bufLen, format, time);
		if (errno == EINVAL)
		{
			LogErrorF(this, L"Time: \"Format=%s\" invalid", format);
			buf[0] = 0;
		}

		_set_invalid_parameter_handler(oldHandler);
	}
}

void MeasureTime::FillCurrentTime()
{
	auto getTZSysTime = [&](bool getDaylight) -> DWORD
	{
		SYSTEMTIME st;
		if (m_TimeStamp == DBL_MIN)
		{
			GetSystemTime(&st);
			m_TimeStamp = (int)st.wYear;
		}

		TIME_ZONE_INFORMATION tzi;
		DWORD ret = GetTimeZoneInformation(&tzi);
		st = getDaylight ? tzi.DaylightDate : tzi.StandardDate;
		st.wYear = (int)m_TimeStamp;

		if (ret == 0 || !GetTimeZoneChangeDate(&st))
		{
			m_Time.QuadPart = 0;
			return ret;
		}

		FILETIME ft;
		SystemTimeToFileTime(&st, &ft);
		m_Time.HighPart = ft.dwHighDateTime;
		m_Time.LowPart = ft.dwLowDateTime;
		return ret;
	};

	auto getCurrentTime = [&]() -> void
	{
		FILETIME ftUTCTime;
		GetSystemTimeAsFileTime(&ftUTCTime);

		// Modify the ltime to match the current timezone
		// This way we can use the value also for the clock
		m_Time.HighPart = ftUTCTime.dwHighDateTime;
		m_Time.LowPart = ftUTCTime.dwLowDateTime;

		m_Time.QuadPart += m_Delta.QuadPart;
	};

	if (m_TimeStampType == DST_END)
	{
		getTZSysTime(false);
	}
	else if (m_TimeStampType == DST_START)
	{
		getTZSysTime(true);
	}
	else if (m_TimeStampType == DST_NEXT_END)
	{
		if (getTZSysTime(false) != 0)
		{
			LARGE_INTEGER dstStandard = m_Time;
			getCurrentTime();
			if (m_Time.QuadPart < dstStandard.QuadPart)
			{
				m_Time = dstStandard;
			}
			else
			{
				// Get next year's DST end date/time
				++m_TimeStamp;
				getTZSysTime(false);
			}
		}
	}
	else if (m_TimeStampType == DST_NEXT_START)
	{
		if (getTZSysTime(true) != 0)
		{
			LARGE_INTEGER dstDaylight = m_Time;
			getCurrentTime();
			if (m_Time.QuadPart < dstDaylight.QuadPart)
			{
				m_Time = dstDaylight;
			}
			else
			{
				// Get next year's DST start date/time
				++m_TimeStamp;
				getTZSysTime(true);
			}
		}
	}
	else if (m_TimeStamp < 0.0) // m_TimeStampType == INVALID
	{
		getCurrentTime();
	}
	else // m_TimeStampType == FIXED
	{
		m_Time.QuadPart = (LONGLONG)(m_TimeStamp * 10000000);
	}
}

/*
** Updates the current time
**
*/
void MeasureTime::UpdateValue()
{
	FillCurrentTime();

	if (!m_Format.empty())
	{
		// If there is some date format, parse the value from it instead
		WCHAR* tmpSz = new WCHAR[MAX_LINE_LENGTH];
		SYSTEMTIME sysToday;
		FILETIME ftToday;

		tmpSz[0] = 0;

		ftToday.dwHighDateTime = m_Time.HighPart;
		ftToday.dwLowDateTime = m_Time.LowPart;

		FileTimeToSystemTime(&ftToday, &sysToday);

		const WCHAR* format = m_Format.c_str();
		if (_wcsicmp(L"locale-time", format) == 0)
		{
			GetTimeFormat(LOCALE_USER_DEFAULT, 0, &sysToday, nullptr, tmpSz, MAX_LINE_LENGTH);
		}
		else if (_wcsicmp(L"locale-date", format) == 0)
		{
			GetDateFormat(LOCALE_USER_DEFAULT, 0, &sysToday, nullptr, tmpSz, MAX_LINE_LENGTH);
		}
		else
		{
			struct tm today;
			today.tm_isdst = 0;
			today.tm_hour = sysToday.wHour;
			today.tm_mday = sysToday.wDay;
			today.tm_min = sysToday.wMinute;
			today.tm_mon = sysToday.wMonth - 1;
			today.tm_sec = sysToday.wSecond;
			today.tm_wday = sysToday.wDayOfWeek;
			today.tm_yday = GetYearDay(sysToday.wYear, sysToday.wMonth, sysToday.wDay);
			today.tm_year = sysToday.wYear - 1900;

			TimeToString(tmpSz, MAX_LINE_LENGTH, format, &today);
		}

		m_Value = wcstod(tmpSz, nullptr);

		delete [] tmpSz;
	}
	else
	{
		m_Value = (double)(m_Time.QuadPart / 10000000);
	}
}


/*
** Returns the time as string.
**
*/
const WCHAR* MeasureTime::GetStringValue()
{
	static WCHAR tmpSz[MAX_LINE_LENGTH];
	struct tm today;

	tmpSz[0] = 0;

	SYSTEMTIME sysToday;
	FILETIME ftToday;
	ftToday.dwHighDateTime = m_Time.HighPart;
	ftToday.dwLowDateTime = m_Time.LowPart;

	FileTimeToSystemTime(&ftToday, &sysToday);

	today.tm_isdst = 0;
	today.tm_hour = sysToday.wHour;
	today.tm_mday = sysToday.wDay;
	today.tm_min = sysToday.wMinute;
	today.tm_mon = sysToday.wMonth - 1;
	today.tm_sec = sysToday.wSecond;
	today.tm_wday = sysToday.wDayOfWeek;
	today.tm_yday = GetYearDay(sysToday.wYear, sysToday.wMonth, sysToday.wDay);
	today.tm_year = sysToday.wYear - 1900;

	// Create the string
	if (!m_Format.empty())
	{
		const WCHAR* format = m_Format.c_str();
		if (_wcsicmp(L"locale-time", format) == 0)
		{
			GetTimeFormat(LOCALE_USER_DEFAULT, 0, &sysToday, nullptr, tmpSz, MAX_LINE_LENGTH);
		}
		else if (_wcsicmp(L"locale-date", format) == 0)
		{
			GetDateFormat(LOCALE_USER_DEFAULT, 0, &sysToday, nullptr, tmpSz, MAX_LINE_LENGTH);
		}
		else
		{
			TimeToString(tmpSz, MAX_LINE_LENGTH, format, &today);
		}
	}
	else
	{
		TimeToString(tmpSz, MAX_LINE_LENGTH, L"%H:%M:%S", &today);
	}

	return CheckSubstitute(tmpSz);
}

/*
** Read the options specified in the ini file.
**
*/
void MeasureTime::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	auto ParseYear = [&](std::wstring year, size_t pos) -> void
	{
		year = year.substr(pos);
		m_TimeStamp = parser.ParseDouble(year.c_str(), DBL_MIN);
	};

	Measure::ReadOptions(parser, section);

	m_Format = parser.ReadString(section, L"Format", L"");

	const WCHAR* timeStamp = parser.ReadString(section, L"TimeStamp", L"-1").c_str();
	if (wcsncmp(timeStamp, L"DSTStart", 8) == 0)
	{
		m_TimeStampType = DST_START;
		ParseYear(timeStamp, 8);
	}
	else if (wcsncmp(timeStamp, L"DSTEnd", 6) == 0)
	{
		m_TimeStampType = DST_END;
		ParseYear(timeStamp, 6);
	}
	else if (_wcsicmp(timeStamp, L"DSTNextStart") == 0)
	{
		m_TimeStampType = DST_NEXT_START;
		m_TimeStamp = DBL_MIN;
	}
	else if (_wcsicmp(timeStamp, L"DSTNextEnd") == 0)
	{
		m_TimeStampType = DST_NEXT_END;
		m_TimeStamp = DBL_MIN;
	}
	else
	{
		m_TimeStamp = parser.ParseDouble(timeStamp, -1.0);
		if (m_TimeStamp < 0.0)
		{
			m_TimeStampType = INVALID;

			const WCHAR* timezone = parser.ReadString(section, L"TimeZone", L"local").c_str();
			if (_wcsicmp(L"local", timezone) == 0)
			{
				m_TimeZone = LOCAL_TIMEZONE;
			}
			else
			{
				m_TimeZone = parser.ParseDouble(timezone, 0.0);
				m_DaylightSavingTime = parser.ReadBool(section, L"DaylightSavingTime", true);
			}

			UpdateDelta();
		}
		else
		{
			m_TimeStampType = FIXED;
		}
	}

	if (!m_Initialized)
	{
		// Initialize m_Time to avoid causing EINVAL in TimeToString() until calling UpdateValue()
		FillCurrentTime();
	}
}

void MeasureTime::UpdateDelta()
{
	if (m_TimeZone == LOCAL_TIMEZONE)
	{
		SYSTEMTIME sysLocalTime, sysUTCTime;
		GetLocalTime(&sysLocalTime);
		GetSystemTime(&sysUTCTime);

		FILETIME ftLocalTime, ftUTCTime;
		SystemTimeToFileTime(&sysLocalTime, &ftLocalTime);
		SystemTimeToFileTime(&sysUTCTime, &ftUTCTime);

		LARGE_INTEGER largeInt1, largeInt2;
		largeInt1.HighPart = ftLocalTime.dwHighDateTime;
		largeInt1.LowPart = ftLocalTime.dwLowDateTime;
		largeInt2.HighPart = ftUTCTime.dwHighDateTime;
		largeInt2.LowPart = ftUTCTime.dwLowDateTime;

		m_Delta.QuadPart = largeInt1.QuadPart - largeInt2.QuadPart;
	}
	else
	{
		time_t now;
		time(&now);
		tm* today = localtime(&now);
		if (m_DaylightSavingTime && today->tm_isdst)
		{
			// Add DST
			TIME_ZONE_INFORMATION tzi;
			GetTimeZoneInformation(&tzi);

			m_Delta.QuadPart = (LONGLONG)((m_TimeZone * 3600) - tzi.DaylightBias * 60) * 10000000;
		}
		else
		{
			m_Delta.QuadPart = (LONGLONG)(m_TimeZone * 3600) * 10000000;
		}
	}
}