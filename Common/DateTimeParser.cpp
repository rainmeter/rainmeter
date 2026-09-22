// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "DateTimeParser.h"
#include "StringUtil.h"

#include <algorithm>
#include <array>
#include <ctime>
#include <cwctype>
#include <iterator>
#include <locale.h>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace {

constexpr int g_DefaultYear = 1900;
constexpr LONGLONG g_TicksPerMinute = 60 * 10000000;
constexpr LONGLONG g_TicksPerDay = 24 * 60 * g_TicksPerMinute;

bool g_RefreshNativeDigits = true;
std::array<WCHAR, 10> g_NativeDigits = {};

void EnsureNativeDigitsLoaded()
{
	if (!g_RefreshNativeDigits) return;

	g_RefreshNativeDigits = false;
	WCHAR digits[32] = {};
	if (GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SNATIVEDIGITS, digits, _countof(digits)) > 0 && wcslen(digits) == 10)
	{
		for (int index = 0; index < 10; ++index) g_NativeDigits[index] = digits[index];
	}
}

bool IsLeapYear(int year)
{
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

int DaysInYear(int year)
{
	return IsLeapYear(year) ? 366 : 365;
}

int DaysInMonth(int year, int month)
{
	static constexpr int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	return month == 2 && IsLeapYear(year) ? 29 : days[month - 1];
}

bool IsValidDate(int year, int month, int day)
{
	return year >= 1601 && year <= 9999 && month >= 1 && month <= 12 && day >= 1 && day <= DaysInMonth(year, month);
}

bool DateFromOrdinal(int year, int ordinal, int& month, int& day)
{
	if (ordinal < 1 || ordinal > DaysInYear(year)) return false;

	month = 1;
	while (ordinal > DaysInMonth(year, month))
	{
		ordinal -= DaysInMonth(year, month++);
	}

	day = ordinal;
	return true;
}

int Weekday(int year, int month, int day)
{
	SYSTEMTIME time = {};
	time.wYear = (WORD)year;
	time.wMonth = (WORD)month;
	time.wDay = (WORD)day;
	FILETIME fileTime = {};
	if (!SystemTimeToFileTime(&time, &fileTime)) return -1;

	FileTimeToSystemTime(&fileTime, &time);
	return time.wDayOfWeek;
}

bool DateFromYearWeek(int year, int week, int weekday, bool mondayFirst, int& month, int& day)
{
	const int firstWeekday = Weekday(year, 1, 1);
	if (firstWeekday < 0) return false;

	const int adjustedFirst = mondayFirst ? (firstWeekday + 6) % 7 : firstWeekday;
	const int adjustedWeekday = mondayFirst ? (weekday + 6) % 7 : weekday;
	const int firstWeekStart = (7 - adjustedFirst) % 7 + 1;
	const int ordinal = week == 0 ? adjustedWeekday - adjustedFirst + 1 : firstWeekStart + (week - 1) * 7 + adjustedWeekday;
	if (week == 0 && (ordinal < 1 || ordinal >= firstWeekStart)) return false;

	return DateFromOrdinal(year, ordinal, month, day);
}

bool DateFromIsoWeek(int year, int week, int isoWeekday, int& resultYear, int& month, int& day)
{
	SYSTEMTIME januaryFourth = {};
	januaryFourth.wYear = (WORD)year;
	januaryFourth.wMonth = 1;
	januaryFourth.wDay = 4;
	FILETIME fileTime = {};
	if (!SystemTimeToFileTime(&januaryFourth, &fileTime)) return false;

	if (!FileTimeToSystemTime(&fileTime, &januaryFourth)) return false;

	const int januaryFourthIsoWeekday = januaryFourth.wDayOfWeek == 0 ? 7 : januaryFourth.wDayOfWeek;
	ULARGE_INTEGER value = {};
	value.HighPart = fileTime.dwHighDateTime;
	value.LowPart = fileTime.dwLowDateTime;
	const LONGLONG dayOffset = -(januaryFourthIsoWeekday - 1) + (week - 1) * 7 + (isoWeekday - 1);
	if (dayOffset < 0 && value.QuadPart < (ULONGLONG)(-dayOffset * g_TicksPerDay)) return false;

	value.QuadPart += dayOffset * g_TicksPerDay;
	fileTime.dwHighDateTime = value.HighPart;
	fileTime.dwLowDateTime = value.LowPart;

	SYSTEMTIME result = {};
	if (!FileTimeToSystemTime(&fileTime, &result)) return false;

	// A requested week 53 is valid only when its Thursday still belongs to the requested ISO year.
	SYSTEMTIME thursday = result;
	ULARGE_INTEGER thursdayValue = value;
	thursdayValue.QuadPart += (4 - isoWeekday) * g_TicksPerDay;
	fileTime.dwHighDateTime = thursdayValue.HighPart;
	fileTime.dwLowDateTime = thursdayValue.LowPart;
	if (!FileTimeToSystemTime(&fileTime, &thursday) || thursday.wYear != year) return false;

	resultYear = result.wYear;
	month = result.wMonth;
	day = result.wDay;
	return true;
}

template<typename T>
bool SetField(std::optional<T>& field, T value)
{
	if (field && *field != value) return false;

	field = value;
	return true;
}

class LocaleData
{
public:
	LocaleData(std::wstring_view name, bool userDefault) :
		m_Locale(nullptr),
		m_Name(userDefault ? std::wstring() : std::wstring(name)),
		m_IsCLocale(!userDefault && StringUtil::EqualsIgnoreCase(name, L"C"))
	{
		const std::wstring localeName = userDefault ? L"" : std::wstring(name);
		m_Locale = _wcreate_locale(LC_ALL, localeName.c_str());
		if (!m_Locale) return;

		LoadNames();
	}

	~LocaleData()
	{
		if (m_Locale) _free_locale(m_Locale);
	}

	LocaleData(const LocaleData&) = delete;
	LocaleData& operator=(const LocaleData&) = delete;

	bool IsValid() const { return m_Locale != nullptr; }
	_locale_t GetLocale() const { return m_Locale; }

	std::span<const std::wstring> GetWeekdays(bool abbreviated) const { return abbreviated ? m_AbbreviatedWeekdays : m_Weekdays; }
	std::span<const std::wstring> GetMonths(bool abbreviated) const { return abbreviated ? m_AbbreviatedMonths : m_Months; }
	std::span<const std::wstring> GetAmPm() const { return m_AmPm; }

	std::wstring GetCompositeFormat(WCHAR directive, WCHAR modifier) const;

	bool IsCLocale() const { return m_IsCLocale; }

	bool IsSpace(WCHAR character) const { return _iswspace_l(character, m_Locale) != 0; }

private:
	std::wstring Format(const WCHAR* format, const tm& value) const
	{
		WCHAR buffer[128] = {};
		const size_t length = _wcsftime_l(buffer, _countof(buffer), format, &value, m_Locale);
		return std::wstring(buffer, length);
	}

	void LoadNames()
	{
		for (int index = 0; index < 7; ++index)
		{
			tm value = {};
			value.tm_year = 124;
			value.tm_mon = 0;
			value.tm_mday = 7 + index;
			value.tm_wday = index;
			m_Weekdays[index] = Format(L"%A", value);
			m_AbbreviatedWeekdays[index] = Format(L"%a", value);
		}

		for (int index = 0; index < 12; ++index)
		{
			tm value = {};
			value.tm_year = 124;
			value.tm_mon = index;
			value.tm_mday = 1;
			m_Months[index] = Format(L"%B", value);
			m_AbbreviatedMonths[index] = Format(L"%b", value);
		}

		tm morning = {};
		morning.tm_hour = 1;
		tm evening = {};
		evening.tm_hour = 13;
		m_AmPm[0] = Format(L"%p", morning);
		m_AmPm[1] = Format(L"%p", evening);
	}

	_locale_t m_Locale;
	std::wstring m_Name;
	bool m_IsCLocale;
	std::array<std::wstring, 7> m_Weekdays;
	std::array<std::wstring, 7> m_AbbreviatedWeekdays;
	std::array<std::wstring, 12> m_Months;
	std::array<std::wstring, 12> m_AbbreviatedMonths;
	std::array<std::wstring, 2> m_AmPm;
};

void AppendFormatLiteral(std::wstring& result, WCHAR character)
{
	if (character == L'%') result += L"%%";
	else result += character;
}

std::wstring ConvertWindowsFormatToCrtFormat(std::wstring_view format)
{
	std::wstring result;
	bool quoted = false;
	for (size_t index = 0; index < format.size(); ++index)
	{
		const WCHAR character = format[index];

		// Text inside single quotes is literal. Two adjacent quotes produce one literal quote,
		// including outside a quoted section.
		if (character == L'\'')
		{
			if (index + 1 < format.size() && format[index + 1] == L'\'')
			{
				AppendFormatLiteral(result, L'\'');
				++index;
			}
			else
			{
				quoted = !quoted;
			}

			continue;
		}

		// Windows also permits a backslash to escape the character that follows it.
		if (character == L'\\' && index + 1 < format.size())
		{
			AppendFormatLiteral(result, format[++index]);
			continue;
		}

		// A percent sign must be doubled so that wcsftime emits it instead of treating it as
		// the start of a directive.
		if (quoted || !wcschr(L"dMyHhmstg", character))
		{
			AppendFormatLiteral(result, character);
			continue;
		}

		// Windows gives repeated letters different meanings, while wcsftime uses one directive
		// for each supported form.
		size_t count = 1;
		while (index + count < format.size() && format[index + count] == character) ++count;
		index += count - 1;

		switch (character)
		{
		case L'd':
			result += count >= 4 ? L"%A" : count == 3 ? L"%a" : L"%d";
			break;

		case L'M':
			result += count >= 4 ? L"%B" : count == 3 ? L"%b" : L"%m";
			break;

		case L'y':
			result += count <= 2 ? L"%y" : L"%Y";
			break;

		case L'H':
			result += L"%H";
			break;

		case L'h':
			result += L"%I";
			break;

		case L'm':
			result += L"%M";
			break;

		case L's':
			result += L"%S";
			break;

		case L't':
			result += L"%p";
			break;

		case L'g':
			result += L"%EC";
			break;
		}
	}

	return result;
}

std::wstring LocaleData::GetCompositeFormat(WCHAR directive, WCHAR modifier) const
{
	if (directive == L'r') return L"%I:%M:%S %p";

	const bool longDate = modifier == L'#';
	if (m_IsCLocale)
	{
		switch (directive)
		{
		case L'c':
			return longDate ? L"%A, %B %d, %Y, %H:%M:%S" : L"%a %b %e %H:%M:%S %Y";

		case L'x':
			return longDate ? L"%A, %B %d, %Y" : L"%m/%d/%y";

		case L'X':
			return L"%H:%M:%S";
		}
	}

	WCHAR buffer[128] = {};
	const LCTYPE type = directive == L'X' ? LOCALE_STIMEFORMAT : longDate ? LOCALE_SLONGDATE : LOCALE_SSHORTDATE;
	const WCHAR* localeName = m_Name.empty() ? LOCALE_NAME_USER_DEFAULT : m_Name.c_str();
	if (GetLocaleInfoEx(localeName, type, buffer, _countof(buffer)) == 0)
	{
		if (directive == L'c') return L"%a %b %e %H:%M:%S %Y";
		if (directive == L'x') return L"%m/%d/%y";
		return L"%H:%M:%S";
	}

	std::wstring format = ConvertWindowsFormatToCrtFormat(buffer);
	if (directive == L'c')
	{
		WCHAR timeBuffer[128] = {};
		if (GetLocaleInfoEx(localeName, LOCALE_STIMEFORMAT, timeBuffer, _countof(timeBuffer)) == 0)
		{
			return L"%a %b %e %H:%M:%S %Y";
		}

		format += longDate ? L", " : L" ";
		format += ConvertWindowsFormatToCrtFormat(timeBuffer);
	}

	return format;
}

struct Fields
{
	std::optional<int> century;
	std::optional<int> year;
	std::optional<int> yearWithinCentury;
	std::optional<int> month;
	std::optional<int> day;
	std::optional<int> ordinal;
	std::optional<int> weekday;
	std::optional<int> hour24;
	std::optional<int> hour12;
	std::optional<int> minute;
	std::optional<int> second;
	std::optional<int> amPm;
	std::optional<int> weekU;
	std::optional<int> weekW;
	std::optional<int> isoYear;
	std::optional<int> isoYearWithinCentury;
	std::optional<int> isoWeek;
	std::optional<int> isoWeekday;
	std::optional<int> utcOffsetMinutes;
	std::optional<int> namedUtcOffsetMinutes;
	std::optional<std::vector<int>> alternativeYears;
};

struct Date
{
	int year;
	int month;
	int day;

	bool operator==(const Date&) const = default;
};

class Parser
{
public:
	Parser(std::wstring_view value, LocaleData& locale, LocaleData& systemLocale) :
		m_Value(value),
		m_Locale(locale),
		m_SystemLocale(systemLocale)
	{
		EnsureNativeDigitsLoaded();
	}

	DateTimeParser::ParseResult Run(std::wstring_view format, DateTimeParser::MatchMode matchMode)
	{
		if (!ParseFormat(format, 0)) return Fail(m_Error);

		const size_t parsedLength = m_Position;
		if (matchMode == DateTimeParser::MatchMode::Full)
		{
			ConsumeWhitespace();
			if (m_Position != m_Value.size()) return Fail(DateTimeParser::ParseError::InputMismatch);
		}

		FILETIME timestamp = {};
		const DateTimeParser::ParseError error = Resolve(timestamp);
		if (error != DateTimeParser::ParseError::None) return Fail(error);

		DateTimeParser::ParseResult result;
		result.timestamp = timestamp;
		result.consumed = parsedLength;
		return result;
	}

private:
	DateTimeParser::ParseResult Fail(DateTimeParser::ParseError error) const
	{
		DateTimeParser::ParseResult result;
		result.consumed = m_Position;
		result.error = error;
		return result;
	}

	void ConsumeWhitespace()
	{
		while (m_Position < m_Value.size() && m_Locale.IsSpace(m_Value[m_Position])) ++m_Position;
	}

	bool ParseFormat(std::wstring_view format, int depth)
	{
		if (depth > 8)
		{
			m_Error = DateTimeParser::ParseError::InvalidFormat;
			return false;
		}

		for (size_t index = 0; index < format.size(); ++index)
		{
			const WCHAR character = format[index];
			if (character != L'%')
			{
				if (m_Locale.IsSpace(character))
				{
					ConsumeWhitespace();
				}
				else if (!ConsumeLiteral(character))
				{
					return false;
				}

				continue;
			}

			if (++index >= format.size()) return InvalidFormat();

			WCHAR modifier = 0;
			if (format[index] == L'#' || format[index] == L'E' || format[index] == L'O')
			{
				modifier = format[index++];
				if (index >= format.size()) return InvalidFormat();
			}

			if (!ParseDirective(format[index], modifier, depth)) return false;
		}

		return true;
	}

	bool InvalidFormat()
	{
		m_Error = DateTimeParser::ParseError::InvalidFormat;
		return false;
	}

	bool Mismatch()
	{
		m_Error = DateTimeParser::ParseError::InputMismatch;
		return false;
	}

	bool ConsumeLiteral(WCHAR character)
	{
		if (m_Position >= m_Value.size() || m_Value[m_Position] != character) return Mismatch();

		++m_Position;
		return true;
	}

	bool ConsumeNumber(int minimum, int maximum, int maxDigits, int& result, bool alternative)
	{
		ConsumeWhitespace();

		const size_t start = m_Position;
		int value = 0;
		int count = 0;
		while (m_Position < m_Value.size() && count < maxDigits)
		{
			int digit = -1;
			const WCHAR character = m_Value[m_Position];
			if (character >= L'0' && character <= L'9')
			{
				digit = character - L'0';
			}
			else if (alternative)
			{
				for (int index = 0; index < 10; ++index)
				{
					if (g_NativeDigits[index] == character) digit = index;
				}
			}
			if (digit < 0) break;

			value = value * 10 + digit;
			++m_Position;
			++count;
		}

		if (m_Position == start || value < minimum || value > maximum)
		{
			m_Position = start;
			return Mismatch();
		}

		result = value;
		return true;
	}

	bool ConsumeField(std::optional<int>& field, int minimum, int maximum, int maxDigits, bool alternative)
	{
		int value = 0;
		return ConsumeNumber(minimum, maximum, maxDigits, value, alternative) && (SetField(field, value) || Mismatch());
	}

	bool MatchNames(std::span<const std::wstring> primary, std::span<const std::wstring> secondary, int& result)
	{
		size_t bestLength = 0;
		int bestIndex = -1;
		auto consider = [&](std::span<const std::wstring> names)
		{
			for (size_t index = 0; index < names.size(); ++index)
			{
				const std::wstring& name = names[index];
				if (name.size() <= bestLength || name.size() > m_Value.size() - m_Position) continue;

				if (_wcsnicmp_l(m_Value.data() + m_Position, name.c_str(), name.size(), m_Locale.GetLocale()) == 0)
				{
					bestLength = name.size();
					bestIndex = (int)index;
				}
			}
		};

		consider(primary);
		consider(secondary);
		if (bestIndex < 0) return Mismatch();

		m_Position += bestLength;
		result = bestIndex;
		return true;
	}

	bool ParseComposite(WCHAR directive, WCHAR modifier, int depth)
	{
		LocaleData& locale = modifier == L'E' ? m_SystemLocale : m_Locale;
		const std::wstring format = locale.GetCompositeFormat(directive, modifier);
		if (format.empty()) return false;

		std::array<std::wstring, 4> formats = { format, {}, {}, {} };
		if (directive == L'c' && modifier == L'#' && locale.IsCLocale())
		{
			formats[1] = L"%A, %B %d, %Y %H:%M:%S";
			formats[2] = L"%A, %B %d, %Y, %I:%M:%S %p";
			formats[3] = L"%A, %B %d, %Y %I:%M:%S %p";
		}

		const size_t startPosition = m_Position;
		const Fields startFields = m_Fields;
		for (const std::wstring& candidate : formats)
		{
			if (candidate.empty()) continue;
			m_Position = startPosition;
			m_Fields = startFields;
			if (ParseFormat(candidate, depth + 1)) return true;
		}

		m_Position = startPosition;
		m_Fields = startFields;
		return false;
	}

	bool TryParseAlternativeYear(WCHAR directive)
	{
		const WCHAR* pattern = directive == L'C' ? L"gg" : directive == L'y' ? L"yy" : L"gg y";
		size_t bestLength = 0;
		std::vector<int> matchingYears;
		for (int year = 1601; year <= 9999; ++year)
		{
			// Sampling both halves of the year catches era transitions without embedding any
			// calendar-specific boundary data in the parser.
			for (WORD month : { (WORD)1, (WORD)7 })
			{
				SYSTEMTIME time = {};
				time.wYear = (WORD)year;
				time.wMonth = month;
				time.wDay = 1;
				WCHAR buffer[128] = {};
				const int length = GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_USE_ALT_CALENDAR, &time, pattern, buffer, _countof(buffer), nullptr);
				if (length <= 1) continue;

				const size_t tokenLength = length - 1;
				if (tokenLength < bestLength || tokenLength > m_Value.size() - m_Position) continue;

				if (_wcsnicmp(m_Value.data() + m_Position, buffer, tokenLength) != 0) continue;

				if (tokenLength > bestLength)
				{
					bestLength = tokenLength;
					matchingYears.clear();
				}
				matchingYears.push_back(year);
			}
		}
		if (matchingYears.empty()) return false;

		std::sort(matchingYears.begin(), matchingYears.end());
		matchingYears.erase(std::unique(matchingYears.begin(), matchingYears.end()), matchingYears.end());
		if (m_Fields.alternativeYears)
		{
			std::vector<int> intersection;
			std::set_intersection(
				m_Fields.alternativeYears->begin(), m_Fields.alternativeYears->end(),
				matchingYears.begin(), matchingYears.end(), std::back_inserter(intersection));
			if (intersection.empty()) return Mismatch();

			m_Fields.alternativeYears = std::move(intersection);
		}
		else
		{
			m_Fields.alternativeYears = std::move(matchingYears);
		}

		m_Position += bestLength;
		return true;
	}

	bool IsLegalModifier(WCHAR directive, WCHAR modifier) const
	{
		if (!modifier) return true;

		if (modifier == L'#') return wcschr(L"cdHIjmMSUwWxyY", directive) != nullptr;

		if (modifier == L'E') return wcschr(L"cCxXyY", directive) != nullptr;

		if (modifier == L'O') return wcschr(L"deHImMSuUVwWy", directive) != nullptr;

		return false;
	}

	bool ParseDirective(WCHAR directive, WCHAR modifier, int depth)
	{
		if (!IsLegalModifier(directive, modifier)) return InvalidFormat();

		const bool alternative = modifier == L'O';
		int value = 0;

		switch (directive)
		{
		case L'%':
			return !modifier && ConsumeLiteral(L'%');

		case L'a':
		case L'A':
			if (!MatchNames(m_Locale.GetWeekdays(directive == L'a'), m_Locale.GetWeekdays(directive != L'a'), value)) return false;

			return SetField(m_Fields.weekday, value) || Mismatch();

		case L'b':
		case L'B':
		case L'h':
			if (!MatchNames(m_Locale.GetMonths(directive != L'B'), m_Locale.GetMonths(directive == L'B'), value)) return false;

			return SetField(m_Fields.month, value + 1) || Mismatch();

		case L'c':
		case L'x':
		case L'X':
			return ParseComposite(directive, modifier, depth);

		case L'C':
			if (modifier == L'E' && TryParseAlternativeYear(directive)) return true;

			return ConsumeField(m_Fields.century, 0, 99, 2, alternative || modifier == L'E');

		case L'd':
		case L'e':
			if (directive == L'e' && m_Position < m_Value.size() && m_Value[m_Position] == L' ') ++m_Position;
			return ConsumeField(m_Fields.day, 1, 31, 2, alternative);

		case L'D':
			return !modifier && ParseFormat(L"%m/%d/%y", depth + 1);

		case L'F':
			return !modifier && ParseFormat(L"%Y-%m-%d", depth + 1);

		case L'g':
			return ConsumeField(m_Fields.isoYearWithinCentury, 0, 99, 2, alternative);

		case L'G':
			return ConsumeField(m_Fields.isoYear, 0, 9999, 4, alternative);

		case L'H':
			return ConsumeField(m_Fields.hour24, 0, 23, 2, alternative);

		case L'I':
			return ConsumeField(m_Fields.hour12, 1, 12, 2, alternative);

		case L'j':
			return ConsumeField(m_Fields.ordinal, 1, 366, 3, alternative);

		case L'm':
			return ConsumeField(m_Fields.month, 1, 12, 2, alternative);

		case L'M':
			return ConsumeField(m_Fields.minute, 0, 59, 2, alternative);

		case L'n':
		case L't':
			ConsumeWhitespace();
			return true;

		case L'p':
			if (!MatchNames(m_Locale.GetAmPm(), {}, value)) return false;

			return SetField(m_Fields.amPm, value) || Mismatch();

		case L'r':
			return ParseComposite(directive, modifier, depth);

		case L'R':
			return !modifier && ParseFormat(L"%H:%M", depth + 1);

		case L'S':
			return ConsumeField(m_Fields.second, 0, 59, 2, alternative);

		case L'T':
			return !modifier && ParseFormat(L"%H:%M:%S", depth + 1);

		case L'u':
			if (!ConsumeNumber(1, 7, 1, value, alternative)) return false;

			return (SetField(m_Fields.isoWeekday, value) && SetField(m_Fields.weekday, value % 7)) || Mismatch();

		case L'U':
			return ConsumeField(m_Fields.weekU, 0, 53, 2, alternative);

		case L'V':
			return ConsumeField(m_Fields.isoWeek, 1, 53, 2, alternative);

		case L'w':
			return ConsumeField(m_Fields.weekday, 0, 6, 1, alternative);

		case L'W':
			return ConsumeField(m_Fields.weekW, 0, 53, 2, alternative);

		case L'y':
			if (modifier == L'E' && m_Fields.alternativeYears && TryParseAlternativeYear(directive)) return true;

			return ConsumeField(m_Fields.yearWithinCentury, 0, 99, 2, alternative || modifier == L'E');

		case L'Y':
			if (modifier == L'E' && TryParseAlternativeYear(directive)) return true;

			return ConsumeField(m_Fields.year, 0, 9999, 4, alternative || modifier == L'E');

		case L'z':
			return !modifier && ParseUtcOffset();

		case L'Z':
			return !modifier && ParseTimeZoneName();

		default:
			return InvalidFormat();
		}
	}

	bool ParseUtcOffset()
	{
		if (m_Position >= m_Value.size() || (m_Value[m_Position] != L'+' && m_Value[m_Position] != L'-')) return Mismatch();

		const bool negative = m_Value[m_Position++] == L'-';
		if (m_Value.size() - m_Position < 4) return Mismatch();

		for (size_t index = 0; index < 4; ++index)
		{
			if (m_Value[m_Position + index] < L'0' || m_Value[m_Position + index] > L'9') return Mismatch();
		}

		const int hours = (m_Value[m_Position] - L'0') * 10 + m_Value[m_Position + 1] - L'0';
		const int minutes = (m_Value[m_Position + 2] - L'0') * 10 + m_Value[m_Position + 3] - L'0';
		if (hours > 23 || minutes > 59) return Mismatch();

		m_Position += 4;
		const int offset = (hours * 60 + minutes) * (negative ? -1 : 1);
		return SetField(m_Fields.utcOffsetMinutes, offset) || Mismatch();
	}

	bool ParseTimeZoneName()
	{
		TIME_ZONE_INFORMATION zone = {};
		GetTimeZoneInformation(&zone);
		struct Candidate
		{
			const WCHAR* name;
			int offset;
		};
		const Candidate candidates[] = {
			{ zone.StandardName, -(zone.Bias + zone.StandardBias) },
			{ zone.DaylightName, -(zone.Bias + zone.DaylightBias) },
			{ L"UTC", 0 },
			{ L"GMT", 0 }
		};

		size_t bestLength = 0;
		int bestOffset = 0;
		for (const Candidate& candidate : candidates)
		{
			const size_t length = wcslen(candidate.name);
			if (length <= bestLength || length > m_Value.size() - m_Position) continue;

			if (_wcsnicmp(m_Value.data() + m_Position, candidate.name, length) == 0)
			{
				bestLength = length;
				bestOffset = candidate.offset;
			}
		}

		if (bestLength == 0)
		{
			m_Error = DateTimeParser::ParseError::InvalidTimeZone;
			return false;
		}
		m_Position += bestLength;
		return SetField(m_Fields.namedUtcOffsetMinutes, bestOffset) || Mismatch();
	}

	int ResolveYear() const
	{
		int year = g_DefaultYear;
		const bool hasRegularYear = m_Fields.year || m_Fields.century || m_Fields.yearWithinCentury;
		if (m_Fields.year) year = *m_Fields.year;
		else if (m_Fields.century) year = *m_Fields.century * 100 + m_Fields.yearWithinCentury.value_or(0);
		else if (m_Fields.yearWithinCentury) year = *m_Fields.yearWithinCentury <= 68 ? 2000 + *m_Fields.yearWithinCentury : 1900 + *m_Fields.yearWithinCentury;

		if (!m_Fields.alternativeYears) return year;

		if (hasRegularYear)
		{
			return std::binary_search(m_Fields.alternativeYears->begin(), m_Fields.alternativeYears->end(), year) ? year : 0;
		}

		if (std::binary_search(m_Fields.alternativeYears->begin(), m_Fields.alternativeYears->end(), g_DefaultYear)) return g_DefaultYear;

		return m_Fields.alternativeYears->front();
	}

	int ResolveIsoYear(int calendarYear) const
	{
		if (m_Fields.isoYear) return *m_Fields.isoYear;

		if (m_Fields.isoYearWithinCentury) return *m_Fields.isoYearWithinCentury <= 68 ? 2000 + *m_Fields.isoYearWithinCentury : 1900 + *m_Fields.isoYearWithinCentury;

		return calendarYear;
	}

	bool MergeDate(std::optional<Date>& resolved, int year, int month, int day) const
	{
		const Date date = { year, month, day };
		if (resolved && *resolved != date) return false;

		resolved = date;
		return true;
	}

	DateTimeParser::ParseError ResolveDate(int year, Date& result) const
	{
		if (m_Fields.year && m_Fields.century && *m_Fields.year / 100 != *m_Fields.century) return DateTimeParser::ParseError::InvalidDate;

		if (m_Fields.year && m_Fields.yearWithinCentury && *m_Fields.year % 100 != *m_Fields.yearWithinCentury) return DateTimeParser::ParseError::InvalidDate;

		if (m_Fields.isoYear && m_Fields.isoYearWithinCentury && *m_Fields.isoYear % 100 != *m_Fields.isoYearWithinCentury) return DateTimeParser::ParseError::InvalidDate;

		std::optional<Date> resolvedDate;
		if (m_Fields.month || m_Fields.day)
		{
			const int month = m_Fields.month.value_or(1);
			const int day = m_Fields.day.value_or(1);
			if (!IsValidDate(year, month, day) || !MergeDate(resolvedDate, year, month, day)) return DateTimeParser::ParseError::InvalidDate;
		}

		if (m_Fields.ordinal)
		{
			int month = 0;
			int day = 0;
			if (!DateFromOrdinal(year, *m_Fields.ordinal, month, day) || !MergeDate(resolvedDate, year, month, day)) return DateTimeParser::ParseError::InvalidDate;
		}

		if (m_Fields.weekU || m_Fields.weekW)
		{
			if (m_Fields.weekU && m_Fields.weekW) return DateTimeParser::ParseError::InvalidDate;

			const bool mondayFirst = m_Fields.weekW.has_value();
			const int week = mondayFirst ? *m_Fields.weekW : *m_Fields.weekU;
			const int weekday = m_Fields.weekday.value_or(mondayFirst ? 1 : 0);
			int month = 0;
			int day = 0;
			if (!DateFromYearWeek(year, week, weekday, mondayFirst, month, day) || !MergeDate(resolvedDate, year, month, day)) return DateTimeParser::ParseError::InvalidDate;
		}

		if (m_Fields.isoWeek || m_Fields.isoYear || m_Fields.isoYearWithinCentury)
		{
			const int isoYear = ResolveIsoYear(year);
			const int isoWeek = m_Fields.isoWeek.value_or(1);
			const int isoWeekday = m_Fields.isoWeekday.value_or(1);
			int resultYear = 0;
			int month = 0;
			int day = 0;
			if (!DateFromIsoWeek(isoYear, isoWeek, isoWeekday, resultYear, month, day)) return DateTimeParser::ParseError::InvalidDate;

			if ((m_Fields.year || m_Fields.century || m_Fields.yearWithinCentury) && resultYear != year) return DateTimeParser::ParseError::InvalidDate;

			if (!MergeDate(resolvedDate, resultYear, month, day)) return DateTimeParser::ParseError::InvalidDate;
		}

		result = resolvedDate.value_or(Date{ year, 1, 1 });
		if (resolvedDate && m_Fields.weekday && Weekday(result.year, result.month, result.day) != *m_Fields.weekday) return DateTimeParser::ParseError::InvalidDate;

		return DateTimeParser::ParseError::None;
	}

	DateTimeParser::ParseError ResolveTime(const Date& date, FILETIME& timestamp) const
	{
		int hour = m_Fields.hour24.value_or(0);
		if (m_Fields.hour12)
		{
			const int hour12 = *m_Fields.hour12 % 12 + (m_Fields.amPm.value_or(0) == 1 ? 12 : 0);
			if (m_Fields.hour24 && *m_Fields.hour24 != hour12) return DateTimeParser::ParseError::InvalidDate;

			hour = hour12;
		}
		else if (m_Fields.amPm)
		{
			return DateTimeParser::ParseError::InvalidDate;
		}

		SYSTEMTIME systemTime = {};
		systemTime.wYear = (WORD)date.year;
		systemTime.wMonth = (WORD)date.month;
		systemTime.wDay = (WORD)date.day;
		systemTime.wHour = (WORD)hour;
		systemTime.wMinute = (WORD)m_Fields.minute.value_or(0);
		systemTime.wSecond = (WORD)m_Fields.second.value_or(0);
		if (!SystemTimeToFileTime(&systemTime, &timestamp)) return DateTimeParser::ParseError::InvalidDate;

		return DateTimeParser::ParseError::None;
	}

	DateTimeParser::ParseError ApplyUtcOffset(FILETIME& timestamp) const
	{
		if (m_Fields.utcOffsetMinutes && m_Fields.namedUtcOffsetMinutes && *m_Fields.utcOffsetMinutes != *m_Fields.namedUtcOffsetMinutes)
		{
			return DateTimeParser::ParseError::InvalidTimeZone;
		}

		const std::optional<int> offset = m_Fields.utcOffsetMinutes ? m_Fields.utcOffsetMinutes : m_Fields.namedUtcOffsetMinutes;
		if (offset)
		{
			ULARGE_INTEGER value = {};
			value.HighPart = timestamp.dwHighDateTime;
			value.LowPart = timestamp.dwLowDateTime;
			const LONGLONG adjustment = (LONGLONG)*offset * g_TicksPerMinute;
			if (adjustment > 0 && value.QuadPart < (ULONGLONG)adjustment) return DateTimeParser::ParseError::InvalidDate;

			value.QuadPart -= adjustment;
			timestamp.dwHighDateTime = value.HighPart;
			timestamp.dwLowDateTime = value.LowPart;
			SYSTEMTIME adjusted = {};
			if (!FileTimeToSystemTime(&timestamp, &adjusted)) return DateTimeParser::ParseError::InvalidDate;
		}

		return DateTimeParser::ParseError::None;
	}

	DateTimeParser::ParseError Resolve(FILETIME& timestamp) const
	{
		const int year = ResolveYear();
		if (year < 1601 || year > 9999) return DateTimeParser::ParseError::InvalidDate;

		Date date = {};
		DateTimeParser::ParseError error = ResolveDate(year, date);
		if (error != DateTimeParser::ParseError::None) return error;

		error = ResolveTime(date, timestamp);
		if (error != DateTimeParser::ParseError::None) return error;

		return ApplyUtcOffset(timestamp);
	}

	std::wstring_view m_Value;
	LocaleData& m_Locale;
	LocaleData& m_SystemLocale;
	size_t m_Position = 0;
	DateTimeParser::ParseError m_Error = DateTimeParser::ParseError::InputMismatch;
	Fields m_Fields;
};

}  // namespace

namespace DateTimeParser {

void RefreshNativeDigits()
{
	g_RefreshNativeDigits = true;
}

ParseResult Parse(std::wstring_view value, std::wstring_view format, std::wstring_view locale, MatchMode matchMode)
{
	const bool userDefault = StringUtil::EqualsIgnoreCase(locale, L"local");
	LocaleData selectedLocale(userDefault ? std::wstring_view() : locale, userDefault);
	if (!selectedLocale.IsValid())
	{
		ParseResult result;
		result.error = ParseError::InvalidLocale;
		return result;
	}

	LocaleData systemLocale({}, true);
	if (!systemLocale.IsValid())
	{
		ParseResult result;
		result.error = ParseError::InvalidLocale;
		return result;
	}

	Parser parser(value, selectedLocale, systemLocale);
	return parser.Run(format, matchMode);
}

}  // namespace DateTimeParser
