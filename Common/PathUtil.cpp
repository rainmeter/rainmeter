/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "PathUtil.h"

namespace PathUtil {

bool IsSeparator(WCHAR ch)
{
	return ch == L'\\' || ch == L'/';
}

bool IsDotOrDotDot(const WCHAR* path)
{
	return path[0] == L'.' && (path[1] == L'\0' || (path[1] == L'.' && path[2] == L'\0'));
}

bool IsUNC(const std::wstring& path)
{
	return path.length() >= 2 && IsSeparator(path[0]) && IsSeparator(path[1]);
}

bool IsAbsolute(const std::wstring& path)
{
	return (path.find(L":\\") != std::wstring::npos ||
		path.find(L":/") != std::wstring::npos ||
		IsUNC(path));
}

void AppendBackslashIfMissing(std::wstring& path)
{
	if (!path.empty() && !IsSeparator(path[path.length() - 1]))
	{
		path += L'\\';
	}
}

void RemoveLeadingBackslash(std::wstring& path)
{
	if (!path.empty() && IsSeparator(path[0]))
	{
		path.erase(path.begin());
	}
}

void RemoveTrailingBackslash(std::wstring& path)
{
	if (!path.empty() && IsSeparator(path[path.length() - 1]))
	{
		path.pop_back();
	}
}

void RemoveLeadingAndTrailingBackslash(std::wstring& path)
{
	RemoveLeadingBackslash(path);
	RemoveTrailingBackslash(path);
}

std::wstring GetFolderFromFilePath(const std::wstring& filePath)
{
	std::wstring::size_type pos = filePath.find_last_of(L"\\/");
	if (pos != std::wstring::npos)
	{
		return filePath.substr(0, pos + 1);
	}
	return L".\\";
}

/*
** Extracts volume path from program path.
** E.g.:
**   "C:\path\" to "C:"
**   "\\server\share\" to "\\server\share"
**   "\\server\C:\path\" to "\\server\C:"
*/
std::wstring GetVolume(const std::wstring& path)
{
	std::wstring::size_type pos;
	if ((pos = path.find_first_of(L':')) != std::wstring::npos)
	{
		return path.substr(0, pos + 1);
	}
	else if (IsUNC(path))
	{
		if ((pos = path.find_first_of(L"\\/", 2)) != std::wstring::npos)
		{
			std::wstring::size_type pos2;
			if ((pos2 = path.find_first_of(L"\\/", pos + 1)) != std::wstring::npos ||
				pos != (path.length() - 1))
			{
				pos = pos2;
			}
		}

		return path.substr(0, pos);
	}

	return std::wstring();
}

void ExpandEnvironmentVariables(std::wstring& path)
{
	std::wstring::size_type pos;
	if ((pos = path.find(L'%')) != std::wstring::npos &&
		path.find(L'%', pos + 2) != std::wstring::npos)
	{
		DWORD bufSize = 4096;
		WCHAR* buffer = new WCHAR[bufSize];

		// %APPDATA% is a special case.
		pos = path.find(L"%APPDATA%", pos);
		if (pos != std::wstring::npos)
		{
			HRESULT hr = SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, buffer);
			if (SUCCEEDED(hr))
			{
				size_t len = wcslen(buffer);
				do
				{
					path.replace(pos, 9, buffer, len);
				}
				while ((pos = path.find(L"%APPDATA%", pos + len)) != std::wstring::npos);
			}
		}

		if ((pos = path.find(L'%')) != std::wstring::npos &&
			path.find(L'%', pos + 2) != std::wstring::npos)
		{
			// Expand the environment variables.
			do
			{
				DWORD ret = ExpandEnvironmentStrings(path.c_str(), buffer, bufSize);
				if (ret == 0)  // Error
				{
					break;
				}
				if (ret <= bufSize)  // Fits in the buffer
				{
					path.assign(buffer, ret - 1);
					break;
				}

				delete [] buffer;
				bufSize = ret;
				buffer = new WCHAR[bufSize];
			}
			while (true);
		}

		delete [] buffer;
	}
}

}  // namespace PathUtil
