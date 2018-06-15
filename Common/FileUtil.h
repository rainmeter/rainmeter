/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_COMMON_FILEUTIL_H_
#define RM_COMMON_FILEUTIL_H_

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

/*
** Reads and allocates memory for file in path. Returns unique_ptr containing addres to
** allocated memory.
*/
std::unique_ptr<BYTE[]> ReadFullFile(const std::wstring& path, size_t* size = nullptr);

} // namespace FileUtil

#endif