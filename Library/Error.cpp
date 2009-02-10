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
/*
  $Header: /home/cvsroot/Rainmeter/Library/Error.cpp,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: Error.cpp,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.3  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.2  2001/10/14 07:30:04  rainy
  Changed from a static storage to a real class.

  Revision 1.1.1.1  2001/08/11 10:58:19  Rainy
  Added to CVS.

*/
#pragma warning(disable: 4996)

#include "Error.h"
#include <stdio.h>

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
