/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureTime.h"
#include "Rainmeter.h"
#include <locale>
#include <sstream>
#include <iomanip>
#include <ctime>

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
	m_FormatLocale(nullptr),
	m_Delta(),
	m_Time(),
	m_TimeStamp(-1.0),
	m_TimeStampType(INVALID),
	m_TimeZone(LOCAL_TIMEZONE),
	m_DaylightSavingTime(true)
{
}

MeasureTime::~MeasureTime()
{
	FreeLocale();
}

void MeasureTime::FreeLocale()
{
	if (m_FormatLocale)
	{
		_free_locale(m_FormatLocale);
		m_FormatLocale = nullptr;
	}
}

/*
** Converts given time to string.
** This function is a wrapper function for _wcsftime_l.
**
*/
void MeasureTime::TimeToString(WCHAR* buf, size_t bufLen, const WCHAR* format, const struct tm* time)
{
	if (bufLen > 0)
	{
		_invalid_parameter_handler oldHandler = _set_invalid_parameter_handler(RmNullCRTInvalidParameterHandler);
		_CrtSetReportMode(_CRT_ASSERT, 0);

		errno = 0;
		if (m_FormatLocale)
		{
			_wcsftime_l(buf, bufLen, format, time, m_FormatLocale);
		}
		else
		{
			wcsftime(buf, bufLen, format, time);
		}
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
		SYSTEMTIME st = { 0 };
		if (m_TimeStamp == DBL_MIN)
		{
			GetSystemTime(&st);
			m_TimeStamp = (int)st.wYear;
		}

		TIME_ZONE_INFORMATION tzi = { 0 };
		DWORD ret = GetTimeZoneInformation(&tzi);
		st = getDaylight ? tzi.DaylightDate : tzi.StandardDate;
		st.wYear = (int)m_TimeStamp;

		if (ret == 0 || !GetTimeZoneChangeDate(&st))
		{
			m_Time.QuadPart = 0;
			return ret;
		}

		FILETIME ft = { 0 };
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
		SYSTEMTIME sysToday = { 0 };
		FILETIME ftToday = { 0 };

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
			struct tm today = { 0 };
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
		tmpSz = nullptr;
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
	struct tm today = { 0 };

	tmpSz[0] = 0;

	SYSTEMTIME sysToday = { 0 };
	FILETIME ftToday = { 0 };
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

	std::wstring timeStamp = parser.ReadString(section, L"TimeStamp", L"-1");
	if (wcsncmp(timeStamp.c_str(), L"DSTStart", 8) == 0)
	{
		m_TimeStampType = DST_START;
		ParseYear(timeStamp, 8);
	}
	else if (wcsncmp(timeStamp.c_str(), L"DSTEnd", 6) == 0)
	{
		m_TimeStampType = DST_END;
		ParseYear(timeStamp, 6);
	}
	else if (_wcsicmp(timeStamp.c_str(), L"DSTNextStart") == 0)
	{
		m_TimeStampType = DST_NEXT_START;
		m_TimeStamp = DBL_MIN;
	}
	else if (_wcsicmp(timeStamp.c_str(), L"DSTNextEnd") == 0)
	{
		m_TimeStampType = DST_NEXT_END;
		m_TimeStamp = DBL_MIN;
	}
	else
	{
		std::wstring tsformat = parser.ReadString(section, L"TimeStampFormat", L"");
		if (tsformat.empty())
		{
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

			m_TimeStamp = parser.ParseDouble(timeStamp.c_str(), -1.0);
			if (m_TimeStamp < 0.0)
			{
				// |TimeStamp| is invalid, measure returns the current time
				m_TimeStampType = INVALID;

				UpdateDelta();
			}
			else
			{
				// |TimeStamp| is a Windows timestamp
				m_TimeStampType = FIXED;

				const bool isUtc = parser.ReadBool(section, L"IsUTC", false);
				const bool isUnix = parser.ReadBool(section, L"IsUnix", false);
				if (isUtc && m_TimeZone == LOCAL_TIMEZONE) {
					if (!isUnix) {
						// timestamp is a Windows Timestamp -> convert to Unix first for timezone adjustment
						m_TimeStamp -= 11644473600;
					}

					const std::time_t utctimestamp = static_cast<std::time_t>(m_TimeStamp);
					std::tm* localtime = std::localtime(&utctimestamp);
					const auto localtimestr = std::string(std::asctime(localtime));
					// convert the localtime to a timestamp without any timezone adjustment,
					// e.g. localtimestamp will be seconds since epoch with the local timezone applied already
					const std::time_t localtimestamp = _mkgmtime(localtime);

					// convert to windows timestamp
					m_TimeStamp = static_cast<double>(localtimestamp + 11644473600);
				}
			}
		}
		else
		{
			// The |TimeStamp| is formatted, parse it and convert to a Windows timestamp
			m_TimeStampType = FIXED;

			std::wstring localeStr = parser.ReadString(section, L"TimeStampLocale", L"C");

			// Because exceptions are disabled, constructing std::locale with an invalid locale will
			// call abort(). To fail gracefully, we need to check if the locale exists first.
			std::locale locale;
			if (_locale_t cLocale = _wcreate_locale(LC_TIME, localeStr.c_str()))
			{
				locale = std::locale(StringUtil::Narrow(localeStr), std::locale::time);
				_free_locale(cLocale);
			}
			else
			{
				LogErrorF(this, L"Invalid TimeStampLocale: %s", localeStr.c_str());
				locale = std::locale("C");
			}

			std::tm time = { 0 };
			time.tm_mday = 1;	// "Day of month" cannot be 0.

			std::basic_istringstream<wchar_t> is(timeStamp);
			is.imbue(locale);
			is >> std::get_time(&time, tsformat.c_str());

			if (is.fail())
			{
				LogErrorF(this, L"Invalid TimeStampFormat: %s", tsformat.c_str());
				m_TimeStamp = 0;
			}
			else
			{
				// Convert std::tm -> SYSTEMTIME -> FILETIME -> LARGE_INTEGER
				SYSTEMTIME st = { 0 };
				st.wDay = time.tm_mday;
				st.wDayOfWeek = time.tm_wday;
				st.wHour = time.tm_hour;
				st.wMilliseconds = 0;
				st.wMinute = time.tm_min;
				st.wMonth = time.tm_mon + 1;
				st.wSecond = time.tm_sec;
				st.wYear = time.tm_year + 1900;

				// Fix known overflow bug when using %p.
				// TODO: VS2015 has *apparently* fixed this (and other bugs with this).
				// https://connect.microsoft.com/VisualStudio/feedback/details/808162
				st.wHour %= 24;

				FILETIME ft;
				if (!SystemTimeToFileTime(&st, &ft))
				{
					LogErrorF(this, L"Parsing error: %s", tsformat.c_str());
					m_TimeStamp = 0;
				}
				else
				{
					LARGE_INTEGER li = { 0 };
					li.HighPart = ft.dwHighDateTime;
					li.LowPart = ft.dwLowDateTime;

					m_TimeStamp = (double)(li.QuadPart / 10000000);
				}
			}
		}
	}

	// Format locale
	FreeLocale();
	const WCHAR* formatLocale = parser.ReadString(section, L"FormatLocale", L"").c_str();
	if (*formatLocale)
	{
		if (_wcsicmp(formatLocale, L"local") == 0)
		{
			// An empty string represents the user's locale, instead of the default "C" locale.
			m_FormatLocale = _wcreate_locale(LC_TIME, L"");
		}
		else
		{
			m_FormatLocale = _wcreate_locale(LC_TIME, formatLocale);
		}

		if (!m_FormatLocale)
		{
			LogErrorF(this, L"Invalid FormatLocale: %s", formatLocale);
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
		SYSTEMTIME sysLocalTime = { 0 }, sysUTCTime = { 0 };
		GetLocalTime(&sysLocalTime);
		GetSystemTime(&sysUTCTime);

		FILETIME ftLocalTime = { 0 }, ftUTCTime = { 0 };
		SystemTimeToFileTime(&sysLocalTime, &ftLocalTime);
		SystemTimeToFileTime(&sysUTCTime, &ftUTCTime);

		LARGE_INTEGER largeInt1 = { 0 }, largeInt2 = { 0 };
		largeInt1.HighPart = ftLocalTime.dwHighDateTime;
		largeInt1.LowPart = ftLocalTime.dwLowDateTime;
		largeInt2.HighPart = ftUTCTime.dwHighDateTime;
		largeInt2.LowPart = ftUTCTime.dwLowDateTime;

		m_Delta.QuadPart = largeInt1.QuadPart - largeInt2.QuadPart;
	}
	else
	{
		time_t now = 0LL;
		time(&now);
		tm* today = localtime(&now);
		if (m_DaylightSavingTime && today->tm_isdst)
		{
			// Add DST
			TIME_ZONE_INFORMATION tzi = { 0 };
			GetTimeZoneInformation(&tzi);

			m_Delta.QuadPart = (LONGLONG)((m_TimeZone * 3600) - tzi.DaylightBias * 60) * 10000000;
		}
		else
		{
			m_Delta.QuadPart = (LONGLONG)(m_TimeZone * 3600) * 10000000;
		}
	}
}
