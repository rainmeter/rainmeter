/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __ERROR_H__
#define __ERROR_H__

#include <windows.h>
#include <string>

class CError
{
public:
	CError(const std::wstring& String) : m_String(String) {}
	CError(const WCHAR* String) : m_String(String) {}

	const std::wstring& GetString() { return m_String; }

private:
	std::wstring m_String;
};

#endif
