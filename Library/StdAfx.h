// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

// WINAPI
#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <comdef.h>
#include <oleidl.h>
#include <Iphlpapi.h>
#include <Mmsystem.h>
#include <Shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <Wininet.h>
#include <VersionHelpers.h>
#include <wrl/client.h>

// STL
#include <array>
#include <map>
#include <set>
#include <deque>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <atomic>
#include <memory>
#include <optional>
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

#include "ankerl/unordered_dense.h"

#include "fmt/base.h"
#include "fmt/xchar.h"

// ADDITIONAL MACRO
#define IsCtrlKeyDown()     (GetKeyState(VK_CONTROL) < 0)
#define IsShiftKeyDown()    (GetKeyState(VK_SHIFT) < 0)
#define IsAltKeyDown()      (GetKeyState(VK_MENU) < 0)
