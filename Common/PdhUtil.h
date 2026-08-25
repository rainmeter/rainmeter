// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>
#include <pdh.h>
#include <string>
#include <vector>

namespace PdhUtil {

bool IsValidStatus(DWORD status);

bool AddEnglishCounter(PDH_HQUERY query, const std::wstring& object, const std::wstring& counter,
	const WCHAR* instance, PDH_HCOUNTER* handle);

// As above, except that the name is also tried as given, for counters that only exist under a
// localized name.
bool AddCounter(PDH_HQUERY query, const std::wstring& object, const std::wstring& counter,
	const WCHAR* instance, PDH_HCOUNTER* handle);

bool GetRawArray(PDH_HCOUNTER counter, std::vector<BYTE>& buffer, DWORD& count);
bool GetFormattedArray(PDH_HCOUNTER counter, std::vector<BYTE>& buffer, DWORD& count);

// Lines the formatted values up with the raw instances, so that neither has to be looked up by
// name. Both arrays are expanded from the same instance list, which is checked as they are walked;
// only if they ever disagree is a lookup needed, and then only for what is left.
void MatchValues(const PDH_RAW_COUNTER_ITEM* rawItems, DWORD rawCount,
	const PDH_FMT_COUNTERVALUE_ITEM* items, DWORD count, std::vector<double>& values);

}  // namespace PdhUtil
