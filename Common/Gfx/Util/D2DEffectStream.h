/* Copyright (C) 2017 Rainmeter Project Developers
*
* This Source Code Form is subject to the terms of the GNU General Public
* License; either version 2 of the License, or (at your option) any later
* version. If a copy of the GPL was not distributed with this file, You can
* obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_UTIL_D2DEFFECTSTREAM_H_
#define RM_GFX_UTIL_D2DEFFECTSTREAM_H_

#include "../D2DBitmap.h"

namespace Gfx {
namespace Util {

class D2DEffectStream
{
public:
	void Crop(const Canvas& canvas, const D2D1_RECT_F& crop);
	void Tint(const Canvas& canvas, const D2D1_MATRIX_5X4_F& matrix);
	D2DBitmap* ToBitmap(Canvas& canvas);

private:
	friend class Canvas;
	friend class D2DBitmap;

	D2DEffectStream(Gfx::D2DBitmap* base);

	void AddEffect(const Canvas& canvas, const GUID& effectId);

	std::vector<Microsoft::WRL::ComPtr<ID2D1Effect>> m_Effects;
	Gfx::D2DBitmap* m_BaseImage;
};

}  // namespace Util
}  // namespace Gfx

#endif
