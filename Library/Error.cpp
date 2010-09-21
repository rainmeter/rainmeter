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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "StdAfx.h"
#include "Error.h"

const WCHAR* CError::c_ErrorStrings[] = 
{
	L"User defined error",
	L"Out of memory",
	L"Null parameter",
	L"Unable to register windowclass",
	L"Unable to create window"
};

/* 
** GetString
**
** Returns the error string
**
*/
const std::wstring& CError::GetString()
{
	static WCHAR Buffer[16];

	if (m_Error != ERROR_USER) 
	{
		m_String = c_ErrorStrings[m_Error];
//		if (m_File) 
//		{
//			swprintf(Buffer, L"%i", m_Line);
//
//			m_String += L"\n(";
//			m_String += m_File;
//			m_String += L" : ";
//			m_String += Buffer;
//			m_String += L")";
//		}
	}

	return m_String; 
}
