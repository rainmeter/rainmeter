// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>
#include <stddef.h>

void FormatString(WCHAR* destination, size_t count, const WCHAR* format, ...);
int Uninstall();
