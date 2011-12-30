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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
		m_String(_wcsdup(str))
	{
	}

	~CRawString()
	{
		if (m_String) free(m_String);
	}
	
	CRawString& operator=(const WCHAR* rhs)
	{
		if (m_String) free(m_String);
		m_String = _wcsdup(rhs);
		return *this;
	}

	CRawString& operator=(const CRawString& rhs)
	{
		if (&rhs != this)
		{
			if (m_String) free(m_String);
			m_String = _wcsdup(rhs.m_String);
		}
		return *this;
	}

	const WCHAR* c_str() const
	{
		return m_String;
	}

	bool empty() const
	{
		return !(*m_String);
	}

private:
	CRawString(const CRawString& p)
	{
	}

	WCHAR* m_String;
};

#endif
