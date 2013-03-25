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
	m_InteropTarget(),
	m_Bitmap(),
	m_GdipGraphics(),
	m_GdipBitmap(),
	m_BeginDrawCalled(false),
	m_TargetBeginDrawCalled(false)
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
	SafeRelease(&m_InteropTarget);
	SafeRelease(&m_Target);
	SafeRelease(&m_Bitmap);

	delete m_GdipGraphics;
	m_GdipGraphics = nullptr;

	delete m_GdipBitmap;
	m_GdipBitmap = nullptr;
}

void CanvasD2D::Resize(int w, int h)
{
	__super::Resize(w, h);

	DiscardDeviceResources();

	const D2D1_PIXEL_FORMAT format = D2D1::PixelFormat(
		DXGI_FORMAT_B8G8R8A8_UNORM,
		D2D1_ALPHA_MODE_PREMULTIPLIED);

	const D2D1_RENDER_TARGET_PROPERTIES properties = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		format,
		0.0f,	// Default DPI
		0.0f,	// Default DPI
		D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE);

	c_WIC->CreateBitmap(
		w,
		h,
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapCacheOnLoad,
		&m_Bitmap);

	HRESULT hr = c_D2D->CreateWicBitmapRenderTarget(m_Bitmap, properties, &m_Target);
	if (SUCCEEDED(hr))
	{
		hr = m_Target->QueryInterface(&m_InteropTarget);  // Always succeeds

		// Get the data pointer of the created IWICBitmap to create a Gdiplus::Bitmap
		// that shares the data. It is assumed that the data pointer will stay valid
		// and writable until the next resize.
		WICRect rect = {0, 0, w, h};
		IWICBitmapLock* lock = nullptr;
		hr = m_Bitmap->Lock(&rect, WICBitmapLockRead | WICBitmapLockWrite, &lock);
		if (SUCCEEDED(hr))
		{
			UINT size;
			BYTE* data;
			HRESULT hr = lock->GetDataPointer(&size, &data);
			if (SUCCEEDED(hr))
			{
				m_GdipBitmap = new Gdiplus::Bitmap(w, h, w * 4, PixelFormat32bppPARGB, data);
				m_GdipGraphics = new Gdiplus::Graphics(m_GdipBitmap);
			}

			lock->Release();
		}
	}
}

bool CanvasD2D::BeginDraw()
{
	if (m_Target)
	{
		m_BeginDrawCalled = true;
	}

	return true;
}

void CanvasD2D::EndDraw()
{
	m_BeginDrawCalled = false;
	EndTargetDraw();
}

void CanvasD2D::BeginTargetDraw()
{
	if (m_BeginDrawCalled && !m_TargetBeginDrawCalled)
	{
		m_TargetBeginDrawCalled = true;
		m_Target->BeginDraw();
	}
}

void CanvasD2D::EndTargetDraw()
{
	if (m_TargetBeginDrawCalled)
	{
		m_TargetBeginDrawCalled = false;
		HRESULT hr  = m_Target->EndDraw();
		if (hr == D2DERR_RECREATE_TARGET)
		{
			DiscardDeviceResources();

			// Attempt to recreate target.
			Resize(m_W, m_H);
		}
	}
}

Gdiplus::Graphics& CanvasD2D::BeginGdiplusContext()
{
	if (m_BeginDrawCalled)
	{
		EndTargetDraw();

		// Pretend that the render target BeginDraw() has been called. This will cause draw calls
		// on the render target to fail until EndGdiplusContext() is used.
		m_TargetBeginDrawCalled = true;
	}

	return *m_GdipGraphics;
}

void CanvasD2D::EndGdiplusContext()
{
	// See BeginGdiplusContext().
	m_TargetBeginDrawCalled = false;
}

HDC CanvasD2D::GetDC()
{
	BeginTargetDraw();

	HDC dcMemory = nullptr;
	m_InteropTarget->GetDC(D2D1_DC_INITIALIZE_MODE_COPY, &dcMemory);
	return dcMemory;
}

void CanvasD2D::ReleaseDC(HDC dc)
{
	// Assume that the DC was not modified.
	RECT r = {0, 0, 0, 0};
	m_InteropTarget->ReleaseDC(&r);
}

bool CanvasD2D::IsTransparentPixel(int x, int y)
{
	if (!(x >= 0 && y >= 0 && x < m_W && y < m_H)) return false;

	bool transparent = true;

	WICRect rect = {0, 0, m_W, m_H};
	IWICBitmapLock* lock = nullptr;
	HRESULT hr = m_Bitmap->Lock(&rect, WICBitmapLockRead, &lock);
	if (SUCCEEDED(hr))
	{
		UINT size;
		DWORD* data;
		hr = lock->GetDataPointer(&size, (BYTE**)&data);
		if (SUCCEEDED(hr))
		{
			DWORD pixel = data[y * m_W + x];  // top-down DIB
			transparent = (pixel & 0xFF000000) != 0;
		}

		lock->Release();
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
	m_Target->SetTextAntialiasMode(
		enable ? D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE : D2D1_TEXT_ANTIALIAS_MODE_ALIASED);
}

void CanvasD2D::Clear(const Gdiplus::Color& color)
{
	BeginTargetDraw();
	m_Target->Clear(ToColorF(color));
}

void CanvasD2D::DrawTextW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect, const Gdiplus::SolidBrush& brush)
{
	BeginTargetDraw();

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
	// The D2D DrawBitmap seems to perform exactly like Gdiplus::Graphics::DrawImage since we are
	// not using a hardware accelerated render target. Nevertheless, we will use it to avoid
	// the EndDraw() call needed for GDI+ drawing.
	bool draw = false;
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
			BeginTargetDraw();

			auto rDst = ToRectF(dstRect);
			auto rSrc = ToRectF(srcRect);
			m_Target->DrawBitmap(d2dBitmap, rDst, 1.0F, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, rSrc);
			draw = true;

			d2dBitmap->Release();
		}

		// D2D will still use the pixel data after this call (at the next Flush() or EndDraw()).
		bitmap->UnlockBits(bitmapLock->GetBitmapData());
	}

	if (!draw)
	{
		delete bitmapLock;
	}
}

void CanvasD2D::FillRectangle(Gdiplus::Rect& rect, const Gdiplus::SolidBrush& brush)
{
	BeginTargetDraw();

	Gdiplus::Color color;
	brush.GetColor(&color);

	ID2D1SolidColorBrush* solidBrush;
	m_Target->CreateSolidColorBrush(ToColorF(color), &solidBrush);

	m_Target->FillRectangle(ToRectF(rect), solidBrush);

	solidBrush->Release();
}

}  // namespace Gfx