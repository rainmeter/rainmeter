// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>
#include <cstdint>
#include <d2d1.h>
#include <string>
#include <string_view>

class MathParser;

namespace ParseUtil {

using FormulaErrorCallback = void (*)(const WCHAR* error, const WCHAR* formula);

// If the given string is invalid format or causes overflow/underflow, returns given default value.
double ParseDouble(LPCTSTR str, double defValue, const MathParser& mathParser, FormulaErrorCallback errorCallback = nullptr);
int ParseInt(LPCTSTR str, int defValue, const MathParser& mathParser, FormulaErrorCallback errorCallback = nullptr);

// As above, for a value that is not null terminated.
double ParseDouble(std::wstring_view str, double defValue, const MathParser& mathParser, FormulaErrorCallback errorCallback = nullptr);
int ParseInt(std::wstring_view str, int defValue, const MathParser& mathParser, FormulaErrorCallback errorCallback = nullptr);

uint32_t ParseUInt(LPCTSTR str, uint32_t defValue, const MathParser& mathParser, FormulaErrorCallback errorCallback = nullptr);
uint64_t ParseUInt64(LPCTSTR str, uint64_t defValue, const MathParser& mathParser, FormulaErrorCallback errorCallback = nullptr);

// Expects three or four comma separated values or one hex-value.
D2D1_COLOR_F ParseColor(LPCTSTR str, const MathParser& mathParser, FormulaErrorCallback errorCallback = nullptr);
D2D1_COLOR_F ParseColor(std::wstring_view str, const MathParser& mathParser, FormulaErrorCallback errorCallback = nullptr);

// Expects four comma separated values (X/Y/Width/Height).
D2D1_RECT_F ParseRect(LPCTSTR str, const MathParser& mathParser, FormulaErrorCallback errorCallback = nullptr);

// Expects four comma separated values (left/top/right/bottom).
RECT ParseRECT(LPCTSTR str, const MathParser& mathParser, FormulaErrorCallback errorCallback = nullptr);

}  // namespace ParseUtil
