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

/*
** The constructor
**
*/
MeasureTime::MeasureTime(MeterWindow* meterWindow, const WCHAR* name) : Measure(meterWindow, name),
	m_DeltaTime(),
	m_Time(),
	m_TimeStamp(-1)
{
	/* Set time zone from TZ environment variable. If TZ is not set,
	 * the operating system is queried to obtain the default value
	 * for the variable.
	 */
	_tzset();
}

/*
** The destructor
**
*/
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
	if (m_TimeStamp < 0.0)
	{
		FILETIME ftUTCTime;
		GetSystemTimeAsFileTime(&ftUTCTime);

		// Modify the ltime to match the current timezone
		// This way we can use the value also for the clock
		m_Time.HighPart = ftUTCTime.dwHighDateTime;
		m_Time.LowPart = ftUTCTime.dwLowDateTime;

		m_Time.QuadPart += m_DeltaTime.QuadPart;
	}
	else
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
	Measure::ReadOptions(parser, section);

	m_Format = parser.ReadString(section, L"Format", L"");

	m_TimeStamp = parser.ReadFloat(section, L"TimeStamp", -1);

	if (m_TimeStamp < 0.0)
	{
		const WCHAR* timezone = parser.ReadString(section, L"TimeZone", L"local").c_str();
		if (_wcsicmp(L"local", timezone) == 0)
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

			m_DeltaTime.QuadPart = largeInt1.QuadPart - largeInt2.QuadPart;
		}
		else
		{
			double zone = parser.ParseDouble(timezone, 0.0);
			bool dst = parser.ReadBool(section, L"DaylightSavingTime", true);

			struct tm* today;
			time_t now;
			time(&now);
			today = localtime(&now);

			if (dst && today->tm_isdst)
			{
				// Add DST
				TIME_ZONE_INFORMATION tzi;
				GetTimeZoneInformation(&tzi);

				m_DeltaTime.QuadPart = (LONGLONG)((zone * 3600) - tzi.DaylightBias * 60) * 10000000;
			}
			else
			{
				m_DeltaTime.QuadPart = (LONGLONG)(zone * 3600) * 10000000;
			}
		}
	}
	else
	{
		m_DeltaTime.QuadPart = 0;
	}

	if (!m_Initialized)
	{
		// Initialize m_Time to avoid causing EINVAL in TimeToString() until calling UpdateValue()
		FillCurrentTime();
	}
}
