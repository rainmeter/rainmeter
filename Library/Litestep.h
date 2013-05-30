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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __LITESTEP_H__
#define __LITESTEP_H__

#include <windows.h>
#include <comdef.h>
#include <string>
#include "../Common/StringUtil.h"
#include "Error.h"

UINT GetUniqueID();

template <typename T>
UINT TypeID() { static UINT id = GetUniqueID(); return id; }

WCHAR* GetString(UINT id);
std::wstring GetFormattedString(UINT id, ...);

HICON GetIcon(UINT id, bool large = false);

void RmNullCRTInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved);

#endif
