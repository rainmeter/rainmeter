/*
  Copyright (C) 2013 Birunthan Mohanathas

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

#include "StringUtil.h"

namespace StringUtil {

std::string Narrow(const WCHAR* str, int strLen, int cp)
{
	std::string narrowStr;

	if (str && *str)
	{
		if (strLen == -1)
		{
			strLen = (int)wcslen(str);
		}

		int bufLen = WideCharToMultiByte(cp, 0, str, strLen, nullptr, 0, nullptr, nullptr);
		if (bufLen > 0)
		{
			narrowStr.resize(bufLen);
			WideCharToMultiByte(cp, 0, str, strLen, &narrowStr[0], bufLen, nullptr, nullptr);
		}
	}
	return narrowStr;
}

std::wstring Widen(const char* str, int strLen, int cp)
{
	std::wstring wideStr;

	if (str && *str)
	{
		if (strLen == -1)
		{
			strLen = strlen(str);
		}

		int bufLen = MultiByteToWideChar(cp, 0, str, strLen, nullptr, 0);
		if (bufLen > 0)
		{
			wideStr.resize(bufLen);
			MultiByteToWideChar(cp, 0, str, strLen, &wideStr[0], bufLen);
		}
	}
	return wideStr;
}

}  // namespace StringUtil
