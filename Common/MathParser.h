// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

// Heavily based on ccalc 0.5.1 by Walery Studennikov <hqsoftware@mail.ru>

#pragma once

#include <Windows.h>

class MathParser
{
public:
	typedef bool (*GetValueFunc)(const WCHAR* str, int len, double* value, void* context);

	MathParser(GetValueFunc getValue = nullptr, void* getValueContext = nullptr);

	const WCHAR* Check(const WCHAR* formula) const;
	const WCHAR* CheckedParse(const WCHAR* formula, double* result) const;

	enum class ParseMode
	{
		EntireString,

		// Parse a parenthesized formula and stop immediately after its outer closing bracket.
		MatchingClosingBracket
	};

	const WCHAR* Parse(const WCHAR* formula, double* result, ParseMode mode = ParseMode::EntireString, const WCHAR** parseEnd = nullptr) const;

	bool IsDelimiter(WCHAR ch) const;

private:
	GetValueFunc m_GetValue;
	void* m_GetValueContext;
};
