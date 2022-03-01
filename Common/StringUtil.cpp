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

void LTrim(std::wstring& str)
{
	str.erase(str.begin(), std::find_if(str.begin(), str.end(),
		[](wint_t ch) { return !std::iswspace(ch); }));
}

void RTrim(std::wstring& str)
{
	str.erase(std::find_if(str.rbegin(), str.rend(),
		[](wint_t ch) { return !std::iswspace(ch); }).base(), str.end());
}

void Trim(std::wstring& str)
{
	LTrim(str);
	RTrim(str);
}

size_t StripLeadingAndTrailingQuotes(std::wstring& str, bool single)
{
	if (str.size() > 1ULL)
	{
		WCHAR first = str.front();
		WCHAR last = str.back();
		if ((first == L'"' && last == L'"') ||				// "some string"
			(single && first == L'\'' && last == L'\''))	// 'some string'
		{
			str.erase(0ULL, 1ULL);
			str.erase(str.size() - 1ULL);
		}
	}
	return str.size();
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
void EncodeUrl(std::wstring& str, bool doReserved)
{
	static const std::string unreserved = "0123456789-.ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcedefghijklmnopqrstuvwxyz~";
	std::string utf8 = NarrowUTF8(str);
	for (size_t pos = 0; pos < utf8.size(); ++pos)
	{
		UCHAR ch = utf8[pos];
		if ((ch <= 0x20 || ch >= 0x7F) ||                              // control characters and non-ascii (includes space)
			(doReserved && unreserved.find(ch) == std::string::npos))  // any character other than unreserved characters
		{
			char buffer[3];
			_snprintf_s(buffer, _countof(buffer), "%.2X", ch);
			utf8[pos] = L'%';
			utf8.insert(pos + 1, buffer);
			pos += 2;
		}
	}
	str = WidenUTF8(utf8);
}

/*
** Case insensitive comparison of strings. If equal, strip str2 from str1 and any leading whitespace.
*/
bool CaseInsensitiveCompareN(std::wstring& str1, const std::wstring& str2)
{
	size_t pos = str2.length();
	if (_wcsnicmp(str1.c_str(), str2.c_str(), pos) == 0)
	{
		str1 = str1.substr(pos);  // remove str2 from str1
		str1.erase(0, str1.find_first_not_of(L" \t\r\n"));  // remove any leading whitespace
		return true;
	}

	return false;
}

}  // namespace StringUtil
