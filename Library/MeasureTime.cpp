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

#pragma warning(disable: 4996)

#include "StdAfx.h"
#include "MeasureTime.h"
#include "Rainmeter.h"
#include <time.h>

int GetYearDay(int year, int month, int day)
{
	int yearDay = 0;
	UINT dates[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	for (int i = 0; i < month - 1; i++) 
	{
		yearDay += dates[i];
	}

	if (month > 2 && ((((year % 4) == 0) && ((year % 100) != 0)) || (year  % 400) == 0))
	{
		yearDay++;
	}

	yearDay += day;

	return yearDay - 1;
}

/*
** CMeasureTime
**
** The constructor
**
*/
CMeasureTime::CMeasureTime(CMeterWindow* meterWindow) : CMeasure(meterWindow)
{
	/* Set time zone from TZ environment variable. If TZ is not set,
     * the operating system is queried to obtain the default value 
     * for the variable. 
     */
    _tzset();

	m_DeltaTime.QuadPart = 0;
	m_Time.QuadPart = 0;
}

/*
** ~CMeasureTime
**
** The destructor
**
*/
CMeasureTime::~CMeasureTime()
{
}

/*
** Update
**
** Updates the current time
**
*/
bool CMeasureTime::Update()
{
	if (!CMeasure::PreUpdate()) return false;

	FILETIME ftUTCTime;
	GetSystemTimeAsFileTime(&ftUTCTime);

	// Modify the ltime to match the current timezone
	// This way we can use the value also for the clock
	m_Time.HighPart = ftUTCTime.dwHighDateTime;
	m_Time.LowPart = ftUTCTime.dwLowDateTime;

	m_Time.QuadPart += m_DeltaTime.QuadPart;
	m_Value = (double)(m_Time.QuadPart / 10000000);

	if (m_Format.size() != 0)
	{
		// If there is some date format, parse the value from it instead
		WCHAR tmpSz[MAX_LINE_LENGTH];
		SYSTEMTIME sysToday;
		FILETIME ftToday;

		ftToday.dwHighDateTime = m_Time.HighPart;
		ftToday.dwLowDateTime = m_Time.LowPart;

		FileTimeToSystemTime(&ftToday, &sysToday);

		if (wcsicmp(L"locale-time", m_Format.c_str()) == 0)
		{
			GetTimeFormat(LOCALE_USER_DEFAULT, 0, &sysToday, NULL, tmpSz, MAX_LINE_LENGTH);
		}
		else if (wcsicmp(L"locale-date", m_Format.c_str()) == 0)
		{
			GetDateFormat(LOCALE_USER_DEFAULT, 0, &sysToday, NULL, tmpSz, MAX_LINE_LENGTH);
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

			wcsftime(tmpSz, MAX_LINE_LENGTH, m_Format.c_str(), &today);
		}

		m_Value = wcstod(tmpSz, NULL);
	}

	return PostUpdate();
}


/*
** GetStringValue
**
** Returns the time as string.
**
*/
const WCHAR* CMeasureTime::GetStringValue(bool autoScale, double scale, int decimals, bool percentual)
{
	static WCHAR tmpSz[MAX_LINE_LENGTH];
	struct tm today;
	
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
	if (m_Format.size() > 0)
	{
		if (wcsicmp(L"locale-time", m_Format.c_str()) == 0)
		{
			GetTimeFormat(LOCALE_USER_DEFAULT, 0, &sysToday, NULL, tmpSz, MAX_LINE_LENGTH);
		}
		else if (wcsicmp(L"locale-date", m_Format.c_str()) == 0)
		{
			GetDateFormat(LOCALE_USER_DEFAULT, 0, &sysToday, NULL, tmpSz, MAX_LINE_LENGTH);
		}
		else
		{
			wcsftime(tmpSz, MAX_LINE_LENGTH, m_Format.c_str(), &today);
		}
	}
	else
	{
		wcsftime(tmpSz, MAX_LINE_LENGTH, L"%H:%M:%S", &today);
	}

	return CheckSubstitute(tmpSz);
}

/*
** ReadConfig
**
** Reads the measure specific configs.
**
*/
void CMeasureTime::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	CMeasure::ReadConfig(parser, section);

	m_Format = parser.ReadString(section, L"Format", L"");

	std::wstring timezone;
	timezone = parser.ReadString(section, L"TimeZone", L"local");
	bool dst = 1 == parser.ReadInt(section, L"DaylightSavingTime", 1);

	SYSTEMTIME sysLocalTime, sysUTCTime;
	GetLocalTime(&sysLocalTime);
	GetSystemTime(&sysUTCTime);

	if (wcsicmp(L"local", timezone.c_str()) == 0)
	{
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
		double zone = wcstod(timezone.c_str(), NULL);

		struct tm* today;
		time_t now;
		time(&now);
		today = localtime(&now);

		if (dst && today->tm_isdst)
		{
			// Add DST
			TIME_ZONE_INFORMATION tzi;
			GetTimeZoneInformation(&tzi);

			m_DeltaTime.QuadPart = (LONGLONG)((zone * 3600) - tzi.DaylightBias * 60) * 10000000.0;
		}
		else
		{
			m_DeltaTime.QuadPart = (LONGLONG)(zone * 3600) * 10000000.0;
		}
	}
}
