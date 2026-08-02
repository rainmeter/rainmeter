/* Copyright (C) 2018 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "../Bitmap.h"

namespace Gfx {
namespace Util {

enum class FlipType : uint8_t
{
	None,
	Vertical,
	Horizontal,
	Both
};

class EffectStream
{
public:
	void Crop(const Canvas& canvas, const D2D1_RECT_F& crop);
	void Tint(const Canvas& canvas, const D2D1_MATRIX_5X4_F& matrix);
	void Rotate(const Canvas& canvas, const FLOAT& angle);
	void Flip(const Canvas& canvas, const FlipType& flipType);
	void ApplyExifOrientation(const Canvas& canvas);
	Bitmap* ToBitmap(Canvas& canvas, const D2D1_SIZE_F* imageSize);
	D2D1_SIZE_F GetSize(const Canvas& canvas);

private:
	friend class Canvas;
	friend class Bitmap;

	EffectStream(Gfx::Bitmap* base);

	void AddEffect(const Canvas& canvas, const IID& effectId);

	std::vector<Microsoft::WRL::ComPtr<ID2D1Effect>> m_Effects;
	Gfx::Bitmap* m_BaseImage;
};

}  // namespace Util
}  // namespace Gfx
