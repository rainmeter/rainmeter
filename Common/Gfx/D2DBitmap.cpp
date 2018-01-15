/* Copyright (C) 2017 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "D2DBitmap.h"
#include "Util/D2DBitmapLoader.h"

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

D2DBitmap* D2DBitmap::Tint(Canvas& canvas, const D2D1_MATRIX_5X4_F& matrix)
{
	Microsoft::WRL::ComPtr<ID2D1Effect> effect;
	HRESULT hr = canvas.m_Target->CreateEffect(CLSID_D2D1ColorMatrix, effect.GetAddressOf());
	if (FAILED(hr)) return nullptr;

	D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET, 
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

	Microsoft::WRL::ComPtr<ID2D1Image> target;
	canvas.m_Target->GetTarget(target.GetAddressOf());

	D2DBitmap* d2dbitmap = new D2DBitmap(m_Path);
	d2dbitmap->m_Width = m_Width;
	d2dbitmap->m_Height = m_Height;
	canvas.BeginDraw();

	for (auto it = m_Segments.begin(); it != m_Segments.end(); ++it)
	{
		Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
		hr = canvas.m_Target->CreateBitmap({ it->m_Width, it->m_Height },
			nullptr, 0, props, bitmap.GetAddressOf());
		if (FAILED(hr))
		{
			canvas.EndDraw();
			canvas.m_Target->SetTarget(target.Get());
			return nullptr;
		}

		canvas.m_Target->SetTarget(bitmap.Get());
		canvas.m_Target->Clear();

		effect->SetInput(0, it->m_Bitmap.Get());

		hr = effect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);
		if (FAILED(hr))
		{
			canvas.EndDraw();
			canvas.m_Target->SetTarget(target.Get());
			return nullptr;
		}

		canvas.m_Target->DrawImage(effect.Get(), D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
		hr = canvas.m_Target->Flush();

		d2dbitmap->AddSegment({ bitmap, it->m_X, it->m_Y, it->m_Width, it->m_Height });
	}
	canvas.EndDraw();

	canvas.m_Target->SetTarget(target.Get());
	return d2dbitmap;
}

}  // namespace Gfx
