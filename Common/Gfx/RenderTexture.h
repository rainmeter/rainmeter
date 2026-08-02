/* Copyright (C) 2018 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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
