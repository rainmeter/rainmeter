// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <windows.h>
#include <comdef.h>
#include <string>
#include "../Common/StringUtil.h"

UINT GetUniqueID();

template <typename T>
UINT TypeID() { static UINT id = GetUniqueID(); return id; }

const WCHAR* GetString(UINT id);
std::wstring GetFormattedString(UINT id, ...);

HICON GetIcon(UINT id, bool large = false);
HICON GetIconBySize(UINT id, int size);

void RmNullCRTInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved);
