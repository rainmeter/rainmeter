/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "FileUtil.h"

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

	hMapping = CreateFileMapping(hFile, nullptr, PAGE_READONLY | SEC_IMAGE, 0UL, 0UL, nullptr);
	if (!hMapping || hMapping == INVALID_HANDLE_VALUE) return cleanUp(false);

	addrMapping = MapViewOfFile(hMapping, FILE_MAP_READ, 0UL, 0UL, 0UL);
	if (!addrMapping) return cleanUp(false);

	PIMAGE_NT_HEADERS pHeader = ImageNtHeader(addrMapping);
	if (!pHeader) return cleanUp(false);

	bitness = pHeader->FileHeader.Machine;

	return cleanUp(true);
}

} // namespace FileUtil
