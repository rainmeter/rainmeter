/* Copyright (C) 2017 Rainmeter Project Developers
*
* This Source Code Form is subject to the terms of the GNU General Public
* License; either version 2 of the License, or (at your option) any later
* version. If a copy of the GPL was not distributed with this file, You can
* obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "D2DEffectStream.h"

#define PI	(3.14159265f)
#define CONVERT_TO_RADIANS(X)	((X) * (PI / 180.0f))

void Gfx::Util::D2DEffectStream::Crop(const Canvas& canvas, const D2D1_RECT_F& crop)
{
	AddEffect(canvas, CLSID_D2D1Crop);
	for (auto& effect : m_Effects)
	{
		effect->SetValue(D2D1_CROP_PROP_RECT, crop);
		effect->SetValue(D2D1_CROP_PROP_BORDER_MODE, D2D1_BORDER_MODE_SOFT);
	}

	AddEffect(canvas, CLSID_D2D12DAffineTransform);
	for (auto& effect : m_Effects)
	{
		effect->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX, D2D1::Matrix3x2F::Translation(-crop.left, -crop.top));
	}
}

void Gfx::Util::D2DEffectStream::Tint(const Canvas& canvas, const D2D1_MATRIX_5X4_F& matrix)
{
	AddEffect(canvas, CLSID_D2D1ColorMatrix);
	for(auto& effect : m_Effects)
	{
		effect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);
	}
}

void Gfx::Util::D2DEffectStream::Rotate(const Canvas& canvas, const FLOAT& angle)
{
	AddEffect(canvas, CLSID_D2D12DAffineTransform);
	auto size = GetSize(canvas);
	for (auto& effect : m_Effects)
	{
		FLOAT originalW = size.width;
		FLOAT originalH = size.height;

		FLOAT cos_f = cos(CONVERT_TO_RADIANS(angle)), sin_f = sin(CONVERT_TO_RADIANS(angle));

		FLOAT transformW = fabs(originalW * cos_f) + fabs(originalH * sin_f);
		FLOAT transformH = fabs(originalW * sin_f) + fabs(originalH * cos_f);

		FLOAT cx = transformW / 2.0f;
		FLOAT cy = transformH / 2.0f;

		effect->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX, 
			D2D1::Matrix3x2F::Rotation(angle, D2D1::Point2F(originalW / 2.0f, originalH / 2.0f)) * 
			D2D1::Matrix3x2F::Translation(cx - originalW / 2.0f, cy - originalH / 2.0f));
	}
}

Gfx::D2DBitmap* Gfx::Util::D2DEffectStream::ToBitmap(Canvas& canvas)
{
	bool changed = false;
	for (auto& effect : m_Effects) changed = changed || effect;
	if (!changed) return m_BaseImage;

	D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

	Microsoft::WRL::ComPtr<ID2D1Image> target;
	canvas.m_Target->GetTarget(target.GetAddressOf());

	D2D1_MATRIX_3X2_F transform = {0};
	canvas.m_Target->GetTransform(&transform);

	D2DBitmap* d2dbitmap = new D2DBitmap(m_BaseImage->m_Path);

	auto size = GetSize(canvas);

	d2dbitmap->m_Width = size.width;
	d2dbitmap->m_Height = size.height;

	canvas.BeginDraw();

	const auto maxBitmapSize = canvas.m_MaxBitmapSize;
	for (UINT y = 0U, H = (UINT)floor(size.height / maxBitmapSize); y <= H; ++y)
	{
		for (UINT x = 0U, W = (UINT)floor(size.width / maxBitmapSize); x <= W; ++x)
		{
			D2D1_RECT_U rect = {
				(INT)(x * maxBitmapSize),
				(INT)(y * maxBitmapSize),
				(INT)(x == W ? (size.width - maxBitmapSize * x) : maxBitmapSize),		// If last x coordinate, find cutoff
				(INT)(y == H ? (size.height - maxBitmapSize * y) : maxBitmapSize) };		// If last y coordinate, find cutoff

			Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
			HRESULT hr = canvas.m_Target->CreateBitmap({ rect.right, rect.bottom},
				nullptr, 0, props, bitmap.GetAddressOf());
			if (FAILED(hr))
			{
				canvas.EndDraw();
				canvas.m_Target->SetTarget(target.Get());
				canvas.m_Target->SetTransform(transform);
				return nullptr;
			}

			canvas.m_Target->SetTarget(bitmap.Get());
			canvas.m_Target->Clear();

			if (FAILED(hr))
			{
				canvas.EndDraw();
				canvas.m_Target->SetTarget(target.Get());
				canvas.m_Target->SetTransform(transform);
				return nullptr;
			}

			FLOAT x2 = 0.0f, y2= 0.0f;

			for (int i = 0; i < m_Effects.size(); ++i)
			{
				auto& it = m_BaseImage->m_Segments[i];
				auto& effect = m_Effects[i];

				Microsoft::WRL::ComPtr<ID2D1Image> image;
				effect->GetOutput(image.GetAddressOf());

				D2D1_RECT_F rect = { 0 };
				hr = canvas.m_Target->GetImageLocalBounds(image.Get(), &rect);
				if (FAILED(hr)) return nullptr;


				canvas.m_Target->SetTransform(D2D1::Matrix3x2F::Translation(x2, y2));
				canvas.m_Target->DrawImage(effect.Get(), D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR); // We don't do any scaling with this image, so use the simplest interpolation

				x2 += rect.right;
				if (m_BaseImage->GetWidth() == it.m_X + it.m_Y) // only increment y if end of row
					y2 += rect.bottom;
			}

			hr = canvas.m_Target->Flush();

			d2dbitmap->AddSegment({ bitmap, rect.left, rect.top, rect.right, rect.bottom });
		}
	}
	canvas.EndDraw();

	canvas.m_Target->SetTarget(target.Get());
	canvas.m_Target->SetTransform(transform);
	return d2dbitmap;
}

D2D1_SIZE_F Gfx::Util::D2DEffectStream::GetSize(const Canvas& canvas)
{
	D2D1_SIZE_F size = {0};
	for (auto& effect : m_Effects)
	{
		Microsoft::WRL::ComPtr<ID2D1Image> image;
		effect->GetOutput(image.GetAddressOf());

		D2D1_RECT_F rect = { 0 };
		HRESULT hr = canvas.m_Target->GetImageLocalBounds(image.Get(), &rect);
		if (FAILED(hr)) return {0};

		size.width += rect.right;
		size.height += rect.bottom;
	}
	return size;
}

Gfx::Util::D2DEffectStream::D2DEffectStream(Gfx::D2DBitmap* base)
{
	m_Effects.resize(base->m_Segments.size());
	m_BaseImage = base;
}

void Gfx::Util::D2DEffectStream::AddEffect(const Canvas& canvas, const IID& effectId)
{
	for(int i = 0; i < m_BaseImage->m_Segments.size(); ++i)
	{
		auto& segment = m_BaseImage->m_Segments[i];
		Microsoft::WRL::ComPtr<ID2D1Effect> effect;
		canvas.m_Target->CreateEffect(effectId, effect.GetAddressOf());
		if (!m_Effects[i])
			effect->SetInput(0, segment.m_Bitmap.Get());
		else
			effect->SetInputEffect(0, m_Effects[i].Get());
		m_Effects[i] = effect;
	}

}
