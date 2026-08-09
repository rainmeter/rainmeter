// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <windows.h>

namespace Locale {

enum class NumberFormat : BYTE
{
	// The format of numbers in skin files, which is never localized: a period for the decimal
	// separator, and no group separator.
	Default,

	// The format of the user's current locale.
	Locale
};

// Converts the number at the start of |str| using the separators of |format|, ignoring anything
// that follows it. Returns 0.0 if |str| does not start with a number.
double StringToNumber(const WCHAR* str, NumberFormat format);

// Re-reads the separators of NumberFormat::Locale, which change when the user changes their
// regional settings.
void RefreshNumberFormat();

}  // namespace Locale
