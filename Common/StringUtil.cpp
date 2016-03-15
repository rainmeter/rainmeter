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
	auto to_lower = [](wchar_t ch) { return std::use_facet<std::ctype<wchar_t>>(std::locale()).tolower(ch); };
	std::transform(str.begin(), str.end(), str.begin(), to_lower);
}

void ToUpperCase(std::wstring& str)
{
	auto to_upper = [](wchar_t ch) { return std::use_facet<std::ctype<wchar_t>>(std::locale()).toupper(ch); };
	std::transform(str.begin(), str.end(), str.begin(), to_upper);
}

void ToProperCase(std::wstring& str)
{
	auto to_upper = [](wchar_t& ch) { ch = std::use_facet<std::ctype<wchar_t>>(std::locale()).toupper(ch); };

	if (!str.empty())
	{
		ToLowerCase(str);
		bool isCapped = false;

		for (size_t i = 0; i < str.length(); ++i)
		{
			if (iswpunct(str[i]) != 0 || iswspace(str[i]) != 0) isCapped = false;

			if (!isCapped && iswalpha(str[i]) != 0)
			{
				to_upper(str[i]);
				isCapped = true;
			}
		}
	}
}

void ToSentenceCase(std::wstring& str)
{
	auto to_upper = [](wchar_t& ch) { ch = std::use_facet<std::ctype<wchar_t>>(std::locale()).toupper(ch); };

	if (!str.empty())
	{
		ToLowerCase(str);
		bool isCapped = false;

		for (size_t i = 0; i < str.length(); ++i)
		{
			if (IsEOSPunct(str[i])) isCapped = false;

			if (!isCapped && iswalpha(str[i]) != 0)
			{
				to_upper(str[i]);
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

}  // namespace StringUtil
