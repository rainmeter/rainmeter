/* Copyright (C) 2002 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_UTIL_H_
#define RM_LIBRARY_UTIL_H_

#include <windows.h>
#include <comdef.h>
#include <string>
#include "../Common/StringUtil.h"

UINT GetUniqueID();

template <typename T>
UINT TypeID() { static UINT id = GetUniqueID(); return id; }

WCHAR* GetString(UINT id);
std::wstring GetFormattedString(UINT id, ...);

HICON GetIcon(UINT id, bool large = false);
HICON GetIconBySize(UINT id, int size);

void RmNullCRTInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved);

#endif
