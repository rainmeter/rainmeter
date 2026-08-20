// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <windows.h>

// Based on Chromium's native_window_occlusion_tracker_win.cc:
// https://chromium.googlesource.com/chromium/src/+/main/ui/aura/native_window_occlusion_tracker_win.cc
//
// The tracker listens for WinEvent changes, debounces recalculation while move loops are active,
// and walks the top-level z-order while subtracting opaque regions from the virtual desktop to
// classify each tracked skin window.
namespace WindowOcclusionTracker {

void Initialize(HWND messageWindow, UINT timerID);
void Finalize();
void TrackWindow(HWND hwnd);
void UntrackWindow(HWND hwnd);
void HandleTimer();
void HandleShowDesktopChange();
void HandleDisplayChange();
void HandlePowerResume();
void HandleSessionChange(WPARAM sessionEvent);

}  // namespace WindowOcclusionTracker
