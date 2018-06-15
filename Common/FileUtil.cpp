/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "FileUtil.h"

namespace FileUtil {

Encoding GetEncoding(const BYTE* buffer, const size_t& size)
{
	if (size >= 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF) return Encoding::UTF8;
	if (size >= 2 && buffer[0] == 0xFF && buffer[1] == 0xFE)                      return Encoding::UTF16LE;

	return Encoding::ANSI;
}

std::unique_ptr<BYTE[]> ReadFullFile(const std::wstring& path, size_t* size)
{
	FILE* file = _wfopen(path.c_str(), L"rb");
	if (!file)
	{
		return nullptr;
	}

	fseek(file, 0, SEEK_END);
	size_t fileSize = ftell(file);
	rewind(file);

	std::unique_ptr<BYTE[]> buffer(new (std::nothrow) BYTE[fileSize + 3]);
	if (buffer)
	{
		fread(buffer.get(), 1, fileSize, file);

		// Triple null terminate the buffer.
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

} // namespace FileUtil
