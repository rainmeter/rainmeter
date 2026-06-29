/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_COMMON_STRINGUTIL_H_
#define RM_COMMON_STRINGUTIL_H_

#include <Windows.h>
#include <string>

namespace StringUtil {

std::string Narrow(const WCHAR* str, int strLen = -1, int cp = CP_ACP);
inline std::string Narrow(const std::wstring& str, int cp = CP_ACP) { return Narrow(str.c_str(), (int)str.length(), cp); }

inline std::string NarrowUTF8(const WCHAR* str, int strLen = -1) { return Narrow(str, strLen, CP_UTF8); }
inline std::string NarrowUTF8(const std::wstring& str) { return Narrow(str.c_str(), (int)str.length(), CP_UTF8); }

std::wstring Widen(const char* str, int strLen = -1, int cp = CP_ACP);
inline std::wstring Widen(const std::string& str, int cp = CP_ACP) { return Widen(str.c_str(), (int)str.length(), cp); }

inline std::wstring WidenUTF8(const char* str, int strLen = -1) { return Widen(str, strLen, CP_UTF8); }
inline std::wstring WidenUTF8(const std::string& str) { return Widen(str.c_str(), (int)str.length(), CP_UTF8); }

void LTrim(std::wstring& str);
void RTrim(std::wstring& str);
void Trim(std::wstring& str);

size_t StripLeadingAndTrailingQuotes(std::wstring& str, bool single = false);

void ToLowerCase(std::wstring& str);
void ToUpperCase(std::wstring& str);
void ToProperCase(std::wstring& str);
void ToSentenceCase(std::wstring& str);

void EscapeRegExp(std::wstring& str);

void EncodeUrl(std::wstring& str, bool doReserved = true);

bool MatchAndSkipPrefix(const WCHAR** str, const WCHAR* end, const WCHAR* prefix);

/*
** Case insensitive find function for std::string and std::wstring.
**
** Modified from http://stackoverflow.com/questions/3152241/case-insensitive-stdstring-find#3152296
*/
std::size_t CaseInsensitiveFind(const std::wstring& str1, const std::wstring& str2);

}  // namespace StringUtil

#endif
