/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_COMMON_PATHUTIL_H_
#define RM_COMMON_PATHUTIL_H_

#include <Windows.h>
#include <string>

namespace PathUtil {

bool IsSeparator(WCHAR ch);

bool IsDotOrDotDot(const WCHAR* path);

bool IsUNC(const std::wstring& path);

bool IsAbsolute(const std::wstring& path);

void AppendBacklashIfMissing(std::wstring& path);

std::wstring GetFolderFromFilePath(const std::wstring& filePath);

std::wstring GetVolume(const std::wstring& path);

void ExpandEnvironmentVariables(std::wstring& strPath);

}  // namespace PathUtil

#endif
