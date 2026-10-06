// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 Rainmeter Team.
// Copyright (c) 2025-2026 ozone10.
// Derived from https://github.com/ozone10/win32-darkmodelib.
// This Source Code Form is subject to the Mozilla Public License, v. 2.0.
// You can obtain a copy of the MPL at https://mozilla.org/MPL/2.0/.

#pragma once

#include <Windows.h>

namespace WindowsTheme {

void Initialize();
bool IsUsingDarkMode();

void ApplyToWindow(HWND window);
void ApplyToControl(HWND window);
void ApplyToTooltip(HWND window);

void EnableDialogTexture(HWND window, bool enable);
void HandleSettingChange(LPARAM lParam);

int ShowMessageBox(HWND parent, const WCHAR* text, const WCHAR* caption, UINT type);

COLORREF GetTextColor();
COLORREF GetDisabledTextColor();
COLORREF GetBackgroundColor();
COLORREF GetHotBackgroundColor();
HBRUSH GetBackgroundBrush();
HBRUSH GetHotBackgroundBrush();

}  // namespace WindowsTheme
