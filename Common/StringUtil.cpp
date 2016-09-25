/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "StringUtil.h"

namespace {

// Is the character a end of sentence punctuation character?
// English only?
bool IsEOSPunct(wchar_t ch)
{
	return ch == '?' || ch == '!' || ch == '.';
}

}

namespace StringUtil {

std::string Narrow(const WCHAR* str, int strLen, int cp)
{
	std::string narrowStr;

	if (str && *str)
	{
		if (strLen == -1)
		{
			strLen = (int)wcslen(str);
		}

		int bufLen = WideCharToMultiByte(cp, 0, str, strLen, nullptr, 0, nullptr, nullptr);
		if (bufLen > 0)
		{
			narrowStr.resize(bufLen);
			WideCharToMultiByte(cp, 0, str, strLen, &narrowStr[0], bufLen, nullptr, nullptr);
		}
	}
	return narrowStr;
}

std::wstring Widen(const char* str, int strLen, int cp)
{
	std::wstring wideStr;

	if (str && *str)
	{
		if (strLen == -1)
		{
			strLen = (int)strlen(str);
		}

		int bufLen = MultiByteToWideChar(cp, 0, str, strLen, nullptr, 0);
		if (bufLen > 0)
		{
			wideStr.resize(bufLen);
			MultiByteToWideChar(cp, 0, str, strLen, &wideStr[0], bufLen);
		}
	}
	return wideStr;
}

void ToLowerCase(std::wstring& str)
{
	WCHAR* srcAndDest = &str[0];
	int strAndDestLen = (int)str.length();
	LCMapString(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, srcAndDest, strAndDestLen, srcAndDest, strAndDestLen);
}

void ToUpperCase(std::wstring& str)
{
	WCHAR* srcAndDest = &str[0];
	int strAndDestLen = (int)str.length();
	LCMapString(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE, srcAndDest, strAndDestLen, srcAndDest, strAndDestLen);
}

void ToProperCase(std::wstring& str)
{
	WCHAR* srcAndDest = &str[0];
	int strAndDestLen = (int)str.length();
	LCMapString(LOCALE_USER_DEFAULT, LCMAP_TITLECASE, srcAndDest, strAndDestLen, srcAndDest, strAndDestLen);
}

void ToSentenceCase(std::wstring& str)
{
	if (!str.empty())
	{
		ToLowerCase(str);
		bool isCapped = false;

		for (size_t i = 0; i < str.length(); ++i)
		{
			if (IsEOSPunct(str[i])) isCapped = false;

			if (!isCapped && iswalpha(str[i]) != 0)
			{
				WCHAR* srcAndDest = &str[i];
				LCMapString(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE, srcAndDest, 1, srcAndDest, 1);
				isCapped = true;
			}
		}
	}
}

/*
** Escapes reserved PCRE regex metacharacters.
*/
void EscapeRegExp(std::wstring& str)
{
	size_t start = 0;
	while ((start = str.find_first_of(L"\\^$|()[{.+*?", start)) != std::wstring::npos)
	{
		str.insert(start, L"\\");
		start += 2;
	}
}

/*
** Escapes reserved URL characters.
*/
void EncodeUrl(std::wstring& str)
{
	size_t pos = 0;
	while ((pos = str.find_first_of(L" !*'();:@&=+$,/?#[]", pos)) != std::wstring::npos)
	{
		WCHAR buffer[3];
		_snwprintf_s(buffer, _countof(buffer), L"%.2X", str[pos]);
		str[pos] = L'%';
		str.insert(pos + 1, buffer);
		pos += 3;
	}
}


void RmNullCRTInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
{
	// Do nothing.
}

std::wstring Format(const WCHAR* str, ...)
{

	va_list args;
	va_start(args, str);
	WCHAR* buffer = new WCHAR[1024];

	_invalid_parameter_handler oldHandler = _set_invalid_parameter_handler(RmNullCRTInvalidParameterHandler);
	_CrtSetReportMode(_CRT_ASSERT, 0);

	errno = 0;
	_vsnwprintf_s(buffer, 1024, _TRUNCATE, str, args);
	if (errno != 0)
	{
		_snwprintf_s(buffer, 1024, _TRUNCATE, L"Internal error: %s", str);
	}

	_set_invalid_parameter_handler(oldHandler);

	std::wstring end(buffer);
	delete[] buffer;
	va_end(args);
	return end;
}

}  // namespace StringUtil
