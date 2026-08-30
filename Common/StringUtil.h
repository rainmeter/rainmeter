// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>
#include <string>
#include <string_view>

#include "StringBuffer.h"

namespace StringUtil {

std::string Narrow(const WCHAR* str, int strLen = -1, int cp = CP_ACP);
inline std::string Narrow(const std::wstring& str, int cp = CP_ACP) { return Narrow(str.c_str(), (int)str.length(), cp); }

inline std::string NarrowUTF8(const WCHAR* str, int strLen = -1) { return Narrow(str, strLen, CP_UTF8); }
inline std::string NarrowUTF8(const std::wstring& str) { return Narrow(str.c_str(), (int)str.length(), CP_UTF8); }

std::wstring Widen(const char* str, int strLen = -1, int cp = CP_ACP);
inline std::wstring Widen(const std::string& str, int cp = CP_ACP) { return Widen(str.c_str(), (int)str.length(), cp); }

inline std::wstring WidenUTF8(const char* str, int strLen = -1) { return Widen(str, strLen, CP_UTF8); }
inline std::wstring WidenUTF8(const std::string& str) { return Widen(str.c_str(), (int)str.length(), CP_UTF8); }

// These convert into a caller-provided buffer, which avoids the heap allocation entirely as long
// as the result fits inline. They also convert straight into the buffer instead of measuring the
// result first, so the common case walks the input only once.
template<size_t N>
void Narrow(const WCHAR* str, int strLen, int cp, StringBuffer<char, N>& out)
{
	if (!str || strLen == 0 || !*str)
	{
		out.SetLength(0);
		return;
	}

	if (strLen == -1) strLen = (int)wcslen(str);

	char* buffer = out.Reserve(out.capacity());
	int bufLen = WideCharToMultiByte(cp, 0, str, strLen, buffer, (int)out.capacity(), nullptr, nullptr);
	if (bufLen == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		bufLen = WideCharToMultiByte(cp, 0, str, strLen, nullptr, 0, nullptr, nullptr);
		buffer = bufLen > 0 ? out.Reserve((size_t)bufLen) : nullptr;
		bufLen = buffer ? WideCharToMultiByte(cp, 0, str, strLen, buffer, bufLen, nullptr, nullptr) : 0;
	}

	out.SetLength(bufLen > 0 ? (size_t)bufLen : 0);
}

template<size_t N>
void NarrowUTF8(const WCHAR* str, int strLen, StringBuffer<char, N>& out) { Narrow(str, strLen, CP_UTF8, out); }

template<size_t N>
void Widen(const char* str, int strLen, int cp, StringBuffer<WCHAR, N>& out)
{
	if (!str || strLen == 0 || !*str)
	{
		out.SetLength(0);
		return;
	}

	if (strLen == -1) strLen = (int)strlen(str);

	WCHAR* buffer = out.Reserve(out.capacity());
	int bufLen = MultiByteToWideChar(cp, 0, str, strLen, buffer, (int)out.capacity());
	if (bufLen == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		bufLen = MultiByteToWideChar(cp, 0, str, strLen, nullptr, 0);
		buffer = bufLen > 0 ? out.Reserve((size_t)bufLen) : nullptr;
		bufLen = buffer ? MultiByteToWideChar(cp, 0, str, strLen, buffer, bufLen) : 0;
	}

	out.SetLength(bufLen > 0 ? (size_t)bufLen : 0);
}

template<size_t N>
void WidenUTF8(const char* str, int strLen, StringBuffer<WCHAR, N>& out) { Widen(str, strLen, CP_UTF8, out); }

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
