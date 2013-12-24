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

#ifndef RM_COMMON_RAWSTRING_H_
#define RM_COMMON_RAWSTRING_H_

#include <malloc.h>

class RawString
{
public:
	RawString() :
		m_String()
	{
	}

	RawString(const wchar_t* str) :
		m_String(str_alloc(str))
	{
	}

	RawString(const RawString& rhs) :
		m_String(str_alloc(rhs.c_str()))
	{
	}

	~RawString()
	{
		clear();
	}
	
	RawString& operator=(const wchar_t* rhs)
	{
		clear();
		m_String = str_alloc(rhs);
		return *this;
	}

	RawString& operator=(const RawString& rhs)
	{
		if (&rhs != this)
		{
			clear();
			m_String = str_alloc(rhs.m_String);
		}
		return *this;
	}

	const wchar_t* c_str() const
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
			m_String = nullptr;
		}
	}

private:
	wchar_t* str_alloc(const wchar_t* str)
	{
		return str ? _wcsdup(str) : nullptr;
	}

	wchar_t* m_String;
};

#endif
