/*
  Copyright (C) 2014 Rainmeter Team

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

// Common is used by projects that don't link to msvcpNNN.dll at all so this header should include
// only C compatible headers.

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include <Windows.h>
#include <Commctrl.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <dwrite_1.h>
#include <ole2.h>  // For Gdiplus.h.
#include <GdiPlus.h>
#include <Shlobj.h>
#include <Uxtheme.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <assert.h>
#include <math.h>
#include <stdint.h>

#endif
