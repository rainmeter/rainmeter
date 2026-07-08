/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __RAINMETER_WINDOWOCCLUSIONTRACKER_H__
#define __RAINMETER_WINDOWOCCLUSIONTRACKER_H__

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

#endif
