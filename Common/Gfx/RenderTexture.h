// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Bitmap.h"
#include "Canvas.h"

namespace Gfx {

class RenderTexture
{
public:
	RenderTexture(Canvas& canvas, UINT width, UINT height);

	void Resize(Canvas& canvas, UINT width, UINT height);
	void InvalidateDeviceResources();

	Bitmap* GetBitmap() { return &m_Bitmap; }

private:
	void CreateBitmap(Canvas& canvas, Bitmap& bitmap, UINT width, UINT height);

	Bitmap m_Bitmap;
};

} // namespace Gfx
