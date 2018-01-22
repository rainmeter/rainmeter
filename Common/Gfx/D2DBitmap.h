/* Copyright (C) 2017 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_UTIL_D2DBITMAP_H_
#define RM_GFX_UTIL_D2DBITMAP_H_

#include "Canvas.h"

namespace Gfx {

class Canvas;

namespace Util {
	class D2DEffectStream;
} // Util

class BitmapSegment
{
public:
	BitmapSegment(Microsoft::WRL::ComPtr<ID2D1Bitmap1>& bitmap, UINT x, UINT y, UINT width, UINT height);
	BitmapSegment(Microsoft::WRL::ComPtr<ID2D1Bitmap1>& bitmap, WICRect& rect);
	~BitmapSegment() {}

private:
	friend class Canvas;
	friend class D2DBitmap;
	friend class Util::D2DEffectStream;

	BitmapSegment() = delete;

	UINT m_X;
	UINT m_Y;
	UINT m_Width;
	UINT m_Height;

	Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_Bitmap;
};


class D2DBitmap
{
public:
	D2DBitmap(const std::wstring& path);
	~D2DBitmap();

	UINT GetWidth() const{ return m_Width; }
	UINT GetHeight() const{ return m_Height; }

	HRESULT AddSegment(const BitmapSegment& segment);
	void SetSize(UINT width, UINT height) { m_Width = width; m_Height = height; }

	std::wstring& GetPath() { return m_Path; }

	bool Load(const Canvas& canvas);

	Util::D2DEffectStream* CreateEffectStream();

private:
	friend class Canvas;
	friend class Util::D2DEffectStream;

	D2DBitmap() = delete;
	D2DBitmap(const D2DBitmap& other) = delete;
	D2DBitmap& operator=(D2DBitmap other) = delete;

	UINT m_Width;
	UINT m_Height;

	std::wstring m_Path;

	std::vector<BitmapSegment> m_Segments;

};

}  // namespace Gfx

#endif
