// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "FileUtil.h"
#include "StringUtil.h"

#include <Imagehlp.h>

namespace FileUtil {

Encoding GetEncoding(const BYTE* buffer, const size_t& size)
{
	if (size >= 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF) return Encoding::UTF8;
	if (size >= 2 && buffer[0] == 0xFF && buffer[1] == 0xFE)                      return Encoding::UTF16LE;

	return Encoding::ANSI;
}

// Reads and allocates memory for file in path. Returns unique_ptr containing address to allocated memory.
std::unique_ptr<BYTE[]> ReadFullFile(const std::wstring& path, size_t* size)
{
	FILE* file;
	if (_wfopen_s(&file, path.c_str(), L"rb") != 0)
	{
		return nullptr;
	}

	fseek(file, 0L, SEEK_END);
	size_t fileSize = ftell(file);
	fseek(file, 0L, SEEK_SET);

	std::unique_ptr<BYTE[]> buffer(new (std::nothrow) BYTE[fileSize + 3]);
	if (buffer)
	{
		fread_s(buffer.get(), fileSize + 3, 1, fileSize, file);

		// Triple null terminate the buffer
		buffer[fileSize] = 0;
		buffer[fileSize + 1] = 0;
		buffer[fileSize + 2] = 0;
	}
	fclose(file);

	if (size)
	{
		*size = fileSize;
	}

	return buffer;
}

bool ReadTextFile(const std::wstring& path, std::wstring& text)
{
	// The BOM of an existing file overrides the encoding given here, so this is the encoding of a
	// file which does not have one.
	FILE* file;
	if (_wfopen_s(&file, path.c_str(), L"r, ccs=UTF-8") != 0)
	{
		return false;
	}

	WCHAR buffer[4096];
	while (fgetws(buffer, _countof(buffer), file))
	{
		text += buffer;
	}

	const bool result = ferror(file) == 0;
	fclose(file);

	return result;
}

// A text mode stream turns an LF into a CRLF, which would leave a CR of its own in front of any
// CRLF the text already has. Reduce the line endings to LF so that each ends up as one CRLF.
static std::wstring ToLineFeeds(const std::wstring& text)
{
	std::wstring result;
	result.reserve(text.length());

	for (size_t i = 0, length = text.length(); i < length; ++i)
	{
		if (text[i] == L'\r' && i + 1 < length && text[i + 1] == L'\n') continue;

		result += text[i];
	}

	return result;
}

bool WriteTextFile(const std::wstring& path, const std::wstring& text, Encoding encoding)
{
	std::wstring mode = L"w";
	switch (encoding)
	{
	case Encoding::UTF8:
		mode += L", ccs=UTF-8";
		break;

	case Encoding::UTF16LE:
		mode += L", ccs=UTF-16LE";
		break;
	}

	FILE* file;
	if (_wfopen_s(&file, path.c_str(), mode.c_str()) != 0)
	{
		return false;
	}

	const std::wstring lines = ToLineFeeds(text);

	// A stream without a ccs is byte oriented, and fputws() would convert the text through the
	// locale of the CRT rather than the active code page, which is only set for LC_NUMERIC here.
	const bool result = (encoding == Encoding::ANSI) ?
		fputs(StringUtil::Narrow(lines).c_str(), file) >= 0 :
		fputws(lines.c_str(), file) >= 0;

	fclose(file);

	return result;
}

bool GetBinaryFileBitness(const WCHAR* path, WORD& bitness)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hMapping = INVALID_HANDLE_VALUE;
	LPVOID addrMapping = nullptr;

	auto cleanUp = [&](bool ret) -> bool
	{
		if (addrMapping)
		{
			UnmapViewOfFile(addrMapping);
			addrMapping = nullptr;
		}

		if (hMapping)
		{
			CloseHandle(hMapping);
			hMapping = nullptr;
		}

		if (hFile)
		{
			CloseHandle(hFile);
			hFile = nullptr;
		}

		return ret;
	};

	hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, nullptr);
	if (!hFile || hFile == INVALID_HANDLE_VALUE) return cleanUp(false);

	hMapping = CreateFileMapping(hFile, nullptr, PAGE_READONLY | SEC_IMAGE, 0, 0, nullptr);
	if (!hMapping || hMapping == INVALID_HANDLE_VALUE) return cleanUp(false);

	addrMapping = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
	if (!addrMapping) return cleanUp(false);

	PIMAGE_NT_HEADERS pHeader = ImageNtHeader(addrMapping);
	if (!pHeader) return cleanUp(false);

	bitness = pHeader->FileHeader.Machine;

	return cleanUp(true);
}

void SetFilePinnedAttribute(const WCHAR* path)
{
	DWORD attrs = GetFileAttributes(path);
	if (attrs == INVALID_FILE_ATTRIBUTES) return;

	attrs |= FILE_ATTRIBUTE_PINNED;
	attrs &= ~FILE_ATTRIBUTE_UNPINNED;
	SetFileAttributes(path, attrs);
}

} // namespace FileUtil
