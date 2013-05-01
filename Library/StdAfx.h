/*
  Copyright (C) 2009 Kimmo Pekkola

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

#ifndef __STDAFX_H__
#define __STDAFX_H__

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

// WINAPI
#define WIN32_LEAN_AND_MEAN
#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <gdiplus.h>
#include <dwmapi.h>
#include <comdef.h>
#include <Iphlpapi.h>
#include <Mmsystem.h>
#include <Shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <Wininet.h>

// STL
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <algorithm>
#include <memory>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <cmath>
#include <cerrno>
#include <cassert>
#include <cstdint>

// RUNTIME
#include <process.h>

// ADDITIONAL MACRO
#define IsCtrlKeyDown()     (GetKeyState(VK_CONTROL) < 0)
#define IsShiftKeyDown()    (GetKeyState(VK_SHIFT) < 0)
#define IsAltKeyDown()      (GetKeyState(VK_MENU) < 0)

#endif
