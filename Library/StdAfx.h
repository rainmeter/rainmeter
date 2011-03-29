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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef __STDAFX_H__
#define __STDAFX_H__

#define _WIN32_IE 0x0600

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

// WINAPI
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <gdiplus.h>
#include <comdef.h>
#include <Iphlpapi.h>
#include <commctrl.h>
#include <shellapi.h>
#include <Mmsystem.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <Wininet.h>
#include <winperf.h>
#include <Windns.h>
#include <Ipexport.h>
#include <Powrprof.h>

// STL
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>

// RUNTIME
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <io.h>
#include <stdarg.h>
#include <process.h>
#include <float.h>

#endif
