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
#include "WICBitmapLockGDIP.h"
#include "../../Library/Litestep.h"

template<class T>
inline void SafeRelease(T** t)
{
	if (*t)
	{
		(*t)->Release();
		*t = nullptr;
	}
}

namespace {

D2D1_COLOR_F ToColorF(const Gdiplus::Color& color)
{
	return D2D1::ColorF(color.GetR() / 255.0f, color.GetG() / 255.0f, color.GetB() / 255.0f, color.GetA() / 255.0f);
}

D2D1_RECT_F ToRectF(const Gdiplus::Rect& rect)
{
	return D2D1::RectF(rect.X, rect.Y, rect.X + rect.Width, rect.Y + rect.Height);
}

D2D1_RECT_F ToRectF(const Gdiplus::RectF& rect)
{
	return D2D1::RectF(rect.X, rect.Y, rect.X + rect.Width, rect.Y + rect.Height);
}

}  // namespace

namespace Gfx {

UINT CanvasD2D::c_Instances = 0;
ID2D1Factory* CanvasD2D::c_D2D = nullptr;
IDWriteFactory* CanvasD2D::c_DW = nullptr;
IWICImagingFactory* CanvasD2D::c_WIC = nullptr;

CanvasD2D::CanvasD2D() : Canvas(),
	m_Target(),
	m_Bitmap(),
	m_GdipGraphics(),
	m_GdipBitmap(),
	m_TextAntiAliasing(false)
{
	Initialize();
}

CanvasD2D::~CanvasD2D()
{
	DiscardDeviceResources();
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
			&c_D2D);
		if (FAILED(hr)) return false;

		hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_IWICImagingFactory,
			(LPVOID*)&c_WIC);
		if (FAILED(hr)) return false;

		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			(IUnknown**)&c_DW);
		if (FAILED(hr)) return false;
	}

	return true;
}

void CanvasD2D::Finalize()
{
	--c_Instances;
	if (c_Instances == 0)
	{
		SafeRelease(&c_D2D);
		SafeRelease(&c_WIC);
		SafeRelease(&c_DW);
	}
}

void CanvasD2D::DiscardDeviceResources()
{
	SafeRelease(&m_Target);

	delete m_GdipGraphics;
	m_GdipGraphics = nullptr;

	delete m_GdipBitmap;
	m_GdipBitmap = nullptr;
}

void CanvasD2D::Resize(int w, int h)
{
	__super::Resize(w, h);

	DiscardDeviceResources();

	m_Bitmap.Resize(w, h);

	m_GdipBitmap = new Gdiplus::Bitmap(w, h, w * 4, PixelFormat32bppPARGB, m_Bitmap.GetData());
	m_GdipGraphics = new Gdiplus::Graphics(m_GdipBitmap);
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
	HRESULT hr = c_D2D->CreateWicBitmapRenderTarget(&m_Bitmap, properties, &m_Target);
	if (SUCCEEDED(hr))
	{
		SetTextAntiAliasing(m_TextAntiAliasing);

		m_Target->BeginDraw();
		return true;
	}

	return false;
}

void CanvasD2D::EndTargetDraw()
{
	if (m_Target)
	{
		m_Target->EndDraw();

		SafeRelease(&m_Target);
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
	if (!BeginTargetDraw()) return;

	m_Target->Clear(ToColorF(color));
}

void CanvasD2D::DrawTextW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect, const Gdiplus::SolidBrush& brush)
{
	if (!BeginTargetDraw()) return;

	Gdiplus::Color color;
	brush.GetColor(&color);

	ID2D1SolidColorBrush* solidBrush;
	m_Target->CreateSolidColorBrush(ToColorF(color), &solidBrush);

	bool right = ((TextFormatD2D&)format).GetHorizontalAlignment() == Gfx::HorizontalAlignment::Right;

	// TODO: Draw cached layout?
	//m_Target->DrawTextLayout(
	//	D2D1::Point2F(right ? rect.X - 2 : rect.X + 2.0f, rect.Y - 1.0f),
	//	textLayout,
	//	solidBrush);

	m_Target->DrawTextW(
		str,
		strLen,
		((TextFormatD2D&)format).m_TextFormat,
		D2D1::RectF(right ? rect.X - 2 : rect.X + 2.0f, rect.Y - 1.0f, (right ? rect.X - 2 : rect.X + 2.0f) + rect.Width, rect.Y + rect.Height  - 1.0f),
		solidBrush,
		D2D1_DRAW_TEXT_OPTIONS_NONE);

	solidBrush->Release();
}

bool CanvasD2D::MeasureTextW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect)
{
	IDWriteTextLayout* textLayout;
	HRESULT hr = c_DW->CreateTextLayout(
		str,
		strLen,
		((TextFormatD2D&)format).m_TextFormat,
		10000,
		10000,
		&textLayout);
	if (SUCCEEDED(hr))
	{
		DWRITE_TEXT_METRICS metrics;
		textLayout->GetMetrics(&metrics);
		rect.Width = metrics.width + 5.0f;
		rect.Height = metrics.height + 1.0f;  // 1.0f to get same result as GDI+.

		textLayout->Release();
		return true;
	}

	return false;
}

bool CanvasD2D::MeasureTextLinesW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect, UINT& lines)
{
	((TextFormatD2D&)format).m_TextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

	IDWriteTextLayout* textLayout;
	HRESULT hr = c_DW->CreateTextLayout(
		str,
		strLen,
		((TextFormatD2D&)format).m_TextFormat,
		rect.Width,
		10000,
		&textLayout);
	if (SUCCEEDED(hr))
	{
		DWRITE_TEXT_METRICS metrics;
		textLayout->GetMetrics(&metrics);
		rect.Width = metrics.width + 5.0f;
		rect.Height = metrics.height + 1.0f;  // 1.0f to get same result as GDI+.
		lines = metrics.lineCount;

		textLayout->Release();
		return true;
	}

	return false;
}

void CanvasD2D::DrawBitmap(Gdiplus::Bitmap* bitmap, const Gdiplus::Rect& dstRect, const Gdiplus::Rect& srcRect)
{
	if (!BeginTargetDraw()) return;

	// The D2D DrawBitmap seems to perform exactly like Gdiplus::Graphics::DrawImage since we are
	// not using a hardware accelerated render target. Nevertheless, we will use it to avoid
	// the EndDraw() call needed for GDI+ drawing.
	WICBitmapLockGDIP* bitmapLock = new WICBitmapLockGDIP();
	Gdiplus::Status status = bitmap->LockBits(
		&srcRect, Gdiplus::ImageLockModeRead, PixelFormat32bppPARGB, bitmapLock->GetBitmapData());
	if (status == Gdiplus::Ok)
	{
		D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
		ID2D1Bitmap* d2dBitmap;
		HRESULT hr = m_Target->CreateSharedBitmap(__uuidof(IWICBitmapLock), bitmapLock, &props, &d2dBitmap);
		if (SUCCEEDED(hr))
		{
			auto rDst = ToRectF(dstRect);
			auto rSrc = ToRectF(srcRect);
			m_Target->DrawBitmap(d2dBitmap, rDst, 1.0F, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, rSrc);

			d2dBitmap->Release();
		}

		// D2D will still use the pixel data after this call (at the next Flush() or EndDraw()).
		bitmap->UnlockBits(bitmapLock->GetBitmapData());
	}

	bitmapLock->Release();
}

void CanvasD2D::FillRectangle(Gdiplus::Rect& rect, const Gdiplus::SolidBrush& brush)
{
	if (!BeginTargetDraw()) return;

	Gdiplus::Color color;
	brush.GetColor(&color);

	ID2D1SolidColorBrush* solidBrush;
	m_Target->CreateSolidColorBrush(ToColorF(color), &solidBrush);

	m_Target->FillRectangle(ToRectF(rect), solidBrush);

	solidBrush->Release();
}

}  // namespace Gfx