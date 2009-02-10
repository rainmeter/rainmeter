/*
  Copyright (C) 2002 Kimmo Pekkola + few lsapi developers

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
  $Header: /home/cvsroot/Rainmeter/Library/Litestep.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: Litestep.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.5  2004/07/11 17:10:01  rainy
  Added ResetLoggingFlag.

  Revision 1.4  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.3  2003/03/22 20:39:20  rainy
  LsLog is exported.

  Revision 1.2  2003/02/10 18:12:17  rainy
  Added LSLog and LSSetVariable

  Revision 1.1  2002/07/01 15:35:54  rainy
  Intial version

*/

#ifndef __LITESTEP_H__
#define __LITESTEP_H__

#include <windows.h>
#include <comdef.h>
#include <string>
#include "Export.h"

#define magicDWord             0x49474541
#define LM_GETREVID            9265
#define LM_REGISTERMESSAGE     9263
#define LM_UNREGISTERMESSAGE   9264

#ifdef _DEBUG
#define DEBUGLOG DebugLog
#else
#define DEBUGLOG //
#endif


typedef void (BangCommand)(HWND sender, LPCSTR args);

// Call this if you want to use lsapi.dll's functions instead of stubs
void InitalizeLitestep();

// The stubs
BOOL AddBangCommand(LPCSTR command, BangCommand f);
HRGN BitmapToRegion(HBITMAP hBmp, COLORREF cTransparentColor, COLORREF cTolerance, int xoffset, int yoffset);
HWND GetLitestepWnd(void);
BOOL GetRCString(LPCSTR lpKeyName, LPSTR value, LPCSTR defStr, int maxLen);
int GetRCInt(LPCSTR lpKeyName, int nDefault);
HINSTANCE LSExecute(HWND Owner, LPCTSTR szCommand, int nShowCmd);
BOOL RemoveBangCommand(LPCSTR command);
void TransparentBltLS (HDC dc, int nXDest, int nYDest, int nWidth, int nHeight, HDC tempDC, int nXSrc, int nYSrc, COLORREF colorTransparent);
void VarExpansion(LPSTR buffer, LPCSTR value);
void LSSetVariable(const BSTR name, const BSTR value);
void DebugLog(const WCHAR* message, ... );

void ResetLoggingFlag();

std::string ConvertToAscii(LPCTSTR str);
std::wstring ConvertToWide(LPCSTR str);

#endif