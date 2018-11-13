/* Copyright (C) 2018 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Stdafx.h"
#include "RenderTexture.h"
#include "Canvas.h"

namespace Gfx {

RenderTexture::RenderTexture(Canvas& canvas, UINT width, UINT height) : m_Bitmap()
{
    m_Bitmap.m_Width = width;
    m_Bitmap.m_Height = height;

	D2D1_BITMAP_PROPERTIES1 bProps = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    Microsoft::WRL::ComPtr<ID2D1Bitmap1> segment;
    canvas.m_Target->CreateBitmap(D2D1::SizeU(width, height), NULL, 0, bProps, segment.GetAddressOf());

    m_Bitmap.AddSegment(segment, 0u, 0u, width, height);
}

void RenderTexture::Resize(Canvas& canvas, UINT width, UINT height) {
	if (width == m_Bitmap.m_Width && height == m_Bitmap.m_Height) return;

	m_Bitmap.m_Segments.clear();

	m_Bitmap.m_Width = width;
	m_Bitmap.m_Height = height;

	D2D1_BITMAP_PROPERTIES1 bProps = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

	Microsoft::WRL::ComPtr<ID2D1Bitmap1> segment;
	canvas.m_Target->CreateBitmap(D2D1::SizeU(width, height), NULL, 0, bProps, segment.GetAddressOf());

	m_Bitmap.AddSegment(segment, 0u, 0u, width, height);
}

D2DBitmap* RenderTexture::GetBitmap() 
{
    return &m_Bitmap;
}

} // namespace Gfx
