// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>
#include <string>
#include <string_view>

namespace StringUtil {

std::string Narrow(const WCHAR* str, int strLen = -1, int cp = CP_ACP);
inline std::string Narrow(const std::wstring& str, int cp = CP_ACP) { return Narrow(str.c_str(), (int)str.length(), cp); }

inline std::string NarrowUTF8(const WCHAR* str, int strLen = -1) { return Narrow(str, strLen, CP_UTF8); }
inline std::string NarrowUTF8(const std::wstring& str) { return Narrow(str.c_str(), (int)str.length(), CP_UTF8); }

std::wstring Widen(const char* str, int strLen = -1, int cp = CP_ACP);
inline std::wstring Widen(const std::string& str, int cp = CP_ACP) { return Widen(str.c_str(), (int)str.length(), cp); }

inline std::wstring WidenUTF8(const char* str, int strLen = -1) { return Widen(str, strLen, CP_UTF8); }
inline std::wstring WidenUTF8(const std::string& str) { return Widen(str.c_str(), (int)str.length(), CP_UTF8); }

inline bool EqualsIgnoreCase(std::wstring_view str, std::wstring_view other)
{
	return str.length() == other.length() && _wcsnicmp(str.data(), other.data(), other.length()) == 0;
}

// Removes a leading and trailing pair of double quotes, or of single quotes if |single| is set.
std::wstring_view StripLeadingAndTrailingQuotes(std::wstring_view str, bool single = false);

bool ToUpperCase(std::wstring_view str, WCHAR* dstBuffer, size_t dstCount);

// These convert |count| characters starting at |str| in place. The conversions never change the
// length of the text, so they can be applied to a part of a larger string.
void ToLowerCase(WCHAR* str, size_t count);
void ToUpperCase(WCHAR* str, size_t count);
void ToProperCase(WCHAR* str, size_t count);
void ToSentenceCase(WCHAR* str, size_t count);

inline void ToLowerCase(std::wstring& str) { ToLowerCase(str.data(), str.length()); }
inline void ToUpperCase(std::wstring& str) { ToUpperCase(str.data(), str.length()); }
inline void ToProperCase(std::wstring& str) { ToProperCase(str.data(), str.length()); }
inline void ToSentenceCase(std::wstring& str) { ToSentenceCase(str.data(), str.length()); }

void EscapeRegExp(std::wstring& str);

void EncodeUrl(std::wstring& str, bool doReserved = true);

std::wstring TruncateWithEllipsis(std::wstring_view str, size_t maxLength);

// Case insensitive find function for std::string and std::wstring.
// Modified from http://stackoverflow.com/questions/3152241/case-insensitive-stdstring-find#3152296
std::size_t CaseInsensitiveFind(const std::wstring& str1, const std::wstring& str2);

}  // namespace StringUtil
