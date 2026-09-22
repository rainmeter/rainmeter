// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>
#include <string_view>

namespace DateTimeParser {

enum class MatchMode
{
	// Preserve strptime-style behavior by returning after the format has matched.
	Prefix,

	// Require the remaining input to contain only whitespace.
	Full
};

enum class ParseError
{
	None,
	InvalidLocale,
	InvalidFormat,
	InputMismatch,
	InvalidDate,
	InvalidTimeZone
};

struct ParseResult
{
	// The parsed wall time, adjusted to UTC when the format contains %z or %Z.
	FILETIME timestamp = {};
	size_t consumed = 0;
	ParseError error = ParseError::None;

	explicit operator bool() const { return error == ParseError::None; }
};

// Re-reads the native digits, which change when the user changes their regional settings.
void RefreshNativeDigits();

// This parser is stricter than MSVC std::get_time:
// - Literal characters are case-sensitive.
// - Numeric directives do not accept a sign (+ or -).
// - %S accepts 0 through 59. Leap seconds are rejected because FILETIME cannot represent them.
// - %p requires %I, and repeated or overlapping fields must agree.
// - Calendar dates, ordinal dates, week dates, and weekday names must describe the same date.
// - Unsupported directives and illegal E, O, or # modifier combinations are InvalidFormat errors.
//
// Whitespace in the format, including %n and %t, matches zero or more whitespace characters as
// classified by the selected locale. Full mode applies the same rule to trailing input.
//
// Locale names are accepted when _wcreate_locale accepts them, including legacy CRT spellings. If a
// legacy name cannot be used with GetLocaleInfoEx, %c, %x, and %X use the MSVC C-locale layouts.
// Missing fields default to 1900-01-01 00:00:00. %z and %Z convert the parsed wall time to UTC.
ParseResult Parse(std::wstring_view value, std::wstring_view format, std::wstring_view locale, MatchMode matchMode = MatchMode::Prefix);

}  // namespace DateTimeParser
