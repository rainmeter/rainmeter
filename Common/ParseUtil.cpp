// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "ParseUtil.h"
#include "MathParser.h"
#include "StringParser.h"

namespace {

void ReportFormulaError(ParseUtil::FormulaErrorCallback errorCallback, const WCHAR* error, const WCHAR* formula)
{
	if (errorCallback)
	{
		errorCallback(error, formula);
	}
}

template <typename T, typename ConsumeFunc>
T ParseNumber(std::wstring_view str, T defValue, const MathParser& mathParser, ParseUtil::FormulaErrorCallback errorCallback, ConsumeFunc consumeFunc)
{
	StringParser parser(str);
	parser.ConsumeWhitespace();

	if (parser.Remaining().starts_with(L'('))
	{
		const std::wstring formula(parser.Remaining());
		double value = 0.0;
		const WCHAR* errMsg = mathParser.CheckedParse(formula.c_str(), &value);
		if (!errMsg)
		{
			return (T)value;
		}

		ReportFormulaError(errorCallback, errMsg, formula.c_str());
		return defValue;
	}

	return consumeFunc(parser).value_or(defValue);
}

template <typename T>
bool ParseInt4(std::wstring_view str, T& v1, T& v2, T& v3, T& v4, const MathParser& mathParser, ParseUtil::FormulaErrorCallback errorCallback)
{
	if (str.find(L',') == std::wstring_view::npos) return false;

	T* const values[] = { &v1, &v2, &v3, &v4 };
	size_t index = 0;

	StringParser parser(str);
	while (index < _countof(values))
	{
		// A trailing value consisting only of whitespace is not a value, and leaves the
		// remaining components at their defaults.
		parser.ConsumeWhitespace();
		if (parser.IsConsumed()) break;

		const auto value = parser.ConsumeUntilOrRest(L',', StringParser::SkipNestedParentheses);
		*values[index++] = (T)ParseUtil::ParseInt(value, 0, mathParser, errorCallback);
	}

	return true;
}

}  // namespace

namespace ParseUtil {

double ParseDouble(LPCTSTR str, double defValue, const MathParser& mathParser, FormulaErrorCallback errorCallback)
{
	assert(str);

	double value = 0.0;
	if (*str == L'(')
	{
		const WCHAR* errMsg = mathParser.CheckedParse(str, &value);
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

int ParseInt(LPCTSTR str, int defValue, const MathParser& mathParser, FormulaErrorCallback errorCallback)
{
	assert(str);

	if (*str == L'(')
	{
		double value = 0.0;
		const WCHAR* errMsg = mathParser.CheckedParse(str, &value);
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

double ParseDouble(std::wstring_view str, double defValue, const MathParser& mathParser, FormulaErrorCallback errorCallback)
{
	return ParseNumber<double>(str, defValue, mathParser, errorCallback, [](StringParser& parser)
		{
			return parser.ConsumeDouble();
		});
}

int ParseInt(std::wstring_view str, int defValue, const MathParser& mathParser, FormulaErrorCallback errorCallback)
{
	return ParseNumber<int>(str, defValue, mathParser, errorCallback, [](StringParser& parser)
		{
			return parser.ConsumeInt();
		});
}

uint32_t ParseUInt(LPCTSTR str, uint32_t defValue, const MathParser& mathParser, FormulaErrorCallback errorCallback)
{
	assert(str);

	if (*str == L'(')
	{
		double value = 0.0;
		const WCHAR* errMsg = mathParser.CheckedParse(str, &value);
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

uint64_t ParseUInt64(LPCTSTR str, uint64_t defValue, const MathParser& mathParser, FormulaErrorCallback errorCallback)
{
	assert(str);

	if (*str == L'(')
	{
		double value = 0.0;
		const WCHAR* errMsg = mathParser.CheckedParse(str, &value);
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

D2D1_COLOR_F ParseColor(LPCTSTR str, const MathParser& mathParser, FormulaErrorCallback errorCallback)
{
	assert(str);
	return ParseColor(std::wstring_view(str), mathParser, errorCallback);
}

D2D1_COLOR_F ParseColor(std::wstring_view str, const MathParser& mathParser, FormulaErrorCallback errorCallback)
{
	int R = 255, G = 255, B = 255, A = 255;

	if (!ParseInt4(str, R, G, B, A, mathParser, errorCallback))
	{
		StringParser parser(str);
		parser.Consume(L"0x", StringParser::MatchCase);  // Skip the optional prefix

		// The alpha component is only read if it is not separated from the others by whitespace.
		const auto value = parser.Remaining();
		const bool hasAlpha = value.length() >= 8 && !iswspace(value[6]);
		if (hasAlpha || value.length() >= 6)
		{
			int* const components[] = { &R, &G, &B, &A };
			const size_t count = hasAlpha ? 4 : 3;
			for (size_t i = 0; i < count; ++i)
			{
				// Leave this and any remaining component at its default if it is not a number.
				const auto component = parser.ConsumeHexByte(StringParser::SkipWhitespace);
				if (!component) break;

				*components[i] = (int)*component;
			}
		}
	}

	return D2D1::ColorF(R / 255.0f, G / 255.0f, B / 255.0f, A / 255.0f);
}

D2D1_RECT_F ParseRect(LPCTSTR str, const MathParser& mathParser, FormulaErrorCallback errorCallback)
{
	D2D1_RECT_F r = D2D1::RectF();
	ParseInt4(str, r.left, r.top, r.right, r.bottom, mathParser, errorCallback);
	r.right += r.left;
	r.bottom += r.top;
	return r;
}

RECT ParseRECT(LPCTSTR str, const MathParser& mathParser, FormulaErrorCallback errorCallback)
{
	RECT r = { 0 };
	ParseInt4(str, r.left, r.top, r.right, r.bottom, mathParser, errorCallback);
	return r;
}

}  // namespace ParseUtil
