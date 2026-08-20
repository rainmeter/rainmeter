// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "LocaleUtil.h"

namespace LocaleUtil {

namespace {

bool g_RefreshNumberFormat = true;

// Some locales use a non-breaking space as their group separator, and a value may have been
// written with an ordinary space instead (or the other way around), so the three are treated as
// the same separator.
bool IsSpaceLike(WCHAR ch)
{
	return ch == L' ' || ch == L'\x00A0' || ch == L'\x202F';
}

bool MatchesSeparator(const WCHAR* str, const WCHAR* separator)
{
	if (*separator == L'\0') return false;

	for ( ; *separator; ++str, ++separator)
	{
		if (*str == *separator) continue;
		if (IsSpaceLike(*str) && IsSpaceLike(*separator)) continue;

		return false;
	}

	return true;
}

}  // namespace

double StringToNumber(const WCHAR* str, NumberFormat format)
{
	if (!str) return 0.0;
	if (format == NumberFormat::Default) return wcstod(str, nullptr);

	static WCHAR s_Decimal[4] = L".";
	static WCHAR s_Group[4] = L"";

	if (g_RefreshNumberFormat)
	{
		g_RefreshNumberFormat = false;
		GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SDECIMAL, s_Decimal, _countof(s_Decimal));
		GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_STHOUSAND, s_Group, _countof(s_Group));
	}

	// wcstod only understands a period and no group separator, so the start of the string is
	// rewritten into that form. Whatever follows the number is left as it is, since wcstod stops
	// at the first character that is not part of a number anyway.
	WCHAR buffer[64];
	size_t length = 0;

	while (*str && length < _countof(buffer) - 1)
	{
		if (MatchesSeparator(str, s_Decimal))
		{
			buffer[length++] = L'.';
			str += wcslen(s_Decimal);
		}
		else if (MatchesSeparator(str, s_Group))
		{
			str += wcslen(s_Group);
		}
		else
		{
			buffer[length++] = *str++;
		}
	}

	buffer[length] = L'\0';

	return wcstod(buffer, nullptr);
}

void RefreshNumberFormat()
{
	g_RefreshNumberFormat = true;
}

}  // namespace LocaleUtil
