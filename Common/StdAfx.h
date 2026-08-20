// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

// Common is used by projects that don't link to msvcpNNN.dll at all so this header should include
// only C compatible headers.

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include <Windows.h>
#include <Commctrl.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <dwrite_1.h>
#include <Shlobj.h>
#include <Uxtheme.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <VersionHelpers.h>

#include <assert.h>
#include <math.h>
#include <stdint.h>

#include <string>

#include "ankerl/unordered_dense.h"
