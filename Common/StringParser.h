// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>
#include <optional>
#include <string>

class MathParser;

// Bounded parser for incrementally consuming strings and numbers from a range.
//
// Consume advances on a match. ConsumeRest additionally requires the match to reach the end of
// the range.
class StringParser
{
public:
	enum Option
	{
		None = 0,

		// Skips leading whitespace before matching, and trailing whitespace of the match.
		SkipWhitespace = 1 << 0,

		// Ignores delimiters inside a pair of parentheses. Only used by ConsumeUntil.
		SkipNestedParentheses = 1 << 1,

		// Matches case sensitively instead of insensitively. Only used when matching a string.
		MatchCase = 1 << 2,

		// Ignores delimiters inside a pair of single or double quotes. Only used by ConsumeUntil.
		SkipQuoted = 1 << 3
	};

	explicit StringParser(std::wstring_view str);
	explicit StringParser(const WCHAR* str, int length = -1);

	template <size_t N>
	bool Consume(const WCHAR (&str)[N], Option option = None)
	{
		static_assert(N > 0, "String buffer must include a null terminator.");
		return Consume(str, N - 1, option);
	}

	template <size_t N>
	bool ConsumeRest(const WCHAR (&str)[N], Option option = None)
	{
		static_assert(N > 0, "String buffer must include a null terminator.");
		return ConsumeRest(str, N - 1, option);
	}

	template <size_t N>
	bool ConsumeSuffix(const WCHAR (&str)[N])
	{
		static_assert(N > 0, "String buffer must include a null terminator.");
		return ConsumeSuffix(str, N - 1);
	}

	bool Consume(const WCHAR* str, size_t length, Option option = None);
	bool ConsumeRest(const WCHAR* str, size_t length, Option option = None);
	bool ConsumeSuffix(const WCHAR* str, size_t length);

	bool Consume(const std::wstring& str, Option option = None) { return Consume(str.c_str(), str.length(), option); }
	bool ConsumeRest(const std::wstring& str, Option option = None) { return ConsumeRest(str.c_str(), str.length(), option); }
	bool ConsumeSuffix(const std::wstring& str) { return ConsumeSuffix(str.c_str(), str.length()); }

	bool Consume(WCHAR ch);
	bool ConsumeRest(WCHAR ch);

	// Returns the value before the delimiter, or an empty view if the range contains no delimiter.
	// The delimiter and, when no delimiter is found, the rest of the range are consumed.
	std::wstring_view ConsumeUntil(WCHAR delimiter, Option option = None);

	// Like ConsumeUntil, but returns the rest of the range if it contains no delimiter. Intended
	// for tokenizing a delimited list until IsConsumed.
	std::wstring_view ConsumeUntilOrRest(WCHAR delimiter, Option option = None);

	std::wstring_view ConsumeRest(Option option = None);

	std::optional<double> ConsumeDouble(Option option = None);
	std::optional<double> ConsumeRestDouble(Option option = None);
	std::optional<double> ConsumeDoubleOrFormula(const MathParser& mathParser, Option option = None);
	std::optional<double> ConsumeRestDoubleOrFormula(const MathParser& mathParser, Option option = None);

	std::optional<int> ConsumeInt(Option option = None);
	std::optional<int> ConsumeRestInt(Option option = None);
	std::optional<int> ConsumeIntOrFormula(const MathParser& mathParser, Option option = None);
	std::optional<int> ConsumeRestIntOrFormula(const MathParser& mathParser, Option option = None);

	// Consumes at most two hexadecimal digits, like the "%02x" scanf format.
	std::optional<UINT> ConsumeHexByte(Option option = None);

	std::optional<UINT> ConsumeUInt(Option option = None);
	std::optional<UINT> ConsumeRestUInt(Option option = None);
	std::optional<UINT> ConsumeUIntOrFormula(const MathParser& mathParser, Option option = None);
	std::optional<UINT> ConsumeRestUIntOrFormula(const MathParser& mathParser, Option option = None);

	void ConsumeWhitespace();
	bool IsWhitespace() const;

	bool IsConsumed() const { return m_Current >= m_End; }

	// Returns the unconsumed part of the range without consuming it.
	std::wstring_view Remaining() const { return std::wstring_view(m_Current, m_End - m_Current); }

private:
	// Advances to the next delimiter, or to the end of the range if there is none. Returns the
	// start of the value.
	const WCHAR* ScanToDelimiter(WCHAR delimiter, Option option);

	const WCHAR* m_Current;
	const WCHAR* m_End;
};

inline StringParser::Option operator|(StringParser::Option lhs, StringParser::Option rhs)
{
	return (StringParser::Option)((int)lhs | (int)rhs);
}
