// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <string>
#include <memory>

namespace FileUtil {

enum class Encoding : BYTE
{
	ANSI,
	UTF8,
	UTF16LE
};

Encoding GetEncoding(const BYTE* buffer, const size_t& size);

std::unique_ptr<BYTE[]> ReadFullFile(const std::wstring& path, size_t* size = nullptr);

// Reads |path| as text. The BOM of the file determines the encoding, and a file without one is
// read as UTF-8. Both functions below open the file in text mode, so a CRLF is read back as a
// lone LF and an LF is written out as a CRLF.
bool ReadTextFile(const std::wstring& path, std::wstring& text);

// Writes |text| to |path| in |encoding|, prepending a BOM for the Unicode encodings.
bool WriteTextFile(const std::wstring& path, std::wstring_view text, Encoding encoding);

bool GetBinaryFileBitness(const WCHAR* path, WORD& bitness);

void SetFilePinnedAttribute(const WCHAR* path);

} // namespace FileUtil
