/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "ParseUtil.h"
#include "MathParser.h"

namespace {

struct PairInfo
{
	const WCHAR begin;
	const WCHAR end;
};

const std::unordered_map<PairedPunctuation, PairInfo> s_PairedPunct =
{
	{ PairedPunctuation::SingleQuote, { L'\'', L'\'' } },
	{ PairedPunctuation::DoubleQuote, { L'"', L'"' } },
	{ PairedPunctuation::BothQuotes,  { L'"', L'\'' } },
	{ PairedPunctuation::Parentheses, { L'(', L')' } },
	{ PairedPunctuation::Brackets,    { L'[', L']' } },
	{ PairedPunctuation::Braces,      { L'{', L'}' } },
	{ PairedPunctuation::Guillemet,   { L'<', L'>' } }
};

void ReportFormulaError(ParseUtil::FormulaErrorCallback errorCallback, const WCHAR* error, const WCHAR* formula)
{
	if (errorCallback)
	{
		errorCallback(error, formula);
	}
}

template <typename T>
bool ParseInt4(LPCTSTR str, T& v1, T& v2, T& v3, T& v4, ParseUtil::FormulaErrorCallback errorCallback)
{
	if (wcschr(str, L','))
	{
		std::wstring string = str;
		std::vector<T> tokens;
		size_t start = 0ULL;
		size_t end = 0ULL;
		int parens = 0;

		auto getToken = [&]() -> void
		{
			start = string.find_first_not_of(L" \t", start); // skip any leading whitespace
			if (start <= end)
			{
				tokens.push_back((T)ParseUtil::ParseInt(
					string.substr(start, end - start).c_str(), 0,
					errorCallback));
			}
		};

		for (auto& iter : string)
		{
			switch (iter)
			{
			case L'(': ++parens; break;
			case L')': --parens; break;
			case L',':
				{
					if (parens == 0)
					{
						getToken();
						start = end + 1ULL; // skip comma
						break;
					}
					//else multi arg function ?
				}
				break;
			}
			++end;
		}

		// read last token
		getToken();

		size_t size = tokens.size();
		if (size > 0ULL) v1 = tokens[0];
		if (size > 1ULL) v2 = tokens[1];
		if (size > 2ULL) v3 = tokens[2];
		if (size > 3ULL) v4 = tokens[3];

		return true;
	}

	return false;
}

}  // namespace

namespace ParseUtil {

double ParseDouble(LPCTSTR str, double defValue, FormulaErrorCallback errorCallback)
{
	assert(str);

	double value = 0.0;
	if (*str == L'(')
	{
		const WCHAR* errMsg = MathParser::CheckedParse(str, &value);
		if (!errMsg)
		{
			return value;
		}

		ReportFormulaError(errorCallback, errMsg, str);
	}
	else if (*str)
	{
		errno = 0;
		double value = wcstod(str, nullptr);
		if (errno != ERANGE)
		{
			return value;
		}
	}

	return defValue;
}

int ParseInt(LPCTSTR str, int defValue, FormulaErrorCallback errorCallback)
{
	assert(str);

	if (*str == L'(')
	{
		double value = 0.0;
		const WCHAR* errMsg = MathParser::CheckedParse(str, &value);
		if (!errMsg)
		{
			return (int)value;
		}

		ReportFormulaError(errorCallback, errMsg, str);
	}
	else if (*str)
	{
		errno = 0;
		int value = wcstol(str, nullptr, 10);
		if (errno != ERANGE)
		{
			return value;
		}
	}

	return defValue;
}

uint32_t ParseUInt(LPCTSTR str, uint32_t defValue, FormulaErrorCallback errorCallback)
{
	assert(str);

	if (*str == L'(')
	{
		double value = 0.0;
		const WCHAR* errMsg = MathParser::CheckedParse(str, &value);
		if (!errMsg)
		{
			return (uint32_t)value;
		}

		ReportFormulaError(errorCallback, errMsg, str);
	}
	else if (*str)
	{
		errno = 0;
		uint32_t value = wcstoul(str, nullptr, 10);
		if (errno != ERANGE)
		{
			return value;
		}
	}

	return defValue;
}

uint64_t ParseUInt64(LPCTSTR str, uint64_t defValue, FormulaErrorCallback errorCallback)
{
	assert(str);

	if (*str == L'(')
	{
		double value = 0.0;
		const WCHAR* errMsg = MathParser::CheckedParse(str, &value);
		if (!errMsg)
		{
			return (uint64_t)value;
		}

		ReportFormulaError(errorCallback, errMsg, str);
	}
	else if (*str)
	{
		errno = 0;
		uint64_t value = _wcstoui64(str, nullptr, 10);
		if (errno != ERANGE)
		{
			return value;
		}
	}

	return defValue;
}

D2D1_COLOR_F ParseColor(LPCTSTR str, FormulaErrorCallback errorCallback)
{
	int R = 255, G = 255, B = 255, A = 255;

	if (!ParseInt4(str, R, G, B, A, errorCallback))
	{
		if (wcsncmp(str, L"0x", 2ULL) == 0)
		{
			str += 2;  // skip prefix
		}

		size_t len = wcslen(str);
		if (len >= 8 && !iswspace(str[6]))
		{
			swscanf_s(str, L"%02x%02x%02x%02x", &R, &G, &B, &A);
		}
		else if (len >= 6ULL)
		{
			swscanf_s(str, L"%02x%02x%02x", &R, &G, &B);
		}
	}

	return D2D1::ColorF(R / 255.0f, G / 255.0f, B / 255.0f, A / 255.0f);
}

D2D1_RECT_F ParseRect(LPCTSTR str, FormulaErrorCallback errorCallback)
{
	D2D1_RECT_F r = D2D1::RectF();
	ParseInt4(str, r.left, r.top, r.right, r.bottom, errorCallback);
	r.right += r.left;
	r.bottom += r.top;
	return r;
}

RECT ParseRECT(LPCTSTR str, FormulaErrorCallback errorCallback)
{
	RECT r = { 0 };
	ParseInt4(str, r.left, r.top, r.right, r.bottom, errorCallback);
	return r;
}

// Modified from http://www.digitalpeer.com/id/simple
std::vector<std::wstring> Tokenize(const std::wstring& str, const std::wstring& delimiters)
{
	std::vector<std::wstring> tokens;

	size_t lastPos = 0ULL, pos = 0ULL;
	do
	{
		lastPos = str.find_first_not_of(delimiters, pos);
		if (lastPos == std::wstring::npos) break;

		pos = str.find_first_of(delimiters, lastPos + 1);
		std::wstring token = str.substr(lastPos, pos - lastPos);  // len = (pos != std::wstring::npos) ? pos - lastPos : pos

		size_t pos2 = token.find_first_not_of(L" \t\r\n");
		if (pos2 != std::wstring::npos)
		{
			size_t lastPos2 = token.find_last_not_of(L" \t\r\n");
			if (pos2 != 0 || lastPos2 != (token.size() - 1))
			{
				// Trim white-space
				token.assign(token, pos2, lastPos2 - pos2 + 1);
			}
			tokens.push_back(token);
		}

		if (pos == std::wstring::npos) break;
		++pos;
	}
	while (true);

	return tokens;
}

std::vector<std::wstring> TokenizeWithPairedPunctuation(const std::wstring& str, const WCHAR delimiter, const PairedPunctuation punct)
{
	std::vector<std::wstring> tokens;
	size_t start = 0ULL;
	size_t end = 0ULL;

	auto getToken = [&]() -> void
	{
		start = str.find_first_not_of(L" \t\r\n", start); // skip any leading whitespace
		if (start <= end)
		{
			std::wstring temp = str.substr(start, end - start);
			temp.erase(temp.find_last_not_of(L" \t\r\n") + 1); // remove any trailing whitespace
			tokens.push_back(temp);
		}
	};

	if (punct == PairedPunctuation::SingleQuote ||
		punct == PairedPunctuation::DoubleQuote)
	{
		bool found = false;
		for (auto& iter : str)
		{
			if (iter == s_PairedPunct.at(punct).begin) found = !found;
			else if (iter == delimiter && !found)
			{
				getToken();
				start = end + 1;  // skip delimiter
			}
			++end;
		}
	}
	else if (punct == PairedPunctuation::BothQuotes)
	{
		// Skip delimiters if inside either a pair of single quotes, or a pair of double quotes
		bool found = false;
		WCHAR current = L'\0';
		for (auto& iter : str)
		{
			if (!current &&
				(iter == s_PairedPunct.at(punct).begin ||	// single quote
				 iter == s_PairedPunct.at(punct).end))		// double quote
			{
				current = iter;
				found = true;
			}
			else if (iter == current)
			{
				current = L'\0';
				found = false;
			}
			else if (iter == delimiter && !found)
			{
				getToken();
				start = end + 1;  // skip delimiter
			}
			++end;
		}
	}
	else
	{
		int pairs = 0;
		for (auto& iter : str)
		{
			if (iter == s_PairedPunct.at(punct).begin) ++pairs;
			else if (iter == s_PairedPunct.at(punct).end) --pairs;
			else if (iter == delimiter && pairs == 0)
			{
				getToken();
				start = end + 1;  // skip delimiter
			}
			++end;
		}
	}

	// Get last token
	getToken();

	return tokens;
}

}  // namespace ParseUtil
