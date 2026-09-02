// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "IniFileReader.h"
#include "FileUtil.h"
#include "StringUtil.h"

namespace {

enum class Encoding
{
	ANSI,
	UTF16,
	Unreadable
};

Encoding DetectEncoding(const BYTE* data, size_t size)
{
	// A UTF-8 BOM makes the whole file invisible to the PrivateProfile API.
	if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) return Encoding::Unreadable;
	if (size >= 2 && data[0] == 0xFE && data[1] == 0xFF) return Encoding::Unreadable;

	const bool hasBom = (size >= 2 && data[0] == 0xFF && data[1] == 0xFE);

	// UTF-16LE is detected by content, so a file without a BOM reads exactly like one with it.
	int flags = IS_TEXT_UNICODE_UNICODE_MASK;
	if (hasBom || (size >= 2 && IsTextUnicode(data, (int)std::min<size_t>(size, INT_MAX), &flags)))
	{
		// An odd byte count leaves half a code unit dangling, and the file is rejected outright.
		return (size % 2 == 0) ? Encoding::UTF16 : Encoding::Unreadable;
	}

	return Encoding::ANSI;
}

std::unique_ptr<WCHAR[]> Decode(const BYTE* data, size_t size, size_t& textLength)
{
	textLength = 0;
	if (!data || size == 0) return nullptr;

	const Encoding encoding = DetectEncoding(data, size);
	if (encoding == Encoding::Unreadable) return nullptr;

	if (encoding == Encoding::UTF16)
	{
		const size_t offset = (data[0] == 0xFF && data[1] == 0xFE) ? 2 : 0;
		const size_t count = (size - offset) / 2;
		if (count == 0) return nullptr;

		std::unique_ptr<WCHAR[]> text(new (std::nothrow) WCHAR[count]);
		if (!text) return nullptr;

		memcpy(text.get(), data + offset, count * sizeof(WCHAR));
		textLength = count;
		return text;
	}

	// No MB_ERR_INVALID_CHARS, which is what the API does: the bytes CP1252 leaves undefined
	// become C1 controls rather than U+FFFD. No codepage can produce more characters than it is
	// given bytes, so one pass into a buffer of |size| is both safe and enough.
	std::unique_ptr<WCHAR[]> text(new (std::nothrow) WCHAR[size]);
	if (!text) return nullptr;

	const int count = MultiByteToWideChar(CP_ACP, 0, (const char*)data, (int)size, text.get(), (int)size);
	if (count <= 0) return nullptr;

	textLength = (size_t)count;
	return text;
}

}  // namespace

namespace IniFileReader {

DecodedText DecodedText::FromMemory(const BYTE* data, size_t size)
{
	size_t length = 0;
	std::unique_ptr<WCHAR[]> text = Decode(data, size, length);
	return DecodedText(std::move(text), length);
}

std::optional<DecodedText> DecodeFile(const std::wstring& path)
{
	size_t size = 0;
	const std::unique_ptr<BYTE[]> data = FileUtil::ReadFullFile(path, &size);
	if (!data) return std::nullopt;

	return DecodedText::FromMemory(data.get(), size);
}

// Whitespace is every character at or below 0x20 and nothing else, so U+00A0 and U+3000 survive.
std::wstring_view DecodedText::Trim(std::wstring_view str)
{
	size_t begin = 0;
	while (begin < str.length() && str[begin] <= L' ') ++begin;

	size_t end = str.length();
	while (end > begin && str[end - 1] <= L' ') --end;

	return str.substr(begin, end - begin);
}

	// One surrounding pair of double or single quotes, removed from every value.
std::wstring_view DecodedText::StripQuotes(std::wstring_view value)
{
	return StringUtil::StripLeadingAndTrailingQuotes(value, true);
}

}  // namespace IniFileReader
