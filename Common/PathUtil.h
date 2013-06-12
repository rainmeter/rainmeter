/*
  Copyright (C) 2013 Rainmeter Team

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

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
