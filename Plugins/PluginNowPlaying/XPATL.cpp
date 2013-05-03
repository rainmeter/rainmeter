/* Copyright (c) 2013 Mike Ryan

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */


// XPSupport ATL Wrappers (for VC2012 Update 2)
// Written by Mike Ryan (aka Ted.)
// http://tedwvc.wordpress.com

// 2013-04-14 1.00 initial release to wrap InitializeCriticalSectionEx
// 2013-04-15 1.01 added x64 asm file (no change to CPP file)
// 2013-04-17 1.02 cleaned up Vista check (was triggering RTCs)

#include "StdAfx.h"

bool Is_VistaOrLater() {
    DWORD version = ::GetVersion();
    DWORD major = (DWORD) (LOBYTE(LOWORD(version)));

    return (major >= 6);
}

typedef BOOL (WINAPI *pInitializeCriticalSectionEx)(__out  LPCRITICAL_SECTION lpCriticalSection, __in   DWORD dwSpinCount, __in   DWORD Flags);

extern "C" BOOL WINAPI VC11Update2InitializeCriticalSectionEx(__out  LPCRITICAL_SECTION lpCriticalSection, __in   DWORD dwSpinCount, __in   DWORD Flags)
{
	static pInitializeCriticalSectionEx InitializeCriticalSectionEx_p = NULL;

	if (Is_VistaOrLater()) { // Vista or higher
		if (!InitializeCriticalSectionEx_p) { 
			HMODULE mod = GetModuleHandle(L"KERNEL32.DLL");
			if (mod) 
				InitializeCriticalSectionEx_p = (pInitializeCriticalSectionEx) GetProcAddress(mod, "InitializeCriticalSectionEx");
		}
		return InitializeCriticalSectionEx_p(lpCriticalSection, dwSpinCount, Flags);
	} 

	// on XP we'll use InitializeCrticialSectionAndSpinCount
	return ::InitializeCriticalSectionAndSpinCount(lpCriticalSection, dwSpinCount);
}
