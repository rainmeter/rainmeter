/* Copyright (C) 2018 Rainmeter Project Developers
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

BitmapSegment::BitmapSegment(Microsoft::WRL::ComPtr<ID2D1Bitmap1>& bitmap, D2D1_RECT_U& rect) :
	m_Bitmap(std::move(bitmap)),
	m_X(rect.left),
	m_Y(rect.top),
	m_Width(rect.right),
	m_Height(rect.bottom)
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

D2DBitmap::D2DBitmap(const std::wstring& path, int exifOrientation) :
	m_Width(0U),
	m_Height(0U),
	m_ExifOrientation(exifOrientation),
	m_Path(path),
	m_FileSize(0UL),
	m_FileTime(0ULL)
	
{
}

D2DBitmap::~D2DBitmap()
{
}

void D2DBitmap::AddSegment(const BitmapSegment& segment)
{
	m_Segments.emplace_back(segment);
}

bool D2DBitmap::HasFileChanged(const std::wstring& file)
{
	return Util::D2DBitmapLoader::HasFileChanged(this, file);
}

HRESULT D2DBitmap::Load(const Canvas& canvas)
{
	return Util::D2DBitmapLoader::LoadBitmapFromFile(canvas, this);
}

Util::D2DEffectStream* D2DBitmap::CreateEffectStream()
{
	return new Util::D2DEffectStream(this);
}

void D2DBitmap::GetPixel(Canvas& canvas, int px, int py, D2D1_COLOR_F& color)
{
	// TODO: Create a duplicate bitmap for every one with CPU_READ instead of creating a small 1 px bitmap? 
	// Maybe have a 1px bitmap in Canvas since we won't ever check two different bitmaps at once and use that to fetch the pixel data?
	// Creating a bitmap every time we have to check a pixel is bad though, so we should consider the above.
	Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
	D2D1_BITMAP_PROPERTIES1 bProps = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
	HRESULT hr = canvas.m_Target->CreateBitmap(
		D2D1::SizeU(1, 1),
		nullptr,
		0U,
		bProps,
		bitmap.ReleaseAndGetAddressOf());
	if (FAILED(hr)) return;


	for (auto& it : m_Segments)
	{
		auto rect = it.GetRect();
		if(rect.left < px && rect.top < py && px <= rect.left + rect.right && py <= rect.top + rect.bottom)
		{
			auto point = D2D1::Point2U(0U, 0U);
			auto srcRect = D2D1::RectU(px - (UINT32)rect.left, py - (UINT32)rect.top, (UINT32)(px - rect.left + 1), (UINT32)(py - rect.top + 1));
			hr = bitmap->CopyFromBitmap(&point, it.GetBitmap(), &srcRect);
			break;
		}
	}

	D2D1_MAPPED_RECT data;
	hr = bitmap->Map(D2D1_MAP_OPTIONS_READ, &data);
	if (FAILED(hr)) return;

	color.r = data.bits[0];
	color.g = data.bits[1];
	color.b = data.bits[2];
	color.a = data.bits[3];

	hr = bitmap->Unmap();
}

HRESULT D2DBitmap::GetFileInfo(const std::wstring& path, FileInfo* fileInfo)
{
	return Util::D2DBitmapLoader::GetFileInfo(path, fileInfo);
}
}  // namespace Gfx
