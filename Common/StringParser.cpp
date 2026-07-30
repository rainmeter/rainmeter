/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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

template <typename T, typename ParseFunc>
std::optional<T> ConsumeNumber(const WCHAR*& current, const WCHAR* end, ParseFunc parseFunc, StringParser::Option option)
{
	if (current >= end) return std::nullopt;
	if (option != StringParser::AllowWhitespace && IsWhitespace(current, end)) return std::nullopt;

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

	if (option == StringParser::AllowWhitespace)
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
	if (option != StringParser::AllowWhitespace && IsWhitespace(current, end)) return std::nullopt;

	const WCHAR* start = current;
	if (option == StringParser::AllowWhitespace)
	{
		while (IsWhitespace(current, end))
		{
			++current;
		}
	}

	if (current < end && *current == L'(')
	{
		const std::wstring formula(current, end);
		const WCHAR* parseEnd = nullptr;
		double value = 0.0;
		const WCHAR* error = mathParser.Parse(
			formula.c_str(), &value, MathParser::ParseMode::MatchingClosingBracket, &parseEnd);
		if (error)
		{
			current = start;
			return std::nullopt;
		}

		current += parseEnd - formula.c_str();
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

	if (option == StringParser::AllowWhitespace)
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

bool StringParser::Consume(const WCHAR* str, size_t length, Option option)
{
	assert(str);

	if (option == AllowWhitespace)
	{
		SkipWhitespace();
	}

	const size_t remaining = (size_t)(m_End - m_Current);
	if (length > remaining) return false;
	if (_wcsnicmp(m_Current, str, length) != 0) return false;

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

	if (option == AllowWhitespace)
	{
		SkipWhitespace();
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

bool StringParser::Consume(WCHAR ch)
{
	if (m_Current >= m_End || towlower(*m_Current) != towlower(ch)) return false;

	++m_Current;
	return true;
}

bool StringParser::ConsumeRest(WCHAR ch)
{
	const WCHAR* current = m_Current;
	if (!Consume(ch) || m_Current != m_End)
	{
		m_Current = current;
		return false;
	}

	return true;
}

std::optional<std::wstring_view> StringParser::ConsumeCharacters(size_t length)
{
	if (length > (size_t)(m_End - m_Current)) return std::nullopt;

	const std::wstring_view result(m_Current, length);
	m_Current += length;
	return result;
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

void StringParser::SkipWhitespace()
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
