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

HINSTANCE LSExecuteAsAdmin(HWND Owner, LPCTSTR szCommand, int nShowCmd);
HINSTANCE ExecuteCommand(HWND Owner, LPCTSTR szCommand, int nShowCmd, LPCTSTR szVerb);

#endif