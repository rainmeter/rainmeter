/*
  Copyright (C) 2013 Birunthan Mohanathas

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "CanvasD2D.h"
#include "TextFormatD2D.h"
#include "Util/DWriteFontCollectionLoader.h"
#include "Util/WICBitmapLockGDIP.h"
#include "../../Library/Litestep.h"

namespace {

D2D1_COLOR_F ToColorF(const Gdiplus::Color& color)
{
	return D2D1::ColorF(color.GetR() / 255.0f, color.GetG() / 255.0f, color.GetB() / 255.0f, color.GetA() / 255.0f);
}

D2D1_RECT_F ToRectF(const Gdiplus::Rect& rect)
{
	return D2D1::RectF(static_cast<float>(rect.X), static_cast<float>(rect.Y), static_cast<float>(rect.X) + static_cast<float>(rect.Width), static_cast<float>(rect.Y) + static_cast<float>(rect.Height));
}

D2D1_RECT_F ToRectF(const Gdiplus::RectF& rect)
{
	return D2D1::RectF(rect.X, rect.Y, rect.X + rect.Width, rect.Y + rect.Height);
}

}  // namespace

namespace Gfx {

UINT CanvasD2D::c_Instances = 0;
Microsoft::WRL::ComPtr<ID2D1Factory> CanvasD2D::c_D2DFactory;
Microsoft::WRL::ComPtr<IDWriteFactory> CanvasD2D::c_DWFactory;
Microsoft::WRL::ComPtr<IDWriteGdiInterop> CanvasD2D::c_DWGDIInterop;
Microsoft::WRL::ComPtr<IWICImagingFactory> CanvasD2D::c_WICFactory;

CanvasD2D::CanvasD2D() : Canvas(),
	m_Bitmap(),
	m_TextAntiAliasing(false)
{
	Initialize();
}

CanvasD2D::~CanvasD2D()
{
	Finalize();
}

bool CanvasD2D::Initialize()
{
	++c_Instances;
	if (c_Instances == 1)
	{
		D2D1_FACTORY_OPTIONS fo = {};
#ifdef _DEBUG
		fo.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

		HRESULT hr = D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			fo,
			c_D2DFactory.GetAddressOf());
		if (FAILED(hr)) return false;

		hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_IWICImagingFactory,
			(LPVOID*)c_WICFactory.GetAddressOf());
		if (FAILED(hr)) return false;

		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			(IUnknown**)c_DWFactory.GetAddressOf());
		if (FAILED(hr)) return false;

		hr = c_DWFactory->GetGdiInterop(c_DWGDIInterop.GetAddressOf());
		if (FAILED(hr)) return false;

		hr = c_DWFactory->RegisterFontCollectionLoader(Util::DWriteFontCollectionLoader::GetInstance());
		if (FAILED(hr)) return false;
	}

	return true;
}

void CanvasD2D::Finalize()
{
	--c_Instances;
	if (c_Instances == 0)
	{
		c_D2DFactory.Reset();
		c_WICFactory.Reset();
		c_DWGDIInterop.Reset();

		c_DWFactory->UnregisterFontCollectionLoader(Util::DWriteFontCollectionLoader::GetInstance());
		c_DWFactory.Reset();
	}
}

void CanvasD2D::Resize(int w, int h)
{
	__super::Resize(w, h);

	m_Target.Reset();

	m_Bitmap.Resize(w, h);

	m_GdipBitmap.reset(new Gdiplus::Bitmap(w, h, w * 4, PixelFormat32bppPARGB, m_Bitmap.GetData()));
	m_GdipGraphics.reset(new Gdiplus::Graphics(m_GdipBitmap.get()));
}

bool CanvasD2D::BeginDraw()
{
	return true;
}

void CanvasD2D::EndDraw()
{
	EndTargetDraw();
}

bool CanvasD2D::BeginTargetDraw()
{
	if (m_Target) return true;

	const D2D1_PIXEL_FORMAT format = D2D1::PixelFormat(
		DXGI_FORMAT_B8G8R8A8_UNORM,
		D2D1_ALPHA_MODE_PREMULTIPLIED);

	const D2D1_RENDER_TARGET_PROPERTIES properties = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		format,
		0.0f,	// Default DPI
		0.0f,	// Default DPI
		D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE);

	// A new Direct2D render target must be created for each sequence of Direct2D draw operations
	// since we use GDI+ to render to the same pixel data. Without creating a new render target
	// each time, it has been found that Direct2D may overwrite the draws by GDI+ since it is
	// unaware of the changes made by GDI+. By creating a new render target and then releasing it
	// before the next GDI+ draw operations, we ensure that the pixel data result is as expected
	// Once GDI+ drawing is no longer needed, we change to recreate the render target only when the
	// bitmap size is changed.
	HRESULT hr = c_D2DFactory->CreateWicBitmapRenderTarget(&m_Bitmap, properties, &m_Target);
	if (SUCCEEDED(hr))
	{
		SetTextAntiAliasing(m_TextAntiAliasing);

		m_Target->BeginDraw();

		// Apply any transforms that occurred before creation of m_Target
		m_Target->SetTransform(GetCurrentTransform());

		return true;
	}

	return false;
}

void CanvasD2D::EndTargetDraw()
{
	if (m_Target)
	{
		m_Target->EndDraw();
		m_Target.Reset();
	}
}

Gdiplus::Graphics& CanvasD2D::BeginGdiplusContext()
{
	EndTargetDraw();
	return *m_GdipGraphics;
}

void CanvasD2D::EndGdiplusContext()
{
}

HDC CanvasD2D::GetDC()
{
	EndTargetDraw();

	HDC dcMemory = CreateCompatibleDC(nullptr);
	SelectObject(dcMemory, m_Bitmap.GetHandle());
	return dcMemory;
}

void CanvasD2D::ReleaseDC(HDC dc)
{
	DeleteDC(dc);
}

bool CanvasD2D::IsTransparentPixel(int x, int y)
{
	if (!(x >= 0 && y >= 0 && x < m_W && y < m_H)) return false;

	bool transparent = true;

	DWORD* data = (DWORD*)m_Bitmap.GetData();
	if (data)
	{
		DWORD pixel = data[y * m_W + x];  // Top-down DIB.
		transparent = (pixel & 0xFF000000) != 0;
	}

	return transparent;
}

D2D1_MATRIX_3X2_F CanvasD2D::GetCurrentTransform()
{
	D2D1_MATRIX_3X2_F d2dMatrix;
	Gdiplus::Matrix gdipMatrix;
	m_GdipGraphics->GetTransform(&gdipMatrix);
	gdipMatrix.GetElements((Gdiplus::REAL*)&d2dMatrix);
	return d2dMatrix;
}

void CanvasD2D::SetTransform(const Gdiplus::Matrix& matrix)
{
	m_GdipGraphics->SetTransform(&matrix);

	if (m_Target)
	{
		m_Target->SetTransform(GetCurrentTransform());
	}
}

void CanvasD2D::ResetTransform()
{
	m_GdipGraphics->ResetTransform();

	if (m_Target)
	{
		m_Target->SetTransform(D2D1::Matrix3x2F::Identity());
	}
}

void CanvasD2D::RotateTransform(float angle, float x, float y, float dx, float dy)
{
	m_GdipGraphics->TranslateTransform(x, y);
	m_GdipGraphics->RotateTransform(angle);
	m_GdipGraphics->TranslateTransform(dx, dy);

	if (m_Target)
	{
		m_Target->SetTransform(GetCurrentTransform());
	}
}

void CanvasD2D::SetAntiAliasing(bool enable)
{
	// TODO: Set m_Target aliasing?
	m_GdipGraphics->SetSmoothingMode(
		enable ? Gdiplus::SmoothingModeHighQuality : Gdiplus::SmoothingModeNone);
	m_GdipGraphics->SetPixelOffsetMode(
		enable ? Gdiplus::PixelOffsetModeHighQuality : Gdiplus::PixelOffsetModeDefault);
}

void CanvasD2D::SetTextAntiAliasing(bool enable)
{
	// TODO: Add support for D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE?
	m_TextAntiAliasing = enable;

	if (m_Target)
	{
		m_Target->SetTextAntialiasMode(
			m_TextAntiAliasing ? D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE : D2D1_TEXT_ANTIALIAS_MODE_ALIASED);
	}
}

void CanvasD2D::Clear(const Gdiplus::Color& color)
{
	if (!m_Target)  // Use GDI+ if D2D render target has not been created.
	{
		m_GdipGraphics->Clear(color);
		return;
	}

	m_Target->Clear(ToColorF(color));
}

void CanvasD2D::DrawTextW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect, const Gdiplus::SolidBrush& brush)
{
	if (!BeginTargetDraw()) return;

	Gdiplus::Color color;
	brush.GetColor(&color);

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> solidBrush;
	HRESULT hr = m_Target->CreateSolidColorBrush(ToColorF(color), solidBrush.GetAddressOf());
	if (SUCCEEDED(hr))
	{
		TextFormatD2D& formatD2D = (TextFormatD2D&)format;
		const bool right = formatD2D.GetHorizontalAlignment() == Gfx::HorizontalAlignment::Right;

		formatD2D.CreateLayout(str, strLen, rect.Width, rect.Height);

		m_Target->DrawTextLayout(
			D2D1::Point2F(m_AccurateText ? rect.X : right ? rect.X - 3.0f : rect.X + 3.0f, rect.Y - 1.0f),
			formatD2D.m_TextLayout.Get(),
			solidBrush.Get());
	}
}

bool CanvasD2D::MeasureTextW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect)
{
	Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
	HRESULT hr = c_DWFactory->CreateTextLayout(
		str,
		strLen,
		((TextFormatD2D&)format).m_TextFormat.Get(),
		10000,
		10000,
		textLayout.GetAddressOf());
	if (SUCCEEDED(hr))
	{
		DWRITE_TEXT_METRICS metrics;
		textLayout->GetMetrics(&metrics);

		if (!m_AccurateText)
		{
			float size = 0.0f;
			textLayout->GetFontSize(0, &size);

			rect.Width = floor(metrics.width + (size / 2.05f) + 0.65f);
			rect.Height = floor(metrics.height + 1.5f);
		}
		else
		{
			rect.Width = metrics.width;
			rect.Height = metrics.height;
		}

		return true;
	}

	return false;
}

bool CanvasD2D::MeasureTextLinesW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect, UINT& lines)
{
	((TextFormatD2D&)format).m_TextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

	Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
	HRESULT hr = c_DWFactory->CreateTextLayout(
		str,
		strLen,
		((TextFormatD2D&)format).m_TextFormat.Get(),
		rect.Width,
		10000,
		textLayout.GetAddressOf());
	if (SUCCEEDED(hr))
	{
		DWRITE_TEXT_METRICS metrics;
		textLayout->GetMetrics(&metrics);

		if (!m_AccurateText)
		{
			float size = 0.0f;
			textLayout->GetFontSize(0, &size);

			rect.Width = floor(metrics.width + (size / 2.05f) + 0.65f);
			rect.Height = floor(metrics.height + 1.5f);
		}
		else
		{
			rect.Width = metrics.width;
			rect.Height = metrics.height + 1.0f;
		}

		lines = metrics.lineCount;
		return true;
	}

	return false;
}

void CanvasD2D::DrawBitmap(Gdiplus::Bitmap* bitmap, const Gdiplus::Rect& dstRect, const Gdiplus::Rect& srcRect)
{
	if (!m_Target)  // Use GDI+ if D2D render target has not been created.
	{
		m_GdipGraphics->DrawImage(
			bitmap, dstRect, srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height, Gdiplus::UnitPixel);
		return;
	}

	// The D2D DrawBitmap seems to perform exactly like Gdiplus::Graphics::DrawImage since we are
	// not using a hardware accelerated render target. Nevertheless, we will use it to avoid
	// the EndDraw() call needed for GDI+ drawing.
	Util::WICBitmapLockGDIP* bitmapLock = new Util::WICBitmapLockGDIP();
	Gdiplus::Rect lockRect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
	Gdiplus::Status status = bitmap->LockBits(
		&lockRect, Gdiplus::ImageLockModeRead, PixelFormat32bppPARGB, bitmapLock->GetBitmapData());
	if (status == Gdiplus::Ok)
	{
		D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
		Microsoft::WRL::ComPtr<ID2D1Bitmap> d2dBitmap;
		HRESULT hr = m_Target->CreateSharedBitmap(
			__uuidof(IWICBitmapLock), bitmapLock, &props, d2dBitmap.GetAddressOf());
		if (SUCCEEDED(hr))
		{
			auto rDst = ToRectF(dstRect);
			auto rSrc = ToRectF(srcRect);
			m_Target->DrawBitmap(d2dBitmap.Get(), rDst, 1.0F, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, rSrc);
		}

		// D2D will still use the pixel data after this call (at the next Flush() or EndDraw()).
		bitmap->UnlockBits(bitmapLock->GetBitmapData());
	}

	bitmapLock->Release();
}

void CanvasD2D::FillRectangle(Gdiplus::Rect& rect, const Gdiplus::SolidBrush& brush)
{
	if (!m_Target)  // Use GDI+ if D2D render target has not been created.
	{
		m_GdipGraphics->FillRectangle(&brush, rect);
		return;
	}

	Gdiplus::Color color;
	brush.GetColor(&color);

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> solidBrush;
	HRESULT hr = m_Target->CreateSolidColorBrush(ToColorF(color), solidBrush.GetAddressOf());
	if (SUCCEEDED(hr))
	{
		m_Target->FillRectangle(ToRectF(rect), solidBrush.Get());
	}
}

}  // namespace Gfx
