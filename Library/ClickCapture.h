// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <d2d1.h>
#include "Mouse.h"

class MeasureMouse;

namespace ClickCapture {

void Initialize();
bool Start(MeasureMouse* owner, const D2D1_COLOR_F& color, MOUSECURSOR cursorType);
void Stop(MeasureMouse* owner);
bool IsOwner(const MeasureMouse* owner);
bool IsOverlayWindow(HWND window);
void Reset();
void Finalize();

}  // namespace ClickCapture
