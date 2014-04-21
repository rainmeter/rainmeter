/*
  Copyright (C) 2013 Birunthan Mohanathas

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef RM_COMMON_STRINGUTIL_H_
#define RM_COMMON_STRINGUTIL_H_

#include <Windows.h>
#include <algorithm>
#include <locale>
#include <string>

/*
** Helper class for case insensitive find function.
*/
template<typename CharT>
struct Is_Equal
{
	Is_Equal(const std::locale& loc) : locale(loc) { }
	bool operator()(CharT ch1, CharT ch2) { return std::toupper(ch1, locale) == std::toupper(ch2, locale); }

private:
	const std::locale& locale;
};

namespace StringUtil {

std::string Narrow(const WCHAR* str, int strLen = -1, int cp = CP_ACP);
inline std::string Narrow(const std::wstring& str, int cp = CP_ACP) { return Narrow(str.c_str(), (int)str.length(), cp); }

inline std::string NarrowUTF8(const WCHAR* str, int strLen = -1) { return Narrow(str, strLen, CP_UTF8); }
inline std::string NarrowUTF8(const std::wstring& str) { return Narrow(str.c_str(), (int)str.length(), CP_UTF8); }

std::wstring Widen(const char* str, int strLen = -1, int cp = CP_ACP);
inline std::wstring Widen(const std::string& str, int cp = CP_ACP) { return Widen(str.c_str(), (int)str.length(), cp); }

inline std::wstring WidenUTF8(const char* str, int strLen = -1) { return Widen(str, strLen, CP_UTF8); }
inline std::wstring WidenUTF8(const std::string& str) { return Widen(str.c_str(), (int)str.length(), CP_UTF8); }

void EscapeRegExp(std::wstring& str);

void EncodeUrl(std::wstring& str);


/*
** Case insensitive find function for std::string and std::wstring.
**
** Modified from http://stackoverflow.com/questions/3152241/case-insensitive-stdstring-find#3152296
*/
template<typename T>
std::size_t CaseInsensitiveFind(const T& str1, const T& str2, const std::locale& loc = std::locale())
{
	T::const_iterator iter = std::search(str1.begin(), str1.end(),
		str2.begin(), str2.end(), Is_Equal<T::value_type>(loc));

	if (iter != str1.end())
	{
		return (iter - str1.begin());
	}
	
	return -1; // not found
}
}  // namespace StringUtil

#endif
