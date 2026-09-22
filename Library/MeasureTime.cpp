// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasureTime.h"
#include "Rainmeter.h"
#include "../Common/DateTimeParser.h"
#include "../Common/StringUtil.h"

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

size_t MeasureTime::TimeToString(WCHAR* buf, size_t bufLen, const WCHAR* format, const struct tm* time)
{
	size_t length = 0;
	if (bufLen > 0)
	{
		_invalid_parameter_handler oldHandler = _set_thread_local_invalid_parameter_handler(RmNullCRTInvalidParameterHandler);
		_CrtSetReportMode(_CRT_ASSERT, 0);

		errno = 0;
		if (m_FormatLocale)
		{
			length = _wcsftime_l(buf, bufLen, format, time, m_FormatLocale);
		}
		else
		{
			length = wcsftime(buf, bufLen, format, time);
		}
		if (errno == EINVAL)
		{
			LogErrorF(this, L"Time: \"Format=%s\" invalid", format);
			buf[0] = 0;
			length = 0;
		}

		_set_thread_local_invalid_parameter_handler(oldHandler);
	}
	return length;
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


std::optional<std::wstring_view> MeasureTime::GetStringValue()
{
	static WCHAR tmpSz[MAX_LINE_LENGTH];
	struct tm today = { 0 };
	size_t length;

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
			const int result = GetTimeFormat(LOCALE_USER_DEFAULT, 0, &sysToday, nullptr, tmpSz, MAX_LINE_LENGTH);
			length = result > 0 ? result - 1 : 0;
		}
		else if (_wcsicmp(L"locale-date", format) == 0)
		{
			const int result = GetDateFormat(LOCALE_USER_DEFAULT, 0, &sysToday, nullptr, tmpSz, MAX_LINE_LENGTH);
			length = result > 0 ? result - 1 : 0;
		}
		else
		{
			length = TimeToString(tmpSz, MAX_LINE_LENGTH, format, &today);
		}
	}
	else
	{
		length = TimeToString(tmpSz, MAX_LINE_LENGTH, L"%H:%M:%S", &today);
	}

	return CheckSubstitute(std::wstring_view(tmpSz, length));
}

void MeasureTime::ReadOptions(ConfigParser::OptionReader& reader)
{
	auto& parser = m_Skin->GetParser();
	auto ParseYear = [&](std::wstring year, size_t pos) -> void
	{
		year = year.substr(pos);
		m_TimeStamp = parser.ParseDouble(year.c_str(), DBL_MIN);
	};

	Measure::ReadOptions(reader);

	reader.ReadString<"Format">(m_Format, L"");

	std::wstring timeStamp = reader.ReadString<"TimeStamp">(L"-1");
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
		std::wstring tsformat = reader.ReadString<"TimeStampFormat">(L"");
		if (tsformat.empty())
		{
			m_TimeStamp = parser.ParseDouble(timeStamp.c_str(), -1.0);
			if (m_TimeStamp < 0.0)
			{
				// |TimeStamp| is invalid, measure returns the current time
				m_TimeStampType = INVALID;

				const WCHAR* timezone = reader.ReadString<"TimeZone">(L"local").c_str();
				if (_wcsicmp(L"local", timezone) == 0)
				{
					m_TimeZone = LOCAL_TIMEZONE;
				}
				else
				{
					m_TimeZone = parser.ParseDouble(timezone, 0.0);
					m_DaylightSavingTime = reader.ReadBool<"DaylightSavingTime">(true);
				}

				UpdateDelta();
			}
			else
			{
				// |TimeStamp| is a Windows timestamp
				m_TimeStampType = FIXED;
			}
		}
		else
		{
			// The |TimeStamp| is formatted, parse it and convert to a Windows timestamp
			m_TimeStampType = FIXED;

			bool useSystemLocale = false;
			if (StringUtil::EqualsIgnoreCase(tsformat, L"locale-date"))
			{
				tsformat = L"%x";
				useSystemLocale = true;
			}
			else if (StringUtil::EqualsIgnoreCase(tsformat, L"locale-time"))
			{
				tsformat = L"%X";
				useSystemLocale = true;
			}

			const auto& configuredLocale = reader.ReadString<"TimeStampLocale">(L"C");
			const std::wstring_view locale = useSystemLocale ? L"local" : std::wstring_view(configuredLocale);
			const auto result = DateTimeParser::Parse(timeStamp, tsformat, locale);
			if (!result)
			{
				switch (result.error)
				{
				case DateTimeParser::ParseError::InvalidLocale:
					LogErrorF(this, L"Invalid TimeStampLocale: %s", configuredLocale.c_str());
					break;
				case DateTimeParser::ParseError::InvalidTimeZone:
					LogErrorF(this, L"Invalid time zone in TimeStamp: %s", timeStamp.c_str());
					break;
				case DateTimeParser::ParseError::InvalidDate:
					LogErrorF(this, L"Parsing error: %s", tsformat.c_str());
					break;
				default:
					LogErrorF(this, L"Invalid TimeStampFormat: %s", tsformat.c_str());
					break;
				}

				m_TimeStamp = 0;
			}
			else
			{
				LARGE_INTEGER timestamp = {};
				timestamp.HighPart = result.timestamp.dwHighDateTime;
				timestamp.LowPart = result.timestamp.dwLowDateTime;
				m_TimeStamp = (double)(timestamp.QuadPart / 10000000);
			}
		}
	}

	// Format locale
	FreeLocale();
	const WCHAR* formatLocale = reader.ReadString<"FormatLocale">(L"").c_str();
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
		time_t now = 0;
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
