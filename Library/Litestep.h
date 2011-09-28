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
#include "Error.h"
#include "Export.h"

void InitalizeLitestep();
void FinalizeLitestep();

void ResetLoggingFlag();

HRGN BitmapToRegion(HBITMAP hBmp, COLORREF cTransparentColor, COLORREF cTolerance, int xoffset, int yoffset);

std::string ConvertToAscii(LPCTSTR str);
std::wstring ConvertToWide(LPCSTR str);
std::string ConvertToUTF8(LPCWSTR str);
std::wstring ConvertUTF8ToWide(LPCSTR str);

void Log(int nLevel, const WCHAR* message, const WCHAR* module = L"Rainmeter");		// Wrapper for LSLog().
void LogWithArgs(int nLevel, const WCHAR* format, ... );	// Replacement for DebugLog(), has the same functionality but has the option to set teh Log Level.
void LogError(CError& error);

void RunCommand(HWND Owner, LPCTSTR szCommand, int nShowCmd, bool asAdmin = false);

WCHAR* GetString(UINT id);
std::wstring GetFormattedString(UINT id, ...);

void RmNullCRTInvalidParameterHandler(const wchar_t* expression, const wchar_t* function,  const wchar_t* file, unsigned int line, uintptr_t pReserved);

#endif
