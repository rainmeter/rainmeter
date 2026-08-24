// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>
#include <string>

namespace PathUtil {

bool IsSeparator(WCHAR ch);

bool IsDotOrDotDot(const WCHAR* path);

bool IsUNC(const std::wstring& path);

bool IsAbsolute(const std::wstring& path);

void AppendBackslashIfMissing(std::wstring& path);

void RemoveLeadingBackslash(std::wstring& path);

void RemoveTrailingBackslash(std::wstring& path);

void RemoveLeadingAndTrailingBackslash(std::wstring& path);

std::wstring GetFolderFromFilePath(const std::wstring& filePath);

std::wstring GetVolume(const std::wstring& path);

void ExpandEnvironmentVariables(std::wstring& strPath, std::wstring::size_type start = 0);

}  // namespace PathUtil
