// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "Stdafx.h"
#include "RenderTexture.h"
#include "Canvas.h"

namespace Gfx {

RenderTexture::RenderTexture(Canvas& canvas, UINT width, UINT height) : m_Bitmap()
{
	m_Bitmap.m_Width = width;
	m_Bitmap.m_Height = height;
	CreateBitmap(canvas, m_Bitmap, width, height);
}

void RenderTexture::Resize(Canvas& canvas, UINT width, UINT height)
{
	if (width == m_Bitmap.m_Width && height == m_Bitmap.m_Height && m_Bitmap.HasDeviceResources()) return;

	m_Bitmap.m_Segments.clear();

	m_Bitmap.m_Width = width;
	m_Bitmap.m_Height = height;
	CreateBitmap(canvas, m_Bitmap, width, height);
}

void RenderTexture::InvalidateDeviceResources()
{
	m_Bitmap.InvalidateDeviceResources();
}

void RenderTexture::CreateBitmap(Canvas& canvas, Bitmap& bitmap, UINT width, UINT height)
{
	const auto props = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
	Microsoft::WRL::ComPtr<ID2D1Bitmap1> segment;
	canvas.m_Target->CreateBitmap(D2D1::SizeU(width, height), nullptr, 0, props, segment.GetAddressOf());
	bitmap.AddSegment(segment, 0, 0, width, height);
}

} // namespace Gfx
