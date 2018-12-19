/* Copyright (C) 2018 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_RENDERTEXTURE_H__
#define RM_GFX_RENDERTEXTURE_H__

#include "D2DBitmap.h"
#include "Canvas.h"

namespace Gfx {

class RenderTexture {
public:
    RenderTexture(Canvas& canvas, UINT width, UINT height);
    
	void Resize(Canvas& canvas, UINT width, UINT height);
	D2DBitmap* GetBitmap();

private:
    D2DBitmap m_Bitmap;
    
};

} // namespace Gfx

#endif
