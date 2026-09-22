// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "DateTimeParser.h"
#include "UnitTest.h"

#include <ctime>
#include <string>

namespace {

SYSTEMTIME Parse(std::wstring_view value, std::wstring_view format, std::wstring_view locale = L"C", DateTimeParser::MatchMode mode = DateTimeParser::MatchMode::Full)
{
	const auto result = DateTimeParser::Parse(value, format, locale, mode);
	Assert::IsTrue((bool)result);
	SYSTEMTIME time = {};
	Assert::IsTrue(FileTimeToSystemTime(&result.timestamp, &time) != FALSE);
	return time;
}

void AssertDate(const SYSTEMTIME& time, int year, int month, int day)
{
	Assert::AreEqual(year, (int)time.wYear);
	Assert::AreEqual(month, (int)time.wMonth);
	Assert::AreEqual(day, (int)time.wDay);
}

void AssertTime(const SYSTEMTIME& time, int hour, int minute, int second)
{
	Assert::AreEqual(hour, (int)time.wHour);
	Assert::AreEqual(minute, (int)time.wMinute);
	Assert::AreEqual(second, (int)time.wSecond);
}

void AssertError(std::wstring_view value, std::wstring_view format, DateTimeParser::ParseError error)
{
	const auto result = DateTimeParser::Parse(value, format, L"C", DateTimeParser::MatchMode::Full);
	Assert::AreEqual((int)error, (int)result.error);
}

std::wstring FormatTime(const WCHAR* format, const tm& value, const WCHAR* localeName = L"C")
{
	_locale_t locale = _wcreate_locale(LC_TIME, localeName);
	Assert::IsTrue(locale != nullptr);
	WCHAR buffer[256] = {};
	const size_t length = _wcsftime_l(buffer, _countof(buffer), format, &value, locale);
	_free_locale(locale);
	Assert::IsTrue(length > 0);
	return std::wstring(buffer, length);
}

}  // namespace

TEST_CLASS(Common_DateTimeParser_Test)
{
public:
	TEST_METHOD(TestCalendarDate)
	{
		AssertDate(Parse(L"2015-12-26", L"%Y-%m-%d"), 2015, 12, 26);
	}

	TEST_METHOD(TestDateAliases)
	{
		AssertDate(Parse(L"12/26/15", L"%D"), 2015, 12, 26);
		AssertDate(Parse(L"2015-12-26", L"%F"), 2015, 12, 26);
	}

	TEST_METHOD(TestTimeAliases)
	{
		AssertTime(Parse(L"22:55", L"%R"), 22, 55, 0);
		AssertTime(Parse(L"22:55:03", L"%T"), 22, 55, 3);
	}

	TEST_METHOD(TestCenturyBeforeYear)
	{
		AssertDate(Parse(L"20 15-12-26", L"%C %y-%m-%d"), 2015, 12, 26);
	}

	TEST_METHOD(TestCenturyAfterYear)
	{
		AssertDate(Parse(L"15 20-12-26", L"%y %C-%m-%d"), 2015, 12, 26);
	}

	TEST_METHOD(TestTwoDigitYearPivot)
	{
		AssertDate(Parse(L"68-01-01", L"%y-%m-%d"), 2068, 1, 1);
		AssertDate(Parse(L"69-01-01", L"%y-%m-%d"), 1969, 1, 1);
	}

	TEST_METHOD(TestOrdinalDate)
	{
		AssertDate(Parse(L"2024 060", L"%Y %j"), 2024, 2, 29);
		AssertDate(Parse(L"2023 365", L"%Y %j"), 2023, 12, 31);
	}

	TEST_METHOD(TestOrdinalDateRejectsNonLeapDay)
	{
		AssertError(L"2023 366", L"%Y %j", DateTimeParser::ParseError::InvalidDate);
	}

	TEST_METHOD(TestIsoWeekDate)
	{
		AssertDate(Parse(L"2015-W53-7", L"%G-W%V-%u"), 2016, 1, 3);
		AssertDate(Parse(L"2016-W01-1", L"%G-W%V-%u"), 2016, 1, 4);
	}

	TEST_METHOD(TestTwoDigitIsoYear)
	{
		AssertDate(Parse(L"15-W53-7", L"%g-W%V-%u"), 2016, 1, 3);
	}

	TEST_METHOD(TestIsoYearDefaultsToWeekOneMonday)
	{
		AssertDate(Parse(L"2016", L"%G"), 2016, 1, 4);
	}

	TEST_METHOD(TestIsoWeekDefaultsToMonday)
	{
		AssertDate(Parse(L"2016-W01", L"%G-W%V"), 2016, 1, 4);
	}

	TEST_METHOD(TestIsoWeekRejectsInvalidWeek53)
	{
		AssertError(L"2014-W53-1", L"%G-W%V-%u", DateTimeParser::ParseError::InvalidDate);
	}

	TEST_METHOD(TestSundayWeek)
	{
		AssertDate(Parse(L"2023 01 0", L"%Y %U %w"), 2023, 1, 1);
	}

	TEST_METHOD(TestMondayWeek)
	{
		AssertDate(Parse(L"2023 01 1", L"%Y %W %w"), 2023, 1, 2);
	}

	TEST_METHOD(TestWeekZero)
	{
		AssertDate(Parse(L"2022 00 6", L"%Y %U %w"), 2022, 1, 1);
		AssertError(L"2022 00 0", L"%Y %U %w", DateTimeParser::ParseError::InvalidDate);
	}

	TEST_METHOD(TestConflictingCalendarAndOrdinalDate)
	{
		AssertError(L"2024-02-29 061", L"%Y-%m-%d %j", DateTimeParser::ParseError::InvalidDate);
	}

	TEST_METHOD(TestMatchingCalendarAndOrdinalDate)
	{
		AssertDate(Parse(L"2024-02-29 060", L"%Y-%m-%d %j"), 2024, 2, 29);
	}

	TEST_METHOD(TestWeekdayNames)
	{
		AssertDate(Parse(L"Saturday 2015-12-26", L"%A %Y-%m-%d"), 2015, 12, 26);
		AssertDate(Parse(L"sat 2015-12-26", L"%a %Y-%m-%d"), 2015, 12, 26);
	}

	TEST_METHOD(TestConflictingWeekday)
	{
		AssertError(L"Monday 2015-12-26", L"%A %Y-%m-%d", DateTimeParser::ParseError::InvalidDate);
	}

	TEST_METHOD(TestMonthNames)
	{
		AssertDate(Parse(L"December 26 2015", L"%B %d %Y"), 2015, 12, 26);
		AssertDate(Parse(L"dec 26 2015", L"%h %d %Y"), 2015, 12, 26);
	}

	TEST_METHOD(TestGermanNames)
	{
		AssertDate(Parse(L"Samstag, 26. Dezember 2015", L"%A, %d. %B %Y", L"de-DE"), 2015, 12, 26);
	}

	TEST_METHOD(TestLocaleCompositeRoundTrips)
	{
		tm source = {};
		source.tm_year = 124;
		source.tm_mon = 1;
		source.tm_mday = 9;
		source.tm_hour = 13;
		source.tm_min = 4;
		source.tm_sec = 5;
		source.tm_wday = 5;

		AssertDate(Parse(FormatTime(L"%x", source), L"%x"), 2024, 2, 9);
		AssertDate(Parse(FormatTime(L"%#x", source), L"%#x"), 2024, 2, 9);
		AssertTime(Parse(FormatTime(L"%X", source), L"%X"), 13, 4, 5);
		const SYSTEMTIME combined = Parse(FormatTime(L"%c", source), L"%c");
		AssertDate(combined, 2024, 2, 9);
		AssertTime(combined, 13, 4, 5);
		const SYSTEMTIME longCombined = Parse(FormatTime(L"%#c", source), L"%#c");
		AssertDate(longCombined, 2024, 2, 9);
		AssertTime(longCombined, 13, 4, 5);
		AssertTime(Parse(FormatTime(L"%r", source), L"%r"), 13, 4, 5);
	}

	TEST_METHOD(TestLocalizedCompositeRoundTrips)
	{
		tm source = {};
		source.tm_year = 124;
		source.tm_mon = 1;
		source.tm_mday = 9;
		source.tm_hour = 13;
		source.tm_min = 4;
		source.tm_sec = 5;

		AssertDate(Parse(FormatTime(L"%x", source, L"de-DE"), L"%x", L"de-DE"), 2024, 2, 9);
		AssertTime(Parse(FormatTime(L"%X", source, L"de-DE"), L"%X", L"de-DE"), 13, 4, 5);
	}

	TEST_METHOD(TestUserDefaultLocaleFormats)
	{
		SYSTEMTIME source = {};
		source.wYear = 2024;
		source.wMonth = 2;
		source.wDay = 9;
		source.wHour = 13;
		source.wMinute = 4;
		source.wSecond = 5;
		WCHAR buffer[256] = {};

		Assert::IsTrue(GetDateFormat(LOCALE_USER_DEFAULT, 0, &source, nullptr, buffer, _countof(buffer)) > 0);
		AssertDate(Parse(buffer, L"%x", L"local"), 2024, 2, 9);
		Assert::IsTrue(GetTimeFormat(LOCALE_USER_DEFAULT, 0, &source, nullptr, buffer, _countof(buffer)) > 0);
		const SYSTEMTIME parsedTime = Parse(buffer, L"%X", L"local");
		Assert::AreEqual(13, (int)parsedTime.wHour);
		Assert::AreEqual(4, (int)parsedTime.wMinute);
	}

	TEST_METHOD(TestTwelveHourTime)
	{
		AssertTime(Parse(L"12:05:09 AM", L"%I:%M:%S %p"), 0, 5, 9);
		AssertTime(Parse(L"12:05:09 PM", L"%I:%M:%S %p"), 12, 5, 9);
		AssertTime(Parse(L"PM 1:05:09", L"%p %I:%M:%S"), 13, 5, 9);
	}

	TEST_METHOD(TestAmPmRequiresTwelveHourValue)
	{
		AssertError(L"PM", L"%p", DateTimeParser::ParseError::InvalidDate);
	}

	TEST_METHOD(TestConflictingHours)
	{
		AssertError(L"13 1 AM", L"%H %I %p", DateTimeParser::ParseError::InvalidDate);
	}

	TEST_METHOD(TestPaddingModifiers)
	{
		const SYSTEMTIME time = Parse(L"2024-2-9 3:4:5", L"%#Y-%#m-%#d %#H:%#M:%#S");
		AssertDate(time, 2024, 2, 9);
		AssertTime(time, 3, 4, 5);
	}

	TEST_METHOD(TestAlternativeModifiersInCLocale)
	{
		AssertDate(Parse(L"20 24-02-09", L"%EC %Ey-%Om-%Od"), 2024, 2, 9);
		AssertTime(Parse(L"3:4:5", L"%OH:%OM:%OS"), 3, 4, 5);
	}

	TEST_METHOD(TestRemainingHashModifiers)
	{
		AssertDate(Parse(L"24 60", L"%#y %#j"), 2024, 2, 29);
		AssertTime(Parse(L"1 PM", L"%#I %p"), 13, 0, 0);
		AssertDate(Parse(L"2023 1 0", L"%Y %#U %#w"), 2023, 1, 1);
		AssertDate(Parse(L"2023 1 1", L"%Y %#W %w"), 2023, 1, 2);
	}

	TEST_METHOD(TestRemainingAlternativeModifiers)
	{
		AssertDate(Parse(L"24-2- 9", L"%Oy-%Om-%Oe"), 2024, 2, 9);
		AssertTime(Parse(L"1 PM", L"%OI %p"), 13, 0, 0);
	}

	TEST_METHOD(TestAllAlternativeWeekModifiers)
	{
		AssertDate(Parse(L"2023 01 1", L"%Y %OW %Ou"), 2023, 1, 2);
		AssertDate(Parse(L"2023 01 0", L"%Y %OU %Ow"), 2023, 1, 1);
		AssertDate(Parse(L"2016 01 1", L"%G %OV %Ou"), 2016, 1, 4);
	}

	TEST_METHOD(TestSpacePaddedDay)
	{
		AssertDate(Parse(L"2024-02- 9", L"%Y-%m-%e"), 2024, 2, 9);
	}

	TEST_METHOD(TestWhitespaceDirectives)
	{
		AssertDate(Parse(L"2024\r\n\t02 09", L"%Y%n%m%t%d"), 2024, 2, 9);
	}

	TEST_METHOD(TestWhitespaceUsesConfiguredLocale)
	{
		AssertDate(Parse(L"2024\v02\v09", L"%Y %m %d", L"C"), 2024, 2, 9);
	}

	TEST_METHOD(TestNumericDirectivesSkipLeadingWhitespace)
	{
		AssertDate(Parse(L" \t2024- \r\n02- 09", L"%Y-%m-%d"), 2024, 2, 9);
	}

	TEST_METHOD(TestLiteralsAreCaseSensitive)
	{
		AssertError(L"T2024", L"t%Y", DateTimeParser::ParseError::InputMismatch);
	}

	TEST_METHOD(TestEmptyInputMatchesWhitespaceOnlyFormat)
	{
		AssertDate(Parse(L"", L" \t\r\n"), 1900, 1, 1);
	}

	TEST_METHOD(TestPercentLiteral)
	{
		AssertDate(Parse(L"2024%02%09", L"%Y%%%m%%%d"), 2024, 2, 9);
	}

	TEST_METHOD(TestPrefixMode)
	{
		const auto result = DateTimeParser::Parse(L"2024-02-09 trailing", L"%F", L"C");
		Assert::IsTrue((bool)result);
		Assert::AreEqual((size_t)10, result.consumed);
	}

	TEST_METHOD(TestFullModeAllowsTrailingWhitespace)
	{
		Parse(L"2024-02-09 \r\n", L"%F");
	}

	TEST_METHOD(TestFullModeRejectsTrailingText)
	{
		AssertError(L"2024-02-09 trailing", L"%F", DateTimeParser::ParseError::InputMismatch);
	}

	TEST_METHOD(TestUtcOffsetMovesToUtc)
	{
		const SYSTEMTIME time = Parse(L"2024-01-01 01:30 +0200", L"%F %R %z");
		AssertDate(time, 2023, 12, 31);
		AssertTime(time, 23, 30, 0);
	}

	TEST_METHOD(TestNegativeUtcOffsetMovesToUtc)
	{
		const SYSTEMTIME time = Parse(L"2024-01-01 22:30 -0500", L"%F %R %z");
		AssertDate(time, 2024, 1, 2);
		AssertTime(time, 3, 30, 0);
	}

	TEST_METHOD(TestUtcZoneName)
	{
		const SYSTEMTIME time = Parse(L"2024-01-01 22:30 UTC", L"%F %R %Z");
		AssertDate(time, 2024, 1, 1);
		AssertTime(time, 22, 30, 0);
	}

	TEST_METHOD(TestUnknownTimeZone)
	{
		AssertError(L"2024-01-01 Nowhere Time", L"%F %Z", DateTimeParser::ParseError::InvalidTimeZone);
	}

	TEST_METHOD(TestMalformedUtcOffset)
	{
		AssertError(L"2024-01-01 +2460", L"%F %z", DateTimeParser::ParseError::InputMismatch);
		AssertError(L"2024-01-01 +010", L"%F %z", DateTimeParser::ParseError::InputMismatch);
	}

	TEST_METHOD(TestInvalidRanges)
	{
		AssertError(L"2024-13-01", L"%F", DateTimeParser::ParseError::InputMismatch);
		AssertError(L"24:00:00", L"%T", DateTimeParser::ParseError::InputMismatch);
		AssertError(L"23:60:00", L"%T", DateTimeParser::ParseError::InputMismatch);
		AssertError(L"23:59:60", L"%T", DateTimeParser::ParseError::InputMismatch);
	}

	TEST_METHOD(TestLeapSecondIsRejected)
	{
		AssertError(L"60", L"%S", DateTimeParser::ParseError::InputMismatch);
	}

	TEST_METHOD(TestInvalidCalendarDate)
	{
		AssertError(L"2023-02-29", L"%F", DateTimeParser::ParseError::InvalidDate);
		AssertError(L"2024-04-31", L"%F", DateTimeParser::ParseError::InvalidDate);
	}

	TEST_METHOD(TestInvalidFormat)
	{
		AssertError(L"2024", L"%Q", DateTimeParser::ParseError::InvalidFormat);
		AssertError(L"2024", L"%Ed", DateTimeParser::ParseError::InvalidFormat);
		AssertError(L"2024", L"%#B", DateTimeParser::ParseError::InvalidFormat);
	}

	TEST_METHOD(TestUnsupportedAndIllegalModifiersAreRejected)
	{
		AssertError(L"2024", L"%QY", DateTimeParser::ParseError::InvalidFormat);
		AssertError(L"December", L"%OB", DateTimeParser::ParseError::InvalidFormat);
	}

	TEST_METHOD(TestInvalidLocale)
	{
		const auto result = DateTimeParser::Parse(L"2024", L"%Y", L"@");
		Assert::AreEqual((int)DateTimeParser::ParseError::InvalidLocale, (int)result.error);
	}

	TEST_METHOD(TestLegacyLocaleSpellingIsAccepted)
	{
		AssertDate(Parse(L"12/26/15", L"%x", L"English_United States.1252"), 2015, 12, 26);
	}

	TEST_METHOD(TestDefaultComponents)
	{
		const SYSTEMTIME time = Parse(L"03:04:05", L"%T");
		AssertDate(time, 1900, 1, 1);
		AssertTime(time, 3, 4, 5);
	}

	TEST_METHOD(TestRepeatedMatchingField)
	{
		AssertDate(Parse(L"2024 2024-02-09", L"%Y %Y-%m-%d"), 2024, 2, 9);
	}

	TEST_METHOD(TestRepeatedConflictingField)
	{
		AssertError(L"2023 2024-02-09", L"%Y %Y-%m-%d", DateTimeParser::ParseError::InputMismatch);
	}
};
