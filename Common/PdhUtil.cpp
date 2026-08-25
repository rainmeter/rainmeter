// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "PdhUtil.h"
#include <pdhmsg.h>

namespace PdhUtil {

bool IsValidStatus(DWORD status)
{
	return status == PDH_CSTATUS_VALID_DATA || status == PDH_CSTATUS_NEW_DATA;
}

// Lets PDH build the "\Object(Instance)\Counter" path, which matters since object and counter
// names may contain the path separators themselves
static bool MakePath(const std::wstring& object, const std::wstring& counter, const WCHAR* instance,
	WCHAR (&path)[PDH_MAX_COUNTER_PATH])
{
	PDH_COUNTER_PATH_ELEMENTS elements = { 0 };
	elements.szObjectName = (LPWSTR)object.c_str();
	elements.szInstanceName = (LPWSTR)instance;
	elements.szCounterName = (LPWSTR)counter.c_str();

	DWORD size = _countof(path);
	return PdhMakeCounterPath(&elements, path, &size, 0) == ERROR_SUCCESS;
}

bool AddEnglishCounter(PDH_HQUERY query, const std::wstring& object, const std::wstring& counter,
	const WCHAR* instance, PDH_HCOUNTER* handle)
{
	WCHAR path[PDH_MAX_COUNTER_PATH] = { 0 };
	if (!MakePath(object, counter, instance, path)) return false;

	return PdhAddEnglishCounter(query, path, 0, handle) == ERROR_SUCCESS;
}

bool AddCounter(PDH_HQUERY query, const std::wstring& object, const std::wstring& counter,
	const WCHAR* instance, PDH_HCOUNTER* handle)
{
	if (AddEnglishCounter(query, object, counter, instance, handle)) return true;

	WCHAR path[PDH_MAX_COUNTER_PATH] = { 0 };
	if (!MakePath(object, counter, instance, path)) return false;

	return PdhAddCounter(query, path, 0, handle) == ERROR_SUCCESS;
}

bool GetRawArray(PDH_HCOUNTER counter, std::vector<BYTE>& buffer, DWORD& count)
{
	DWORD size = 0;
	count = 0;
	if (PdhGetRawCounterArray(counter, &size, &count, nullptr) != PDH_MORE_DATA) return false;

	buffer.resize(size);
	return PdhGetRawCounterArray(counter, &size, &count, (PPDH_RAW_COUNTER_ITEM)buffer.data()) == ERROR_SUCCESS;
}

bool GetFormattedArray(PDH_HCOUNTER counter, std::vector<BYTE>& buffer, DWORD& count)
{
	// PDH_FMT_NOCAP100 keeps values such as the processor time of a process that is spread over
	// several cores from being clamped to 100.
	const DWORD format = PDH_FMT_DOUBLE | PDH_FMT_NOCAP100;

	DWORD size = 0;
	count = 0;
	if (PdhGetFormattedCounterArray(counter, format, &size, &count, nullptr) != PDH_MORE_DATA) return false;

	buffer.resize(size);
	return PdhGetFormattedCounterArray(counter, format, &size, &count,
		(PPDH_FMT_COUNTERVALUE_ITEM)buffer.data()) == ERROR_SUCCESS;
}

void MatchValues(const PDH_RAW_COUNTER_ITEM* rawItems, DWORD rawCount,
	const PDH_FMT_COUNTERVALUE_ITEM* items, DWORD count, std::vector<double>& values)
{
	values.assign(rawCount, 0.0);

	DWORD i = 0;
	for (const DWORD shared = min(rawCount, count); i < shared; ++i)
	{
		if (wcscmp(items[i].szName, rawItems[i].szName) != 0) break;

		if (IsValidStatus(items[i].FmtValue.CStatus))
		{
			values[i] = items[i].FmtValue.doubleValue;
		}
	}

	if (i == rawCount) return;

	// The names point into the buffer the caller still owns, so nothing is copied here either
	ankerl::unordered_dense::map<std::wstring_view, double> byName;
	byName.reserve(count);

	for (DWORD j = 0; j < count; ++j)
	{
		if (IsValidStatus(items[j].FmtValue.CStatus))
		{
			byName.insert_or_assign(std::wstring_view(items[j].szName), items[j].FmtValue.doubleValue);
		}
	}

	for ( ; i < rawCount; ++i)
	{
		const auto found = byName.find(std::wstring_view(rawItems[i].szName));
		if (found != byName.end())
		{
			values[i] = found->second;
		}
	}
}

}  // namespace PdhUtil
