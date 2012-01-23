/*
  Copyright (C) 2001 Kimmo Pekkola

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
