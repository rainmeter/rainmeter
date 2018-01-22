/* Copyright (C) 2017 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "D2DBitmap.h"
#include "Util/D2DBitmapLoader.h"
#include "Util/D2DEffectStream.h"

namespace Gfx {

BitmapSegment::BitmapSegment(Microsoft::WRL::ComPtr<ID2D1Bitmap1>& bitmap,
	UINT x, UINT y, UINT width, UINT height) :
	m_Bitmap(std::move(bitmap)),
	m_X(x),
	m_Y(y),
	m_Width(width),
	m_Height(height)	
{
}

BitmapSegment::BitmapSegment(Microsoft::WRL::ComPtr<ID2D1Bitmap1>& bitmap, WICRect& rect) :
	m_Bitmap(std::move(bitmap)),
	m_X(rect.X),
	m_Y(rect.Y),
	m_Width(rect.Width),
	m_Height(rect.Height)
{
}

D2DBitmap::D2DBitmap(const std::wstring& path) :
	m_Path(path)
{
}

D2DBitmap::~D2DBitmap()
{
}

HRESULT D2DBitmap::AddSegment(const BitmapSegment& segment)
{
	m_Segments.emplace_back(segment);
	return S_OK;
}

bool D2DBitmap::Load(const Canvas& canvas)
{
	return SUCCEEDED(Util::D2DBitmapLoader::LoadBitmapFromFile(canvas, this));
}

Util::D2DEffectStream* D2DBitmap::CreateEffectStream()
{
	return new Util::D2DEffectStream(this);
}

}  // namespace Gfx
