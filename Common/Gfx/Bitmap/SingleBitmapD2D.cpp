#include "stdafx.h"
#include "SingleBitmapD2D.h"
#include <wrl/client.h>

#define PI	(3.14159265f)
#define CONVERT_TO_RADIANS(X)	((X) * (PI / 180.0f))

namespace Gfx
{

SingleBitmapD2D::SingleBitmapD2D(Microsoft::WRL::ComPtr<ID2D1Image>& bitmap) : BitmapBase(), m_Bitmap(bitmap)
{

}

SingleBitmapD2D::~SingleBitmapD2D()
{
}

void SingleBitmapD2D::Draw(const Gfx::Canvas& canvas, const Gdiplus::Rect& dstRect, const Gdiplus::Rect& srcRect)
{
	if (m_Bitmap)
	{
		D2D1_RECT_F rect = Util::ToRectF(srcRect);
		D2D1_POINT_2F offset = { (FLOAT)dstRect.X, (FLOAT)dstRect.Y };
		canvas.m_Target->DrawImage(m_Bitmap.Get(), &offset, &rect, D2D1_INTERPOLATION_MODE_LINEAR);
	}
}

HRESULT SingleBitmapD2D::Tint(const Gfx::Canvas& canvas, const D2D1_MATRIX_5X4_F& matrix, Microsoft::WRL::ComPtr<ID2D1Effect>& m_Image)
{
	Microsoft::WRL::ComPtr<ID2D1Effect> m_Effect;
	HRESULT hr = canvas.m_Target->CreateEffect(CLSID_D2D1ColorMatrix, m_Effect.GetAddressOf());

	if (FAILED(hr)) return hr;
	if (m_Image)
	{
		m_Effect->SetInputEffect(0, m_Image.Get());
	}
	else
	{
		m_Effect->SetInput(0, m_Bitmap.Get());
	}

	hr = m_Effect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);
	if (FAILED(hr)) return hr;

	m_Image = m_Effect;
	return hr;
}

HRESULT SingleBitmapD2D::Scale(const Gfx::Canvas& canvas, const D2D1_VECTOR_2F& scale, Microsoft::WRL::ComPtr<ID2D1Effect>& m_Image)
{
	Microsoft::WRL::ComPtr<ID2D1Effect> m_Effect;
	HRESULT hr = canvas.m_Target->CreateEffect(CLSID_D2D1Scale, m_Effect.GetAddressOf());
	if (FAILED(hr)) return hr;

	if (m_Image)
	{
		m_Effect->SetInputEffect(0, m_Image.Get());
	}
	else
	{
		m_Effect->SetInput(0, m_Bitmap.Get());
	}

	m_Effect->SetValue(D2D1_SCALE_PROP_SCALE, scale);
	hr = m_Effect->SetValue(D2D1_SCALE_PROP_INTERPOLATION_MODE, D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC);
	if (FAILED(hr)) return hr;

	m_Image = m_Effect;
	return hr;
}

HRESULT SingleBitmapD2D::Crop(const Gfx::Canvas& canvas, const D2D1_RECT_F& crop, CROPMODE cropmode, Microsoft::WRL::ComPtr<ID2D1Effect>& m_Image)
{
	Microsoft::WRL::ComPtr<ID2D1Effect> m_Effect;
	HRESULT hr = canvas.m_Target->CreateEffect(CLSID_D2D1Crop, m_Effect.GetAddressOf());

	if (FAILED(hr)) return hr;
	if (m_Image)
	{
		m_Effect->SetInputEffect(0, m_Image.Get());
	}
	else
	{
		m_Effect->SetInput(0, m_Bitmap.Get());
	}

	int x, y;
	switch (cropmode)
	{
	case CROPMODE_TL:
	default:
		x = crop.left;
		y = crop.top;
		break;

	case CROPMODE_TR:
		x = crop.left + m_Width;
		y = crop.top;
		break;

	case CROPMODE_BR:
		x = crop.left + m_Width;
		y = crop.top + m_Height;
		break;

	case CROPMODE_BL:
		x = crop.left;
		y = crop.top + m_Height;
		break;

	case CROPMODE_C:
		x = crop.left + (m_Width / 2);
		y = crop.top + (m_Height / 2);
		break;
	}

	auto rect = D2D1::RectF(x, y, crop.right - crop.left + x, crop.bottom - crop.top + y);

	FLOAT scaleX = m_Width / (rect.right - rect.left);
	FLOAT scaleY = m_Height / (rect.bottom - rect.top);

	if (rect.right > m_Width) rect.right = m_Width;
	if (rect.bottom > m_Height) rect.bottom = m_Height;

	hr = m_Effect->SetValue(D2D1_CROP_PROP_RECT, rect);
	if (FAILED(hr)) return hr;

	auto original = m_Image;

	m_Image = m_Effect;

	D2D1_MATRIX_3X2_F matrix = D2D1::Matrix3x2F(1.0f, 0.0f, 0.0f, 1.0f, -x, -y);
	hr = Transform(canvas, matrix, m_Image);
	if (FAILED(hr))
	{
		m_Image = original;
		return hr;
	}

	hr = Scale(canvas, { scaleX, scaleY }, m_Image);
	if (FAILED(hr))
	{
		m_Image = original;
		return hr;
	}

	return hr;
}

HRESULT SingleBitmapD2D::Transform(const Gfx::Canvas& canvas, const D2D1_MATRIX_3X2_F& transform, Microsoft::WRL::ComPtr<ID2D1Effect>& m_Image)
{
	Microsoft::WRL::ComPtr<ID2D1Effect> transformEffect;
	HRESULT hr = canvas.m_Target->CreateEffect(CLSID_D2D12DAffineTransform, transformEffect.GetAddressOf());
	if (FAILED(hr)) return hr;

	transformEffect->SetInputEffect(0, m_Image.Get());

	hr = transformEffect->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX, transform);
	m_Image = transformEffect;
	return hr;
}

HRESULT SingleBitmapD2D::Greyscale(const Gfx::Canvas& canvas, Microsoft::WRL::ComPtr<ID2D1Effect>& m_Image)
{
	return Tint(canvas, { 0.299f, 0.299f, 0.299f, 0, 0.587f, 0.587f, 0.587f, 0, 0.114f, 0.114f, 0.114f, 0, 0, 0, 0, 1, 0, 0, 0, 0 }, m_Image);
}

BitmapBase* SingleBitmapD2D::ApplyDynamicOptions(const Gfx::Canvas& canvas, BitmapD2D* bitmap)
{
	Microsoft::WRL::ComPtr<ID2D1Effect> effect;

	HRESULT hr = S_OK;

	if (bitmap->m_GreyScale)
	{
		hr = Greyscale(canvas, effect);
		if (FAILED(hr)) return nullptr;
	}

	hr = Tint(canvas, bitmap->m_ColorMatrix, effect);
	if (FAILED(hr)) return nullptr;

	auto crop = bitmap->m_Crop;
	if (crop.left >= 0 || crop.right >= 0 || crop.top >= 0 || crop.bottom >= 0)
	{
		hr = Crop(canvas, crop, bitmap->m_CropMode, effect);
		if (FAILED(hr)) return nullptr;
	}


	D2D1_VECTOR_2F scale;
	if (bitmap->m_Rotate != 0)
	{
		FLOAT rotate = bitmap->m_Rotate;

		FLOAT originalW = m_Width;
		FLOAT originalH = m_Height;

		FLOAT cos_f = cos(CONVERT_TO_RADIANS(rotate));
		FLOAT sin_f = sin(CONVERT_TO_RADIANS(rotate));
		FLOAT transformW = fabs(originalW * cos_f) + fabs(originalH * sin_f);
		FLOAT transformH = fabs(originalW * sin_f) + fabs(originalH * cos_f);

		FLOAT cx = transformW / 2.0f;
		FLOAT cy = transformH / 2.0f;

		auto transform = D2D1::Matrix3x2F::Rotation(bitmap->m_Rotate, { originalW / 2.0f, originalH / 2.0f }) * 
			D2D1::Matrix3x2F::Translation(cx - originalW / 2.0f, cy - originalH / 2.0f);

		scale.x = originalW / transformW;
		scale.y = originalH / transformH;

		hr = Transform(canvas, transform, effect);
		if (FAILED(hr)) return nullptr;

		hr = Scale(canvas, scale, effect);
		if (FAILED(hr)) return nullptr;
	}



	if (effect)
	{
		Microsoft::WRL::ComPtr<ID2D1Image> image;
		effect->GetOutput(image.GetAddressOf());

		auto res = new SingleBitmapD2D(image);
		res->m_Width = m_Width * scale.x;
		res->m_Height = m_Height * scale.y;

		return res;
	}

	return nullptr;
}

} // namespace Gfx