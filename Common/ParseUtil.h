/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_COMMON_PARSEUTIL_H_
#define RM_COMMON_PARSEUTIL_H_

#include <Windows.h>
#include <cstdint>
#include <d2d1.h>
#include <string>
#include <vector>

enum class PairedPunctuation : BYTE
{
	SingleQuote,
	DoubleQuote,
	BothQuotes,
	Parentheses,
	Brackets,
	Braces,
	Guillemet
};

namespace ParseUtil {

using FormulaErrorCallback = void (*)(const WCHAR* error, const WCHAR* formula);

// If the given string is invalid format or causes overflow/underflow, returns given default value.
double ParseDouble(LPCTSTR str, double defValue, FormulaErrorCallback errorCallback = nullptr);
int ParseInt(LPCTSTR str, int defValue, FormulaErrorCallback errorCallback = nullptr);
uint32_t ParseUInt(LPCTSTR str, uint32_t defValue, FormulaErrorCallback errorCallback = nullptr);
uint64_t ParseUInt64(LPCTSTR str, uint64_t defValue, FormulaErrorCallback errorCallback = nullptr);

// Expects three or four comma separated values or one hex-value.
D2D1_COLOR_F ParseColor(LPCTSTR str, FormulaErrorCallback errorCallback = nullptr);

// Expects four comma separated values (X/Y/Width/Height).
D2D1_RECT_F ParseRect(LPCTSTR str, FormulaErrorCallback errorCallback = nullptr);

// Expects four comma separated values (left/top/right/bottom).
RECT ParseRECT(LPCTSTR str, FormulaErrorCallback errorCallback = nullptr);

// Splits the string from the delimiters and trims empty elements and whitespace.
std::vector<std::wstring> Tokenize(const std::wstring& str, const std::wstring& delimiters);

// Similar to Tokenize, but skips delimiters inside of the defined paired punctuation.
std::vector<std::wstring> TokenizeWithPairedPunctuation(const std::wstring& str, const WCHAR delimiter, const PairedPunctuation punct);

}  // namespace ParseUtil

#endif
