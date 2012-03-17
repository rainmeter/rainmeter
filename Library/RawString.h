/*
  Copyright (C) 2011 Birunthan Mohanathas

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

#ifndef __RAWSTRING_H__
#define __RAWSTRING_H__

#include <windows.h>

class CRawString
{
public:
	CRawString() :
		m_String()
	{
	}

	CRawString(const WCHAR* str) :
		m_String(str_alloc(str))
	{
	}

	CRawString(const CRawString& rhs) :
		m_String(str_alloc(rhs.c_str()))
	{
	}

	~CRawString()
	{
		clear();
	}
	
	CRawString& operator=(const WCHAR* rhs)
	{
		clear();
		m_String = str_alloc(rhs);
		return *this;
	}

	CRawString& operator=(const CRawString& rhs)
	{
		if (&rhs != this)
		{
			clear();
			m_String = str_alloc(rhs.m_String);
		}
		return *this;
	}

	const WCHAR* c_str() const
	{
		return m_String ? m_String : L"";
	}

	bool empty() const
	{
		return !m_String || !(*m_String);
	}

	void clear()
	{
		if (m_String)
		{
			free(m_String);
			m_String = NULL;
		}
	}

private:
	WCHAR* str_alloc(const WCHAR* str)
	{
		return str ? _wcsdup(str) : NULL;
	}

	WCHAR* m_String;
};

#endif
