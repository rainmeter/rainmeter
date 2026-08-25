// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "StringParser.h"

#include "MathParser.h"

namespace {

bool IsWhitespace(const WCHAR* current, const WCHAR* end)
{
	if (current >= end) return false;

	const auto ch = *current;
	return ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n';
}

bool HasOption(StringParser::Option option, StringParser::Option flag)
{
	return (option & flag) != 0;
}

std::wstring_view TrimValue(const WCHAR* start, const WCHAR* end, StringParser::Option option)
{
	if (HasOption(option, StringParser::SkipWhitespace))
	{
		while (end > start && iswspace(*(end - 1)))
		{
			--end;
		}
	}

	return std::wstring_view(start, end - start);
}

template <typename T, typename ParseFunc>
std::optional<T> ConsumeNumber(const WCHAR*& current, const WCHAR* end, ParseFunc parseFunc, StringParser::Option option)
{
	if (current >= end) return std::nullopt;
	if (!HasOption(option, StringParser::SkipWhitespace) && IsWhitespace(current, end)) return std::nullopt;

	WCHAR* parseEnd = nullptr;
	errno = 0;
	const T parsedValue = parseFunc(current, &parseEnd, 10);
	if (parseEnd == current || parseEnd > end || errno == ERANGE) return std::nullopt;

	current = parseEnd;
	return parsedValue;
}

template <typename T, typename ParseFunc>
std::optional<T> ConsumeRestNumber(const WCHAR*& current, const WCHAR* end, ParseFunc parseFunc, StringParser::Option option)
{
	const WCHAR* start = current;
	const auto value = ConsumeNumber<T>(current, end, parseFunc, option);
	if (!value)
	{
		current = start;
		return std::nullopt;
	}

	if (HasOption(option, StringParser::SkipWhitespace))
	{
		while (IsWhitespace(current, end))
		{
			++current;
		}
	}

	if (current != end)
	{
		current = start;
		return std::nullopt;
	}

	return value;
}

template <typename T, typename ParseFunc>
std::optional<T> ConsumeNumberOrFormula(
	const WCHAR*& current,
	const WCHAR* end,
	const MathParser& mathParser,
	ParseFunc parseFunc,
	StringParser::Option option)
{
	if (current >= end) return std::nullopt;
	if (!HasOption(option, StringParser::SkipWhitespace) && IsWhitespace(current, end)) return std::nullopt;

	const WCHAR* start = current;
	if (HasOption(option, StringParser::SkipWhitespace))
	{
		while (IsWhitespace(current, end))
		{
			++current;
		}
	}

	if (current < end && *current == L'(')
	{
		const WCHAR* parseEnd = nullptr;
		double value = 0.0;
		const WCHAR* error = mathParser.Parse(
			std::wstring_view(current, end - current), &value, MathParser::ParseMode::MatchingClosingBracket, &parseEnd);
		if (error)
		{
			current = start;
			return std::nullopt;
		}

		current = parseEnd;
		return (T)value;
	}

	current = start;
	return ConsumeNumber<T>(current, end, parseFunc, option);
}

template <typename T, typename ParseFunc>
std::optional<T> ConsumeRestNumberOrFormula(
	const WCHAR*& current,
	const WCHAR* end,
	const MathParser& mathParser,
	ParseFunc parseFunc,
	StringParser::Option option)
{
	const WCHAR* start = current;
	const auto value = ConsumeNumberOrFormula<T>(current, end, mathParser, parseFunc, option);
	if (!value)
	{
		current = start;
		return std::nullopt;
	}

	if (HasOption(option, StringParser::SkipWhitespace))
	{
		while (IsWhitespace(current, end))
		{
			++current;
		}
	}

	if (current != end)
	{
		current = start;
		return std::nullopt;
	}

	return value;
}

}  // namespace

StringParser::StringParser(std::wstring_view str) :
	StringParser(str.data(), (int)str.length())
{
}

StringParser::StringParser(const WCHAR* str, int length) :
	m_Current(str ? str : L""),
	m_End(m_Current + ((length >= 0) ? length : (int)wcslen(m_Current)))
{
}

void StringParser::Split(std::wstring_view str, WCHAR delimiter, std::vector<std::wstring>& out, Option option)
{
	out.clear();
	ForEachToken(str, delimiter, [&](std::wstring_view token) { out.emplace_back(token); }, option);
}

bool StringParser::Consume(const WCHAR* str, size_t length, Option option)
{
	assert(str);

	if (HasOption(option, SkipWhitespace))
	{
		ConsumeWhitespace();
	}

	const size_t remaining = (size_t)(m_End - m_Current);
	if (length > remaining) return false;

	const int result = HasOption(option, MatchCase) ?
		wcsncmp(m_Current, str, length) :
		_wcsnicmp(m_Current, str, length);
	if (result != 0) return false;

	m_Current += length;
	return true;
}

bool StringParser::ConsumeRest(const WCHAR* str, size_t length, Option option)
{
	const WCHAR* current = m_Current;
	if (!Consume(str, length, option))
	{
		m_Current = current;
		return false;
	}

	if (HasOption(option, SkipWhitespace))
	{
		ConsumeWhitespace();
	}

	if (m_Current != m_End)
	{
		m_Current = current;
		return false;
	}

	return true;
}

bool StringParser::ConsumeSuffix(const WCHAR* str, size_t length)
{
	assert(str);

	const size_t remaining = (size_t)(m_End - m_Current);
	if (length > remaining || _wcsnicmp(m_End - length, str, length) != 0)
	{
		return false;
	}

	m_End -= length;
	return true;
}

bool StringParser::ConsumeSuffixFromLast(WCHAR ch)
{
	const std::wstring_view remaining = Remaining();
	const size_t pos = remaining.find_last_of(ch);
	if (pos == std::wstring_view::npos)
	{
		return false;
	}

	m_End = m_Current + pos;
	return true;
}

bool StringParser::Consume(WCHAR ch, Option option)
{
	if (HasOption(option, SkipWhitespace))
	{
		ConsumeWhitespace();
	}

	if (m_Current >= m_End || towlower(*m_Current) != towlower(ch)) return false;

	++m_Current;
	return true;
}

bool StringParser::ConsumeRest(WCHAR ch, Option option)
{
	const WCHAR* current = m_Current;
	if (!Consume(ch, option))
	{
		m_Current = current;
		return false;
	}

	if (HasOption(option, SkipWhitespace))
	{
		ConsumeWhitespace();
	}

	if (m_Current != m_End)
	{
		m_Current = current;
		return false;
	}

	return true;
}

const WCHAR* StringParser::ScanToDelimiter(WCHAR delimiter, Option option)
{
	if (HasOption(option, SkipWhitespace))
	{
		ConsumeWhitespace();
	}

	const WCHAR* start = m_Current;
	const bool skipNested = HasOption(option, SkipNestedParentheses);
	const bool skipQuoted = HasOption(option, SkipQuoted);
	int depth = 0;
	WCHAR quote = L'\0';
	while (m_Current < m_End)
	{
		const WCHAR ch = *m_Current;
		if (quote) { if (ch == quote) quote = L'\0'; }
		else if (skipQuoted && (ch == L'"' || ch == L'\'')) quote = ch;
		else if (skipNested && ch == L'(') ++depth;
		else if (skipNested && ch == L')') --depth;
		else if (ch == delimiter && depth == 0) break;

		++m_Current;
	}

	return start;
}

std::wstring_view StringParser::ConsumeUntil(WCHAR delimiter, Option option)
{
	const WCHAR* start = ScanToDelimiter(delimiter, option);
	if (m_Current == m_End) return {};

	return TrimValue(start, m_Current++, option);
}

std::wstring_view StringParser::ConsumeUntilOrRest(WCHAR delimiter, Option option)
{
	const WCHAR* start = ScanToDelimiter(delimiter, option);
	const WCHAR* valueEnd = m_Current;
	if (m_Current < m_End)
	{
		++m_Current;  // Skip the delimiter
	}

	return TrimValue(start, valueEnd, option);
}

std::wstring_view StringParser::ConsumeRest(Option option)
{
	if (HasOption(option, SkipWhitespace))
	{
		ConsumeWhitespace();
		while (m_End > m_Current && iswspace(*(m_End - 1)))
		{
			--m_End;
		}
	}

	const std::wstring_view value(m_Current, m_End - m_Current);
	m_Current = m_End;
	return value;
}

std::optional<double> StringParser::ConsumeDouble(Option option)
{
	return ConsumeNumber<double>(m_Current, m_End, [](const WCHAR* current, WCHAR** parseEnd, int)
		{
			return wcstod(current, parseEnd);
		}, option);
}

std::optional<double> StringParser::ConsumeRestDouble(Option option)
{
	return ConsumeRestNumber<double>(m_Current, m_End, [](const WCHAR* current, WCHAR** parseEnd, int)
		{
			return wcstod(current, parseEnd);
		}, option);
}

std::optional<double> StringParser::ConsumeDoubleOrFormula(const MathParser& mathParser, Option option)
{
	return ConsumeNumberOrFormula<double>(m_Current, m_End, mathParser, [](const WCHAR* current, WCHAR** parseEnd, int)
		{
			return wcstod(current, parseEnd);
		}, option);
}

std::optional<double> StringParser::ConsumeRestDoubleOrFormula(const MathParser& mathParser, Option option)
{
	return ConsumeRestNumberOrFormula<double>(m_Current, m_End, mathParser, [](const WCHAR* current, WCHAR** parseEnd, int)
		{
			return wcstod(current, parseEnd);
		}, option);
}

std::optional<int> StringParser::ConsumeInt(Option option)
{
	return ConsumeNumber<int>(m_Current, m_End, wcstol, option);
}

std::optional<int> StringParser::ConsumeRestInt(Option option)
{
	return ConsumeRestNumber<int>(m_Current, m_End, wcstol, option);
}

std::optional<int> StringParser::ConsumeIntOrFormula(const MathParser& mathParser, Option option)
{
	return ConsumeNumberOrFormula<int>(m_Current, m_End, mathParser, wcstol, option);
}

std::optional<int> StringParser::ConsumeRestIntOrFormula(const MathParser& mathParser, Option option)
{
	return ConsumeRestNumberOrFormula<int>(m_Current, m_End, mathParser, wcstol, option);
}

std::optional<UINT> StringParser::ConsumeHexByte(Option option)
{
	if (HasOption(option, SkipWhitespace))
	{
		ConsumeWhitespace();
	}

	UINT value = 0;
	int digits = 0;
	while (digits < 2 && m_Current < m_End)
	{
		const WCHAR ch = *m_Current;
		UINT digit = 0;
		if (ch >= L'0' && ch <= L'9') digit = ch - L'0';
		else if (ch >= L'a' && ch <= L'f') digit = ch - L'a' + 10;
		else if (ch >= L'A' && ch <= L'F') digit = ch - L'A' + 10;
		else break;

		value = (value * 16) + digit;
		++m_Current;
		++digits;
	}

	if (digits == 0) return std::nullopt;
	return value;
}

std::optional<UINT> StringParser::ConsumeUInt(Option option)
{
	return ConsumeNumber<UINT>(m_Current, m_End, wcstoul, option);
}

std::optional<UINT> StringParser::ConsumeRestUInt(Option option)
{
	return ConsumeRestNumber<UINT>(m_Current, m_End, wcstoul, option);
}

std::optional<UINT> StringParser::ConsumeUIntOrFormula(const MathParser& mathParser, Option option)
{
	return ConsumeNumberOrFormula<UINT>(m_Current, m_End, mathParser, wcstoul, option);
}

std::optional<UINT> StringParser::ConsumeRestUIntOrFormula(const MathParser& mathParser, Option option)
{
	return ConsumeRestNumberOrFormula<UINT>(m_Current, m_End, mathParser, wcstoul, option);
}

void StringParser::ConsumeWhitespace()
{
	while (IsWhitespace())
	{
		++m_Current;
	}
}

bool StringParser::IsWhitespace() const
{
	return ::IsWhitespace(m_Current, m_End);
}
