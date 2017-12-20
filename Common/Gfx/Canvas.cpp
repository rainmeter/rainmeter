/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Canvas.h"
#include "TextFormatD2D.h"
#include "Util/D2DUtil.h"
#include "Util/DWriteFontCollectionLoader.h"
#include "Util/WICBitmapLockGDIP.h"
#include "../../Library/Util.h"
#include "../../Library/Logger.h"
#include "D2DBitmap.h"

namespace Gfx {

UINT Canvas::c_Instances = 0;
D3D_FEATURE_LEVEL Canvas::c_FeatureLevel;
Microsoft::WRL::ComPtr<ID3D11Device> Canvas::c_D3DDevice;
Microsoft::WRL::ComPtr<ID3D11DeviceContext> Canvas::c_D3DContext;
Microsoft::WRL::ComPtr<ID2D1Device> Canvas::c_D2DDevice;
Microsoft::WRL::ComPtr<IDXGIDevice1> Canvas::c_DxgiDevice;
Microsoft::WRL::ComPtr<ID2D1Factory1> Canvas::c_D2DFactory;
Microsoft::WRL::ComPtr<IDWriteFactory1> Canvas::c_DWFactory;
Microsoft::WRL::ComPtr<IWICImagingFactory> Canvas::c_WICFactory;

Canvas::Canvas() :
	m_W(0),
	m_H(0),
	m_MaxBitmapSize(0U),
	m_IsDrawing(false),
	m_EnableDrawAfterGdi(false),
	m_TextAntiAliasing(false),
	m_CanUseAxisAlignClip(false),
	m_Layers()
{
	Initialize();
}

Canvas::~Canvas()
{
	Finalize();
}

bool Canvas::Initialize()
{
	++c_Instances;
	if (c_Instances == 1)
	{
		
		// Required for Direct2D interopability.
		UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		auto tryCreateContext = [&](D3D_DRIVER_TYPE driverType)
		{
			return D3D11CreateDevice(
				NULL,
				driverType,
				NULL,
				creationFlags,
				NULL,
				0u,
				D3D11_SDK_VERSION,
				c_D3DDevice.GetAddressOf(),
				&c_FeatureLevel,
				c_D3DContext.GetAddressOf());
		};

		// D3D selects the best feature level automatically and sets it
		// to |c_FeatureLevel|. First, we try to use the hardware driver
		// and if that fails, we try the WARP rasterizer for cases
		// where there is no graphics card or other failures.

		HRESULT hr = tryCreateContext(D3D_DRIVER_TYPE_HARDWARE);
		if (FAILED(hr))
		{
			hr = tryCreateContext(D3D_DRIVER_TYPE_WARP);
			if (FAILED(hr)) return false;
		}

		hr = c_D3DDevice.As(&c_DxgiDevice);
		if (FAILED(hr)) return false;

		D2D1_FACTORY_OPTIONS fo = {};
#ifdef _DEBUG
		fo.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

		hr = D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			fo,
			c_D2DFactory.GetAddressOf());
		if (FAILED(hr)) return false;

		hr = c_D2DFactory->CreateDevice(
			c_DxgiDevice.Get(),
			c_D2DDevice.GetAddressOf());
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
			__uuidof(c_DWFactory),
			(IUnknown**)c_DWFactory.GetAddressOf());
		if (FAILED(hr)) return false;

		hr = c_DWFactory->RegisterFontCollectionLoader(Util::DWriteFontCollectionLoader::GetInstance());
		if (FAILED(hr)) return false;
	}

	return true;
}

void Canvas::Finalize()
{
	--c_Instances;
	if (c_Instances == 0)
	{
		c_D3DDevice.Reset();
		c_D3DContext.Reset();
		c_D2DDevice.Reset();
		c_DxgiDevice.Reset();
		c_D2DFactory.Reset();
		c_WICFactory.Reset();

		if (c_DWFactory)
		{
			c_DWFactory->UnregisterFontCollectionLoader(Util::DWriteFontCollectionLoader::GetInstance());
			c_DWFactory.Reset();
		}
	}
}

bool Canvas::InitializeRenderTarget(HWND hwnd)
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
	swapChainDesc.Width = 1u;
	swapChainDesc.Height = 1u;
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1u;
	swapChainDesc.SampleDesc.Quality = 0u;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2u;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;

	Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
	HRESULT hr = c_DxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());
	if (FAILED(hr)) return false ;

	// Ensure that DXGI does not queue more than one frame at a time.
	hr = c_DxgiDevice->SetMaximumFrameLatency(1u);
	if (FAILED(hr)) return false;

	Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
	hr = dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));
	if (FAILED(hr)) return false;

	hr = dxgiFactory->CreateSwapChainForHwnd(
		c_DxgiDevice.Get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		m_SwapChain.ReleaseAndGetAddressOf());
	if (FAILED(hr)) return false;

	hr = CreateRenderTarget();
	if (FAILED(hr)) return false;

	return CreateTargetBitmap(0U, 0U);
}

void Canvas::Resize(int w, int h)
{
	// Truncate the size of the skin if it's too big.
	if (w > m_MaxBitmapSize) w = (int)m_MaxBitmapSize;
	if (h > m_MaxBitmapSize) h = (int)m_MaxBitmapSize;

	m_W = w;
	m_H = h;

	// Check if target, targetbitmap, backbuffer, swap chain are valid?

	// Unmap all resources tied to the swap chain.
	m_Target->SetTarget(nullptr);
	m_TargetBitmap.Reset();
	m_BackBuffer.Reset();

	// Resize swap chain.
	HRESULT hr = m_SwapChain->ResizeBuffers(
		0u,
		(UINT)w,
		(UINT)h,
		DXGI_FORMAT_B8G8R8A8_UNORM,
		DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE);
	if (FAILED(hr)) return;

	CreateTargetBitmap((UINT32)w, (UINT32)h);
}

bool Canvas::BeginDraw()
{
	if (!m_Target)
	{
		HRESULT hr = CreateRenderTarget();
		if (FAILED(hr))
		{
			m_IsDrawing = false;
			return false;
		}

		// Recreate target bitmap
		Resize(m_W, m_H);
	}

	m_Target->BeginDraw();
	m_IsDrawing = true;
	return true;
}

void Canvas::EndDraw()
{
	HRESULT hr = m_Target->EndDraw();
	if (FAILED(hr))
	{
		m_Target.Reset();
	}

	m_IsDrawing = false;

	if (m_TargetBitmap && m_BufferSnapshot)
	{
		// Create a snapshot to test for transparent pixels later.
		auto point = D2D1::Point2U(0U, 0U);
		auto srcRect = D2D1::RectU(0U, 0U, (UINT32)m_W, (UINT32)m_H);
		m_BufferSnapshot->CopyFromBitmap(&point, m_TargetBitmap.Get(), &srcRect);

		if (m_SwapChain)
		{
			// Present the frame to the screen. Wait until the next VSync to not
			// waste time rendering frames that will never be displayed on the screen.
			DXGI_PRESENT_PARAMETERS  presentParameters = { 0 };
			hr = m_SwapChain->Present1(1u, NULL, &presentParameters);
			if (FAILED(hr))
			{
				// Error, recreate all resources, or just swap chain?
			}
		}
	}
	else
	{
		// Recreate target bitmap?
	}
}

Gdiplus::Graphics& Canvas::BeginGdiplusContext()
{
	m_GdiGraphics.reset(new Gdiplus::Graphics(GetDC()));

	if (m_Target)
	{
		bool enable = m_Target->GetAntialiasMode() == D2D1_ANTIALIAS_MODE_PER_PRIMITIVE;
		SetAntiAliasing(enable);
		UpdateGdiTransform();
	}

	return *m_GdiGraphics;
}

void Canvas::EndGdiplusContext()
{
	m_GdiGraphics.release();
	ReleaseDC();
}

HDC Canvas::GetDC()
{
	if (m_IsDrawing)
	{
		m_EnableDrawAfterGdi = true;
		m_IsDrawing = false;
		EndDraw();
	}

	HDC hdc;
	HRESULT hr = m_BackBuffer->GetDC(FALSE, &hdc);
	if (FAILED(hr)) return nullptr;

	return hdc;
}

void Canvas::ReleaseDC()
{
	m_BackBuffer->ReleaseDC(NULL);

	if (m_EnableDrawAfterGdi)
	{
		m_EnableDrawAfterGdi = false;
		m_IsDrawing = true;
		BeginDraw();
	}
}

bool Canvas::IsTransparentPixel(int x, int y)
{
	if (!(x >= 0 && y >= 0 && x < m_W && y < m_H)) return false;

	D2D1_MAPPED_RECT data;
	HRESULT hr = m_BufferSnapshot->Map(D2D1_MAP_OPTIONS_READ, &data);
	if (FAILED(hr)) true;

	int index = 4 * (y * m_W + x);
	BYTE pixel = data.bits[index + 3];

	hr = m_BufferSnapshot->Unmap();
	if (FAILED(hr))
	{
		// Error
	}

	return pixel != 0;
}

void Canvas::SetTransform(const Gdiplus::Matrix& matrix)
{
	D2D1_MATRIX_3X2_F d2dMatrix;
	matrix.GetElements((Gdiplus::REAL*)&d2dMatrix);
	m_Target->SetTransform(d2dMatrix);

	UpdateGdiTransform();

	m_CanUseAxisAlignClip =
		d2dMatrix._12 == 0.0f && d2dMatrix._21 == 0.0f &&
		d2dMatrix._31 == 0.0f && d2dMatrix._32 == 0.0f;
}

void Canvas::ResetTransform()
{
	m_Target->SetTransform(D2D1::Matrix3x2F::Identity());

	if (m_GdiGraphics)
	{
		m_GdiGraphics->ResetTransform();
	}
}

void Canvas::RotateTransform(float angle, float x, float y, float dx, float dy)
{
	D2D1::Matrix3x2F transform = D2D1::Matrix3x2F::Identity();
	m_Target->GetTransform(&transform);
	transform.Rotation(angle, D2D1::Point2F(x, y));
	transform.Translation(dx, dy);
	m_Target->SetTransform(transform);

	UpdateGdiTransform();
}

void Canvas::PushClip(Gfx::Shape* clip)
{
	Microsoft::WRL::ComPtr<ID2D1Layer> layer;
	m_Target->CreateLayer(layer.GetAddressOf());
	m_Target->PushLayer(D2D1::LayerParameters1(D2D1::InfiniteRect(), clip->m_Shape.Get(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE), layer.Get());
	m_Layers.push(layer);
}

void Canvas::PopClip()
{
	m_Target->PopLayer();
	if (m_Layers.top())
	{
		m_Layers.pop();
	}

}

void Canvas::SetAntiAliasing(bool enable)
{
	if (m_GdiGraphics)
	{
		m_GdiGraphics->SetSmoothingMode(
			enable ? Gdiplus::SmoothingModeHighQuality : Gdiplus::SmoothingModeNone);
		m_GdiGraphics->SetPixelOffsetMode(
			enable ? Gdiplus::PixelOffsetModeHighQuality : Gdiplus::PixelOffsetModeDefault);
	}

	m_Target->SetAntialiasMode(enable ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
}

void Canvas::SetTextAntiAliasing(bool enable)
{
	// TODO: Add support for D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE?
	m_TextAntiAliasing = enable;
	m_Target->SetTextAntialiasMode(enable ? D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE : D2D1_TEXT_ANTIALIAS_MODE_ALIASED);
}

void Canvas::Clear(const Gdiplus::Color& color)
{
	if (!m_Target) return;

	m_Target->Clear(Util::ToColorF(color));
}

void Canvas::DrawTextW(const std::wstring& srcStr, const TextFormat& format, Gdiplus::RectF& rect,
	const Gdiplus::SolidBrush& brush, bool applyInlineFormatting)
{
	Gdiplus::Color color;
	brush.GetColor(&color);

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> solidBrush;
	HRESULT hr = m_Target->CreateSolidColorBrush(Util::ToColorF(color), solidBrush.GetAddressOf());
	if (FAILED(hr)) return;

	TextFormatD2D& formatD2D = (TextFormatD2D&)format;

	static std::wstring str;
	str = srcStr;
	formatD2D.ApplyInlineCase(str);

	if (!formatD2D.CreateLayout(m_Target.Get(), str, rect.Width, rect.Height, !m_AccurateText && m_TextAntiAliasing)) return;

	D2D1_POINT_2F drawPosition;
	drawPosition.x = [&]()
	{
		if (!m_AccurateText)
		{
			const float xOffset = formatD2D.m_TextFormat->GetFontSize() / 6.0f;
			switch (formatD2D.GetHorizontalAlignment())
			{
			case HorizontalAlignment::Left: return rect.X + xOffset;
			case HorizontalAlignment::Right: return rect.X - xOffset;
			}
		}

		return rect.X;
	} ();

	drawPosition.y = [&]()
	{
		// GDI+ compatibility.
		float yPos = rect.Y - formatD2D.m_LineGap;
		switch (formatD2D.GetVerticalAlignment())
		{
		case VerticalAlignment::Bottom: yPos -= formatD2D.m_ExtraHeight; break;
		case VerticalAlignment::Center: yPos -= formatD2D.m_ExtraHeight / 2; break;
		}

		return yPos;
	} ();

	if (formatD2D.m_Trimming)
	{
		D2D1_RECT_F clipRect = Util::ToRectF(rect);

		if (m_CanUseAxisAlignClip)
		{
			m_Target->PushAxisAlignedClip(clipRect, D2D1_ANTIALIAS_MODE_ALIASED);
		}
		else
		{
			const D2D1_LAYER_PARAMETERS1 layerParams =
				D2D1::LayerParameters1(clipRect, nullptr, D2D1_ANTIALIAS_MODE_ALIASED);
			m_Target->PushLayer(layerParams, nullptr);
		}
	}

	// When different "effects" are used with inline coloring options, we need to
	// remove the previous inline coloring, then reapply them (if needed) - instead
	// of destroying/recreating the text layout.
	UINT32 strLen = (UINT32)str.length();
	formatD2D.ResetInlineColoring(solidBrush.Get(), strLen);
	if (applyInlineFormatting)
	{
		formatD2D.ApplyInlineColoring(m_Target.Get(), &drawPosition);

		// Draw any 'shadow' effects
		formatD2D.ApplyInlineShadow(m_Target.Get(), solidBrush.Get(), strLen, drawPosition);
	}

	m_Target->DrawTextLayout(drawPosition, formatD2D.m_TextLayout.Get(), solidBrush.Get());

	if (applyInlineFormatting)
	{
		// Inline gradients require the drawing position, so in case that position
		// changes, we need a way to reset it after drawing time so on the next
		// iteration it will know the correct position.
		formatD2D.ResetGradientPosition(&drawPosition);
	}

	if (formatD2D.m_Trimming)
	{
		if (m_CanUseAxisAlignClip)
		{
			m_Target->PopAxisAlignedClip();
		}
		else
		{
			m_Target->PopLayer();
		}
	}
}

bool Canvas::MeasureTextW(const std::wstring& str, const TextFormat& format, Gdiplus::RectF& rect)
{
	TextFormatD2D& formatD2D = (TextFormatD2D&)format;

	static std::wstring formatStr;
	formatStr = str;
	formatD2D.ApplyInlineCase(formatStr);

	const DWRITE_TEXT_METRICS metrics = formatD2D.GetMetrics(formatStr, !m_AccurateText);
	rect.Width = metrics.width;
	rect.Height = metrics.height;
	return true;
}

bool Canvas::MeasureTextLinesW(const std::wstring& str, const TextFormat& format, Gdiplus::RectF& rect, UINT& lines)
{
	TextFormatD2D& formatD2D = (TextFormatD2D&)format;
	formatD2D.m_TextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

	static std::wstring formatStr;
	formatStr = str;
	formatD2D.ApplyInlineCase(formatStr);

	const DWRITE_TEXT_METRICS metrics = formatD2D.GetMetrics(formatStr, !m_AccurateText, rect.Width);
	rect.Width = metrics.width;
	rect.Height = metrics.height;
	lines = metrics.lineCount;

	if (rect.Height > 0.0f)
	{
		// GDI+ draws multi-line text even though the last line may be clipped slightly at the
		// bottom. This is a workaround to emulate that behaviour.
		rect.Height += 1.0f;
	}
	else
	{
		// GDI+ compatibility: Zero height text has no visible lines.
		lines = 0;
	}
	return true;
}

void Canvas::DrawBitmap(Gdiplus::Bitmap* bitmap, const Gdiplus::Rect& dstRect, const Gdiplus::Rect& srcRect)
{
	Microsoft::WRL::ComPtr<ID2D1Bitmap> d2dBitmap = ConvertBitmap(bitmap);
	if (!d2dBitmap) return;

	auto rDst = Util::ToRectF(dstRect);
	auto rSrc = Util::ToRectF(srcRect);
	m_Target->DrawBitmap(d2dBitmap.Get(), rDst, 1.0F, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, rSrc);
}

void Canvas::DrawBitmap(const D2DBitmap* bitmap, const Gdiplus::Rect& dstRect, const Gdiplus::Rect& srcRect)
{
	auto& segments = bitmap->m_Segments;
	const auto width = bitmap->m_Width;
	const auto height = bitmap->m_Height;

	auto getRectSubregion = [&](const D2D1_RECT_F& source, const Gdiplus::Rect& dst)
	{
		return D2D1_RECT_F {
			source.left / width * dst.Width + dst.X,
			source.top / height * dst.Height + dst.Y,
			source.right / width * dst.Width + dst.X,
			source.bottom / height * dst.Height + dst.Y,
		};
	};

	for (auto it = segments.begin(); it != segments.end(); ++it)
	{
		D2D1_RECT_F rSeg = { (FLOAT)it->m_X, (FLOAT)it->m_Y, (FLOAT)it->m_Width, (FLOAT)it->m_Height };
		auto rSrc = getRectSubregion(rSeg, srcRect);
		auto rDst = getRectSubregion(rSeg, dstRect);
		m_Target->DrawBitmap(it->m_Bitmap.Get(), rDst, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &rSrc);
	}
}

void Canvas::DrawMaskedBitmap(Gdiplus::Bitmap* bitmap, Gdiplus::Bitmap* maskBitmap, const Gdiplus::Rect& dstRect,
	const Gdiplus::Rect& srcRect, const Gdiplus::Rect& srcRect2)
{
	auto rDst = Util::ToRectF(dstRect);
	auto rSrc = Util::ToRectF(srcRect);

	Microsoft::WRL::ComPtr<ID2D1Bitmap> d2dBitmap = ConvertBitmap(bitmap);
	Microsoft::WRL::ComPtr<ID2D1Bitmap> d2dMaskBitmap = ConvertBitmap(maskBitmap);
	
	if (!d2dBitmap || !d2dMaskBitmap) return;

	// Create bitmap brush from original |bitmap|.
	Microsoft::WRL::ComPtr<ID2D1BitmapBrush> brush;
	D2D1_BITMAP_BRUSH_PROPERTIES propertiesXClampYClamp = D2D1::BitmapBrushProperties(
		D2D1_EXTEND_MODE_CLAMP,
		D2D1_EXTEND_MODE_CLAMP,
		D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);

	// "Move" and "scale" the |bitmap| to match the destination.
	D2D1_MATRIX_3X2_F translate = D2D1::Matrix3x2F::Translation(rDst.left, rDst.top);
	D2D1_MATRIX_3X2_F scale = D2D1::Matrix3x2F::Scale(
		D2D1::SizeF((rDst.right - rDst.left) / (float)srcRect2.Width, (rDst.bottom - rDst.top) / (float)srcRect2.Height));
	D2D1_BRUSH_PROPERTIES brushProps = D2D1::BrushProperties(1.0F, scale * translate);

	HRESULT hr = m_Target->CreateBitmapBrush(
		d2dBitmap.Get(),
		propertiesXClampYClamp,
		brushProps,
		brush.GetAddressOf());
	if (FAILED(hr)) return;

	auto aaMode = m_Target->GetAntialiasMode();
	m_Target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // required
	m_Target->FillOpacityMask(
		d2dMaskBitmap.Get(),
		brush.Get(),
		D2D1_OPACITY_MASK_CONTENT_GRAPHICS,
		&rDst,
		&rSrc);
	m_Target->SetAntialiasMode(aaMode);
}

void Canvas::FillRectangle(Gdiplus::Rect& rect, const Gdiplus::SolidBrush& brush)
{
	Gdiplus::Color color;
	brush.GetColor(&color);

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> solidBrush;
	HRESULT hr = m_Target->CreateSolidColorBrush(Util::ToColorF(color), solidBrush.GetAddressOf());
	if (SUCCEEDED(hr))
	{
		m_Target->FillRectangle(Util::ToRectF(rect), solidBrush.Get());
	}
}

void Canvas::DrawGeometry(Shape& shape, int xPos, int yPos)
{
	D2D1_MATRIX_3X2_F worldTransform;
	m_Target->GetTransform(&worldTransform);
	m_Target->SetTransform(
		shape.GetShapeMatrix() *
		D2D1::Matrix3x2F::Translation((FLOAT)xPos, (FLOAT)yPos) *
		worldTransform
	);

	if (shape.m_FillColor.a > 0.0f)
	{
		auto brush = shape.GetFillBrush(m_Target.Get());
		m_Target->FillGeometry(shape.m_Shape.Get(), brush.Get());
	}

	if (shape.m_StrokeColor.a > 0.0f && shape.m_StrokeWidth > 0.0f)
	{
		auto brush = shape.GetStrokeFillBrush(m_Target.Get());
		m_Target->DrawGeometry(
			shape.m_Shape.Get(),
			brush.Get(),
			shape.m_StrokeWidth,
			shape.m_StrokeStyle.Get());
	}

	m_Target->SetTransform(worldTransform);
}

HRESULT Canvas::CreateRenderTarget()
{
	HRESULT hr = E_FAIL;
	if (c_D2DDevice)
	{
		c_D2DDevice->ClearResources();

		hr = c_D2DDevice->CreateDeviceContext(
			D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS,
			m_Target.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			hr = c_D2DDevice->CreateDeviceContext(
				D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
				m_Target.ReleaseAndGetAddressOf());
		}
	}

	// Hardware accelerated targets have a hard limit to the size of bitmaps they can support.
	// The size will depend on the D3D feature level of the driver used. The WARP software
	// renderer has a limit of 16MP (16*1024*1024 = 16777216).

	// https://msdn.microsoft.com/en-us/library/windows/desktop/ff476876(v=vs.85).aspx#Overview
	// Max Texture Dimension
	// D3D_FEATURE_LEVEL_11_0 = 16348
	// D3D_FEATURE_LEVEL_10_1 = 8192
	// D3D_FEATURE_LEVEL_10_0 = 8192
	// D3D_FEATURE_LEVEL_9_3  = 4096
	// D3D_FEATURE_LEVEL_9_2  = 2048
	// D3D_FEATURE_LEVEL_9_1  = 2048

	if (SUCCEEDED(hr))
	{
		m_MaxBitmapSize = m_Target->GetMaximumBitmapSize();
	}

	return hr;
}

bool Canvas::CreateTargetBitmap(UINT32 width, UINT32 height)
{
	HRESULT hr = m_SwapChain->GetBuffer(0, IID_PPV_ARGS(m_BackBuffer.GetAddressOf()));
	if (FAILED(hr)) return false;

	D2D1_BITMAP_PROPERTIES1 bProps = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

	hr = m_Target->CreateBitmapFromDxgiSurface(
		m_BackBuffer.Get(),
		&bProps,
		m_TargetBitmap.GetAddressOf());
	if (FAILED(hr)) return false;

	bProps.bitmapOptions = D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_CPU_READ;
	hr = m_Target->CreateBitmap(
		D2D1::SizeU(width, height),
		nullptr,
		0U,
		bProps,
		m_BufferSnapshot.ReleaseAndGetAddressOf());
	if (FAILED(hr)) return false;

	m_Target->SetTarget(m_TargetBitmap.Get());
	return true;
}

Microsoft::WRL::ComPtr<ID2D1Bitmap> Canvas::ConvertBitmap(Gdiplus::Bitmap* bitmap)
{
	UINT32 width = (UINT32)bitmap->GetWidth();
	UINT32 height = (UINT32)bitmap->GetHeight();

	// Truncate the bitmap if the size it too big.
	if (width > m_MaxBitmapSize) width = m_MaxBitmapSize;
	if (height > m_MaxBitmapSize) height = m_MaxBitmapSize;

	Microsoft::WRL::ComPtr<ID2D1Bitmap> d2dBitmap;

	Util::WICBitmapLockGDIP* bitmapLock = new Util::WICBitmapLockGDIP();
	Gdiplus::Rect lockRect(0, 0, (UINT)width, (UINT)height);
	Gdiplus::Status status = bitmap->LockBits(
		&lockRect, Gdiplus::ImageLockModeRead, PixelFormat32bppPARGB, bitmapLock->GetBitmapData());

	if (status == Gdiplus::Ok)
	{
		D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

		auto size = D2D1::SizeU(width, height);

		HRESULT hr = m_Target->CreateBitmap(size, props, d2dBitmap.GetAddressOf());
		if (SUCCEEDED(hr))
		{
			BYTE* data = nullptr;
			UINT bufferSize = 0u;
			hr = bitmapLock->GetDataPointer(&bufferSize, &data);
			if (SUCCEEDED(hr))
			{
				UINT stride = 0u;
				hr = bitmapLock->GetStride(&stride);
				if (SUCCEEDED(hr))
				{
					hr = d2dBitmap->CopyFromMemory(nullptr, data, (UINT32)stride);
					if (FAILED(hr))
					{
						d2dBitmap.Reset();
					}
				}
			}
		}

		// D2D will still use the pixel data after this call (at the next Flush() or EndDraw()).
		bitmap->UnlockBits(bitmapLock->GetBitmapData());
	}

	bitmapLock->Release();

	return d2dBitmap;
}

void Canvas::UpdateGdiTransform()
{
	if (!m_GdiGraphics || !m_Target) return;

	D2D1_MATRIX_3X2_F transform;
	m_Target->GetTransform(&transform);
	
	Gdiplus::Matrix matrix(
		transform._11,
		transform._12,
		transform._21,
		transform._22,
		transform._31,
		transform._32);

	m_GdiGraphics->SetTransform(&matrix);
}

}  // namespace Gfx
