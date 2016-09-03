/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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
