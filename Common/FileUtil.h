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

bool GetBinaryFileBitness(const WCHAR* path, WORD& bitness);

void SetFilePinnedAttribute(const WCHAR* path);

} // namespace FileUtil
