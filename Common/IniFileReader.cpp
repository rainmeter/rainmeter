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

	// An odd byte count leaves half a code unit dangling, and the file is rejected outright.
	if (size >= 2 && data[0] == 0xFF && data[1] == 0xFE)
	{
		return (size % 2 == 0) ? Encoding::UTF16 : Encoding::Unreadable;
	}

	// UTF-16LE is detected by content too, so a file without a BOM reads exactly like one with it.
	// Both tests below need a 0x00 byte to pass, and ANSI text has none, so neither can claim an
	// ANSI file. IS_TEXT_UNICODE_STATISTICS is left out because it decides from byte frequency
	// alone and does say yes to some plain ASCII. A file wrongly taken for UTF-16 decodes to
	// garbage, which means no sections, no keys, and nothing in the log to say why.
	int flags = IS_TEXT_UNICODE_ASCII16 | IS_TEXT_UNICODE_CONTROLS;
	if (size >= 2 && IsTextUnicode(data, (int)std::min<size_t>(size, INT_MAX), &flags))
	{
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

	DecodedText text = DecodedText::FromMemory(data.get(), size);

	// A file with bytes in it that decodes to nothing was turned down by the encoding detection:
	// a UTF-8 BOM, UTF-16BE, or an odd byte count. Saying so is the whole point -- such a file is
	// otherwise indistinguishable from an empty one, and a skin whose @Include silently
	// contributes nothing is the hardest kind of breakage to find in a log.
	if (size != 0 && text.IsEmpty()) return std::nullopt;

	return std::optional<DecodedText>(std::move(text));
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
