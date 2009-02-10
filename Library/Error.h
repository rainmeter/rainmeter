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
  $Header: /home/cvsroot/Rainmeter/Library/Error.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: Error.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.3  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.2  2001/10/14 07:30:04  rainy
  Changed from a static storage to a real class.

  Revision 1.1.1.1  2001/08/11 10:58:19  Rainy
  Added to CVS.

*/

#ifndef __ERROR_H__
#define __ERROR_H__

#include <windows.h>
#include <string>

class CError
{
public:
	// Few predefined errors
	enum RAINERROR 
	{
		ERROR_USER,
		ERROR_OUT_OF_MEM,
		ERROR_NULL_PARAMETER,
		ERROR_REGISTER_WINDOWCLASS,
		ERROR_CREATE_WINDOW
	};

    CError(const std::wstring& String) { m_Error = ERROR_USER; m_String = String; m_File = NULL; };
    CError(const WCHAR* String ) { m_Error = ERROR_USER; m_String = String; m_File = NULL; };
    CError(const std::wstring& String, int Line, const char* File) { m_Error = ERROR_USER; m_String = String; m_Line = Line; m_File = File; };
    CError(const WCHAR* String, int Line, const char* File) { m_Error = ERROR_USER; m_String = String; m_Line = Line; m_File = File; };
    CError(RAINERROR Error) { m_Error = Error; m_File = NULL; };
    CError(RAINERROR Error, int Line, const char* File) { m_Error = Error; m_Line = Line; m_File = File; };

    const std::wstring& GetString(); 

private:
	std::wstring m_String;
	int m_Line;
	const char* m_File;
	RAINERROR m_Error;

	static const WCHAR* c_ErrorStrings[];
};

#endif
