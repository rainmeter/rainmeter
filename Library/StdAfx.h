/* Copyright (C) 2009 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __STDAFX_H__
#define __STDAFX_H__

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

// WINAPI
#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <ole2.h>  // For Gdiplus.h.
#include <gdiplus.h>
#include <dwmapi.h>
#include <comdef.h>
#include <Iphlpapi.h>
#include <Imagehlp.h>
#include <Mmsystem.h>
#include <Shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <Wininet.h>
#include <VersionHelpers.h>

// STL
#include <map>
#include <set>
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
