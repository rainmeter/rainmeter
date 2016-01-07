/* Copyright (C) 2014 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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
#include <VersionHelpers.h>

#include <assert.h>
#include <math.h>
#include <stdint.h>

#include <string>
#include <unordered_map>

#endif
