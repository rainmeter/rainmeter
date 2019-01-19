/* Copyright (C) 2018 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "D2DEffectStream.h"

namespace Gfx {
namespace Util {

const FLOAT PI = 3.14159265f;
constexpr FLOAT ToRadians(FLOAT deg) { return deg * (PI / 180.0f); }

void D2DEffectStream::Crop(const Canvas& canvas, const D2D1_RECT_F& crop)
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

void D2DEffectStream::Tint(const Canvas& canvas, const D2D1_MATRIX_5X4_F& matrix)
{
	AddEffect(canvas, CLSID_D2D1ColorMatrix);
	for (auto& effect : m_Effects)
	{
		effect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);
	}
}

void D2DEffectStream::Rotate(const Canvas& canvas, const FLOAT& angle)
{
	AddEffect(canvas, CLSID_D2D12DAffineTransform);
	const auto size = GetSize(canvas);
	const FLOAT originalW = size.width;
	const FLOAT originalH = size.height;
	if (originalW == 0.0f || originalH == 0.0f) return;

	const D2D1_POINT_2F pt = D2D1::Point2F(originalW / 2.0f, originalH / 2.0f);

	for (auto& effect : m_Effects)
	{
		const FLOAT cos_f = cos(ToRadians(angle));
		const FLOAT sin_f = sin(ToRadians(angle));

		const FLOAT transformW = fabs(originalW * cos_f) + fabs(originalH * sin_f);
		const FLOAT transformH = fabs(originalW * sin_f) + fabs(originalH * cos_f);

		const FLOAT cx = transformW / 2.0f;
		const FLOAT cy = transformH / 2.0f;

		effect->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX,
			D2D1::Matrix3x2F::Rotation(angle, pt) *
			D2D1::Matrix3x2F::Translation(cx - pt.x, cy - pt.y));

		if (fmod(angle, 90.0f) == 0.0f)
		{
			effect->SetValue(D2D1_2DAFFINETRANSFORM_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);
		}
	}
}

void D2DEffectStream::Flip(const Canvas& canvas, const FlipType& flipType)
{
	AddEffect(canvas, CLSID_D2D12DAffineTransform);
	const auto size = GetSize(canvas);
	if (size.width == 0.0f || size.height == 0.0f) return;

	const D2D1_POINT_2F pt = D2D1::Point2F(size.width / 2.0f, size.height / 2.0f);

	for (auto& effect : m_Effects)
	{
		D2D1_MATRIX_3X2_F transform = D2D1::Matrix3x2F::Identity();

		switch (flipType)
		{
		case FlipType::Vertical: transform = D2D1::Matrix3x2F::Scale(1.0f, -1.0f, pt); break;
		case FlipType::Horizontal: transform = D2D1::Matrix3x2F::Scale(-1.0f, 1.0f, pt); break;
		case FlipType::Both: transform = D2D1::Matrix3x2F::Scale(-1.0f, -1.0f, pt); break;

		case FlipType::None:
		default:
			continue;
		}

		effect->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX, transform);
	}
}

void D2DEffectStream::ApplyExifOrientation(const Canvas& canvas)
{
	switch (m_BaseImage->GetOrientation())
	{
	case 2: Flip(canvas, FlipType::Horizontal); break;
	case 3: Rotate(canvas, 180.0f); break;
	case 4: Flip(canvas, FlipType::Vertical); break;
	case 5: Flip(canvas, FlipType::Horizontal); Rotate(canvas, 270.0f); break;
	case 6: Rotate(canvas, 90.0f); break;
	case 7: Flip(canvas, FlipType::Horizontal); Rotate(canvas, 90.0f); break;
	case 8: Rotate(canvas, 270.0f); break;
	}
}

D2DBitmap* D2DEffectStream::ToBitmap(Canvas& canvas, const D2D1_SIZE_F* imageSize)
{
	bool changed = false;
	for (const auto& effect : m_Effects)
	{
		changed |= !!effect;
	}
	if (!changed) return m_BaseImage;

	const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

	Microsoft::WRL::ComPtr<ID2D1Image> target;
	canvas.m_Target->GetTarget(target.GetAddressOf());

	D2D1_MATRIX_3X2_F transform = D2D1::Matrix3x2F::Identity();
	canvas.m_Target->GetTransform(&transform);

	const UINT maxBitmapSize = (UINT)canvas.m_MaxBitmapSize;
	const auto size = (imageSize) ? *imageSize : GetSize(canvas);
	if (size.width < 0.0f || size.height < 0.0f) return nullptr;

	const UINT width = (UINT)size.width;
	const UINT height = (UINT)size.height;

	D2DBitmap* d2dbitmap = new D2DBitmap(m_BaseImage->m_Path, m_BaseImage->m_ExifOrientation);
	d2dbitmap->m_Width = width;
	d2dbitmap->m_Height = height;

	auto deleteImage = [&d2dbitmap]() -> void
	{
		delete d2dbitmap;
		d2dbitmap = nullptr;
	};

	auto didDraw = canvas.IsDrawing();
	if (didDraw)
	{
		canvas.m_Target->Flush();
	}
	else
	{
		canvas.BeginDraw();
	}

	for (UINT y = 0U, H = height / maxBitmapSize; y <= H; ++y)
	{
		for (UINT x = 0U, W = width / maxBitmapSize; x <= W; ++x)
		{
			D2D1_RECT_U rect = D2D1::RectU(
				(x * maxBitmapSize),
				(y * maxBitmapSize),
				(x == W ? (width - maxBitmapSize * x) : maxBitmapSize),		// If last x coordinate, find cutoff
				(y == H ? (height - maxBitmapSize * y) : maxBitmapSize));	// If last y coordinate, find cutoff

			Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
			HRESULT hr = canvas.m_Target->CreateBitmap(
				D2D1::SizeU(rect.right, rect.bottom),
				nullptr,
				0U,
				props,
				bitmap.GetAddressOf());
			if (FAILED(hr))
			{
				canvas.EndDraw();
				canvas.m_Target->SetTarget(target.Get());
				canvas.m_Target->SetTransform(transform);
				deleteImage();
				return nullptr;
			}

			canvas.m_Target->SetTarget(bitmap.Get());
			canvas.m_Target->Clear();

			FLOAT x2 = -(FLOAT)rect.left;
			FLOAT y2 = -(FLOAT)rect.top;

			for (size_t i = 0; i < m_Effects.size(); ++i)
			{
				auto& it = m_BaseImage->m_Segments[i];
				const auto& effect = m_Effects[i];

				Microsoft::WRL::ComPtr<ID2D1Image> image;
				effect->GetOutput(image.GetAddressOf());

				D2D1_RECT_F rect = D2D1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
				hr = canvas.m_Target->GetImageLocalBounds(image.Get(), &rect);
				if (FAILED(hr))
				{
					deleteImage();
					return nullptr;
				}

				canvas.m_Target->SetTransform(D2D1::Matrix3x2F::Translation(x2, y2));
				canvas.m_Target->DrawImage(effect.Get(), D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR); // We don't do any scaling with this image, so use the simplest interpolation

				x2 += rect.right;
				if (m_BaseImage->GetWidth() >= (it.GetX() + it.GetY())) // only increment y if end of row
				{
					y2 += rect.bottom;
					x2 = 0.0f;
				}
			}

			canvas.m_Target->Flush();
			d2dbitmap->AddSegment(bitmap, rect);
		}
	}

	if (didDraw)
	{
		canvas.m_Target->Flush();
	}
	else
	{
		canvas.EndDraw();
	}

	canvas.m_Target->SetTarget(target.Get());
	canvas.m_Target->SetTransform(transform);
	return d2dbitmap;
}

D2D1_SIZE_F D2DEffectStream::GetSize(const Canvas& canvas)
{
	D2D1_SIZE_F size = D2D1::SizeF(0.0f, 0.0f);

	UINT prevY = 0u;
	for (size_t i = 0; i < m_Effects.size(); ++i)
	{
		const auto& effect = m_Effects[i];
		if (!effect) return size;

		auto& segment = m_BaseImage->m_Segments[i];

		Microsoft::WRL::ComPtr<ID2D1Image> image;
		effect->GetOutput(image.GetAddressOf());

		D2D1_RECT_F rect = D2D1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
		HRESULT hr = canvas.m_Target->GetImageLocalBounds(image.Get(), &rect);
		if (FAILED(hr)) return size;

		if (i == 0)
		{
			size.height = rect.bottom;
			size.width = rect.right;
			continue;
		}

		const UINT y = segment.GetY();
		if (y != prevY)
		{
			prevY = y;
			size.height += rect.bottom;
		}
		else
		{
			size.width += rect.right;
		}
	}

	return size;
}

D2DEffectStream::D2DEffectStream(D2DBitmap* base)
{
	m_Effects.resize(base->m_Segments.size());
	m_BaseImage = base;
}

void D2DEffectStream::AddEffect(const Canvas& canvas, const IID& effectId)
{
	for (size_t i = 0; i < m_BaseImage->m_Segments.size(); ++i)
	{
		auto& segment = m_BaseImage->m_Segments[i];
		Microsoft::WRL::ComPtr<ID2D1Effect> effect;
		canvas.m_Target->CreateEffect(effectId, effect.GetAddressOf());

		if (!m_Effects[i])
		{
			effect->SetInput(0U, segment.GetBitmap());
		}
		else
		{
			effect->SetInputEffect(0U, m_Effects[i].Get());
		}

		m_Effects[i] = effect;
	}
}

}  // namespace Util
}  // namespace Gfx
