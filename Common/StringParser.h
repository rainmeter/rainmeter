/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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
		AllowWhitespace = 1
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

	std::optional<double> ConsumeDouble(Option option = None);
	std::optional<double> ConsumeRestDouble(Option option = None);
	std::optional<double> ConsumeDoubleOrFormula(const MathParser& mathParser, Option option = None);
	std::optional<double> ConsumeRestDoubleOrFormula(const MathParser& mathParser, Option option = None);

	std::optional<int> ConsumeInt(Option option = None);
	std::optional<int> ConsumeRestInt(Option option = None);
	std::optional<int> ConsumeIntOrFormula(const MathParser& mathParser, Option option = None);
	std::optional<int> ConsumeRestIntOrFormula(const MathParser& mathParser, Option option = None);

	std::optional<UINT> ConsumeUInt(Option option = None);
	std::optional<UINT> ConsumeRestUInt(Option option = None);
	std::optional<UINT> ConsumeUIntOrFormula(const MathParser& mathParser, Option option = None);
	std::optional<UINT> ConsumeRestUIntOrFormula(const MathParser& mathParser, Option option = None);

	void SkipWhitespace();
	bool IsWhitespace() const;

	bool IsConsumed() const { return m_Current >= m_End; }

private:
	const WCHAR* m_Current;
	const WCHAR* m_End;
};
