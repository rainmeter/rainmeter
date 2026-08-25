// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "Canvas.h"
#include "TextFormat.h"
#include "Bitmap.h"
#include "Svg.h"
#include "RenderTexture.h"
#include "Util/D2DUtil.h"
#include "Util/DWriteFontCollectionLoader.h"
#include "../Platform.h"

#include <dxgidebug.h>

namespace Gfx {

const DXGI_SWAP_CHAIN_DESC1 g_SwapChainDesc =
{
	.Width = 1,
	.Height = 1,
	.Format = DXGI_FORMAT_B8G8R8A8_UNORM,
	.Stereo = false,
	.SampleDesc = { .Count = 1, .Quality = 0 },
	.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
	.BufferCount = 1,
	.Scaling = DXGI_SCALING_STRETCH,
	.SwapEffect = DXGI_SWAP_EFFECT_DISCARD,
	.AlphaMode = DXGI_ALPHA_MODE_IGNORE,
	.Flags = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE,
};

bool Canvas::c_HardwareAccelerated = true;
Canvas::DeviceLostCallback Canvas::c_DeviceLostCallback = nullptr;
Microsoft::WRL::ComPtr<ID3D11Device> Canvas::c_D3DDevice;
Microsoft::WRL::ComPtr<ID3D11DeviceContext> Canvas::c_D3DContext;
Microsoft::WRL::ComPtr<ID2D1Device> Canvas::c_D2DDevice;
Microsoft::WRL::ComPtr<ID2D1DeviceContext5> Canvas::c_EffectTarget;
Microsoft::WRL::ComPtr<IDXGIDevice1> Canvas::c_DxgiDevice;
Microsoft::WRL::ComPtr<ID2D1Factory1> Canvas::c_D2DFactory;
Microsoft::WRL::ComPtr<IDWriteFactory1> Canvas::c_DWFactory;
Microsoft::WRL::ComPtr<IWICImagingFactory> Canvas::c_WICFactory;

Canvas::Canvas() :
	m_W(0),
	m_H(0),
	m_Dpi(96.0f),
	m_MaxBitmapSize(0),
	m_ValidDeviceContext(false),
	m_IsDrawing(false),
	m_EnableDrawAfterGdi(false),
	m_TextAntiAliasing(false),
	m_CanUseAxisAlignClip(true)
{
}

Canvas::~Canvas()
{
}

bool Canvas::Initialize(bool hardwareAccelerated, DeviceLostCallback deviceLostCallback)
{
	c_HardwareAccelerated = hardwareAccelerated;
	c_DeviceLostCallback = deviceLostCallback;

	D2D1_FACTORY_OPTIONS fo = {};
	const bool debug = false;
	if (debug)
	{
		fo.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
	}

	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, fo, c_D2DFactory.GetAddressOf());
	if (FAILED(hr)) return false;

	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(c_DWFactory), (IUnknown**)c_DWFactory.GetAddressOf());
	if (FAILED(hr)) return false;

	hr = c_DWFactory->RegisterFontCollectionLoader(Util::DWriteFontCollectionLoader::GetInstance());
	if (FAILED(hr)) return false;

	hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(c_WICFactory.GetAddressOf()));
	if (FAILED(hr)) return false;

	return AttachDevice();
}

bool Canvas::AttachDevice()
{
	c_EffectTarget.Reset();
	c_D2DDevice.Reset();
	c_DxgiDevice.Reset();
	c_D3DContext.Reset();
	c_D3DDevice.Reset();

	// Required for Direct2D interopability.
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
	// The D3D11 debug layer is not available to x64 processes emulated on ARM64.
	if (!GetPlatform().IsEmulatedOnArm64())
	{
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}
#endif

	auto tryCreateDevice = [&](D3D_DRIVER_TYPE driverType, const D3D_FEATURE_LEVEL* levels, UINT numLevels)
		{
			D3D_FEATURE_LEVEL deviceFeatureLevel;
			return D3D11CreateDevice(
				nullptr,
				driverType,
				nullptr,
				creationFlags,
				levels,
				numLevels,
				D3D11_SDK_VERSION,
				c_D3DDevice.ReleaseAndGetAddressOf(),
				&deviceFeatureLevel,
				c_D3DContext.ReleaseAndGetAddressOf());
		};

	const D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	HRESULT hr = E_FAIL;
	if (c_HardwareAccelerated)
	{
		hr = tryCreateDevice(D3D_DRIVER_TYPE_HARDWARE, levels, _countof(levels));
		if (hr == E_INVALIDARG)
		{
			hr = tryCreateDevice(D3D_DRIVER_TYPE_HARDWARE, &levels[1], _countof(levels) - 1);
		}
	}

	if (FAILED(hr))
	{
		// Fallback to software renderer if hardware acceleration is not available.
		hr = tryCreateDevice(D3D_DRIVER_TYPE_WARP, nullptr, 0);
		if (FAILED(hr)) return false;
	}

	hr = c_D3DDevice.As(&c_DxgiDevice);
	if (FAILED(hr)) return false;

	hr = c_D2DFactory->CreateDevice(c_DxgiDevice.Get(), c_D2DDevice.ReleaseAndGetAddressOf());
	if (FAILED(hr)) return false;

	c_EffectTarget = CreateDeviceContext();
	if (!c_EffectTarget) return false;

	return true;
}

ComPtr<ID2D1DeviceContext5> Canvas::CreateDeviceContext()
{
	if (!c_D2DDevice) return nullptr;

	ComPtr<ID2D1DeviceContext> deviceContext;
	auto hr = c_D2DDevice->CreateDeviceContext(
		D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS,
		deviceContext.GetAddressOf());
	if (FAILED(hr))
	{
		hr = c_D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, deviceContext.GetAddressOf());
	}
	if (FAILED(hr)) return nullptr;

	ComPtr<ID2D1DeviceContext5> deviceContext5;
	deviceContext.As(&deviceContext5);
	return deviceContext5;
}

void Canvas::Finalize()
{
// Dump extra dxgi debugging information (if needed)
// On the following line, change |FALSE| to |TRUE|
#if defined(_DEBUG) && FALSE
	// More info: https://docs.microsoft.com/en-us/windows/win32/api/dxgidebug/nf-dxgidebug-dxgigetdebuginterface
	typedef HRESULT(__stdcall* fDebugInterface)(const IID&, void**);
	HMODULE hDll = GetModuleHandle(L"Dxgidebug.dll");
	if (hDll)
	{
		fDebugInterface DXGIGetDebugInterface = (fDebugInterface)GetProcAddress(hDll, "DXGIGetDebugInterface");
		IDXGIDebug* pDxgiDebug = nullptr;
		HRESULT hr = DXGIGetDebugInterface(__uuidof(IDXGIDebug), (void**)&pDxgiDebug);
		if (SUCCEEDED(hr))
		{
			pDxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);  // Use |DXGI_DEBUG_RLO_SUMMARY| if needed
			pDxgiDebug->Release();
		}
	}
#endif

	if (c_DWFactory)
	{
		c_DWFactory->UnregisterFontCollectionLoader(Util::DWriteFontCollectionLoader::GetInstance());
	}

	c_EffectTarget.Reset();
	c_D2DDevice.Reset();
	c_DxgiDevice.Reset();
	c_D3DContext.Reset();
	c_D3DDevice.Reset();

	c_D2DFactory.Reset();
	c_DWFactory.Reset();
	c_WICFactory.Reset();
}

bool Canvas::EnumerateInstalledFontFamilies(UINT32& familyCount, std::wstring& families)
{
	bool success = false;
	FontCollection* collection = new FontCollection();
	collection->InitializeCollection();

	success = collection->GetSystemFontFamilies(familyCount, families);

	if (collection)
	{
		delete collection;
		collection = nullptr;
	}

	return success;
}

HRESULT Canvas::InitializeDeviceContextForWindow(HWND window)
{
	ComPtr<ID2D1DeviceContext5> target = CreateDeviceContext();
	if (!target) return E_FAIL;

	ComPtr<IDXGIAdapter> dxgiAdapter;
	HRESULT hr = c_DxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());
	if (FAILED(hr)) return hr;

	// Ensure that DXGI does not queue more than one frame at a time.
	hr = c_DxgiDevice->SetMaximumFrameLatency(1);
	if (FAILED(hr)) return hr;

	ComPtr<IDXGIFactory2> dxgiFactory;
	hr = dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));
	if (FAILED(hr)) return hr;

	decltype(m_SwapChain) swapChain;
	hr = dxgiFactory->CreateSwapChainForHwnd(c_DxgiDevice.Get(), window, &g_SwapChainDesc, nullptr, nullptr, swapChain.GetAddressOf());
	if (FAILED(hr)) return hr;

	// Prevent DXGI from monitoring window changes through "alt + enter" (full screen mode)
	dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);

	m_Target = std::move(target);
	m_SwapChain = std::move(swapChain);
	Resize(m_W, m_H);

	m_SolidColorBrushCache.clear();
	m_ValidDeviceContext = true;
	return hr;
}

bool Canvas::Resize(int w, int h)
{
	if (!m_SwapChain || !m_Target) return false;

	// Truncate the size of the skin if it's too big.
	m_MaxBitmapSize = m_Target->GetMaximumBitmapSize();
	m_W = min(w, (int)m_MaxBitmapSize);
	m_H = min(h, (int)m_MaxBitmapSize);

	m_Target->SetTarget(nullptr);
	m_TargetBitmap.Reset();
	m_BackBuffer.Reset();

	const auto dxgiFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
	HRESULT hr = m_SwapChain->ResizeBuffers(0, m_W, m_H, g_SwapChainDesc.Format, g_SwapChainDesc.Flags);
	if (FAILED(hr))
	{
		HWND window = nullptr;
		ComPtr<IDXGIAdapter> dxgiAdapter;
		ComPtr<IDXGIFactory2> dxgiFactory;

		hr = m_SwapChain->GetHwnd(&window);
		if (FAILED(hr)) return false;

		hr = c_DxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());
		if (FAILED(hr)) return false;

		hr = dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));
		if (FAILED(hr)) return false;

		dxgiFactory->CreateSwapChainForHwnd(c_DxgiDevice.Get(), window, &g_SwapChainDesc, nullptr, nullptr, m_SwapChain.ReleaseAndGetAddressOf());

		// Even if the swapchain was recreated, we return false here. The recreate is needed to avoid
		// keeping a now invalid swapchain around.
		return false;
	}

	hr = m_SwapChain->GetBuffer(0, IID_PPV_ARGS(m_BackBuffer.GetAddressOf()));
	if (FAILED(hr)) return false;

	const auto props = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(dxgiFormat, D2D1_ALPHA_MODE_PREMULTIPLIED));
	hr = m_Target->CreateBitmapFromDxgiSurface(m_BackBuffer.Get(), &props, m_TargetBitmap.GetAddressOf());
	if (FAILED(hr)) return false;

	m_Target->SetTarget(m_TargetBitmap.Get());
	m_Target->SetDpi(m_Dpi, m_Dpi);
	return true;
}

bool Canvas::BeginDraw()
{
	if (!m_ValidDeviceContext || !m_TargetBitmap)
	{
		m_IsDrawing = false;
		return false;
	}

	m_Target->BeginDraw();
	m_IsDrawing = true;
	return true;
}

void Canvas::EndDraw()
{
	const auto hr = m_Target->EndDraw();
	if (hr == D2DERR_RECREATE_TARGET)
	{
		if (m_ValidDeviceContext && c_DeviceLostCallback)
		{
			c_DeviceLostCallback();
		}

		m_ValidDeviceContext = false;
	}

	m_IsDrawing = false;
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
	if (m_BackBuffer && SUCCEEDED(m_BackBuffer->GetDC(FALSE, &hdc)))
	{
		return hdc;
	}

	return nullptr;
}

void Canvas::ReleaseDC()
{
	if (m_BackBuffer)
	{
		RECT dirtyRect = { 0, 0, 0, 0 };
		m_BackBuffer->ReleaseDC(&dirtyRect);
	}

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

	auto hdc = GetDC();
	if (hdc)
	{
		const auto pixel = GetPixel(hdc, x, y);
		ReleaseDC();
		return (pixel & 0xFF000000) == 0;
	}

	return false;
}

void Canvas::GetTransform(D2D1_MATRIX_3X2_F* matrix)
{
	if (m_Target)
	{
		m_Target->GetTransform(matrix);
	}
	else
	{
		*matrix = D2D1::Matrix3x2F::Identity();
	}
}

void Canvas::SetTransform(const D2D1_MATRIX_3X2_F& matrix)
{
	m_Target->SetTransform(matrix);

	m_CanUseAxisAlignClip =
		(matrix.m11 ==  1.0f && matrix.m12 ==  0.0f && matrix.m21 ==  0.0f && matrix.m22 ==  1.0f) ||	// Angle: 0
		(matrix.m11 ==  0.0f && matrix.m12 ==  1.0f && matrix.m21 == -1.0f && matrix.m22 ==  0.0f) ||	// Angle: 90
		(matrix.m11 == -1.0f && matrix.m12 ==  0.0f && matrix.m21 ==  0.0f && matrix.m22 == -1.0f) ||	// Angle: 180
		(matrix.m11 ==  0.0f && matrix.m12 == -1.0f && matrix.m21 ==  1.0f && matrix.m22 ==  0.0f);		// Angle: 270
}

void Canvas::ResetTransform()
{
	SetTransform(D2D1::Matrix3x2F::Identity());
}

void Canvas::PushOpacityLayer(FLOAT opacity)
{
	if (!m_Target) return;

	m_Target->PushLayer(
		D2D1::LayerParameters(
			D2D1::InfiniteRect(),
			nullptr,
			D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
			D2D1::Matrix3x2F::Identity(),
			opacity),
		nullptr);
}

void Canvas::PushClip(const D2D1_RECT_F& rect)
{
	if (m_CanUseAxisAlignClip)
	{
		m_Target->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_ALIASED);
	}
	else
	{
		const D2D1_LAYER_PARAMETERS1 layerParams =
			D2D1::LayerParameters1(rect, nullptr, D2D1_ANTIALIAS_MODE_ALIASED);
		m_Target->PushLayer(layerParams, nullptr);
	}
}

void Canvas::PopClip()
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

void Canvas::PopLayer()
{
	if (!m_Target) return;

	m_Target->PopLayer();
}

void Canvas::SetDpiScale(float dpiScale)
{
	auto dpi = 96.0f * dpiScale;
	if (dpi <= 0.0f)
	{
		dpi = 96.0f;
	}

	m_Dpi = dpi;
	m_Target->SetDpi(m_Dpi, m_Dpi);
}

FLOAT Canvas::SnapToPixel(FLOAT value) const
{
	const FLOAT scale = m_Dpi / 96.0f;
	return roundf(value * scale) / scale;
}

bool Canvas::SetTarget(RenderTexture* texture)
{
	auto bitmap = texture->GetBitmap();
	if (bitmap->m_Segments.size() == 0) return false;

	auto image = bitmap->m_Segments[0].GetBitmap();
	m_Target->SetTarget(image);
	m_Target->SetDpi(m_Dpi, m_Dpi);
	return true;
}

void Canvas::ResetTarget()
{
	m_Target->SetTarget(m_TargetBitmap.Get());
	m_Target->SetDpi(m_Dpi, m_Dpi);
}

void Canvas::SetAntiAliasing(bool enable)
{
	m_Target->SetAntialiasMode(enable ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
}

void Canvas::SetTextAntiAliasing(bool enable)
{
	// TODO: Add support for D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE?
	m_TextAntiAliasing = enable;
	m_Target->SetTextAntialiasMode(enable ? D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE : D2D1_TEXT_ANTIALIAS_MODE_ALIASED);
}

void Canvas::Clear(const D2D1_COLOR_F& color)
{
	if (!m_Target) return;

	m_Target->Clear(color);
}

D2D1_POINT_2F Canvas::GetTextDrawPosition(TextFormat& format, const D2D1_RECT_F& rect) const
{
	D2D1_POINT_2F drawPosition = D2D1::Point2F();
	drawPosition.x = [&]()
	{
		if (!m_AccurateText)
		{
			const float xOffset = format.m_TextFormat->GetFontSize() / 6.0f;
			switch (format.GetHorizontalAlignment())
			{
			// Justified text starts at the leading edge, just like left aligned text.
			case HorizontalAlignment::Left:
			case HorizontalAlignment::Justify: return rect.left + xOffset;
			case HorizontalAlignment::Right: return rect.left - xOffset;
			}
		}

		return rect.left;
	} ();

	drawPosition.y = [&]()
	{
		// GDI+ compatibility.
		float yPos = rect.top - format.m_LineGap;
		switch (format.GetVerticalAlignment())
		{
		case VerticalAlignment::Bottom: yPos -= format.m_ExtraHeight; break;
		case VerticalAlignment::Center: yPos -= format.m_ExtraHeight / 2.0f; break;
		}

		return yPos;
	} ();

	return drawPosition;
}

bool Canvas::PrepareTextLayout(const std::wstring& srcStr, TextFormat& format,
	const D2D1_RECT_F& rect, D2D1_POINT_2F& drawPosition, UINT32& strLen)
{
	if (!m_Target || !format.IsInitialized()) return false;

	std::wstring buffer;
	const std::wstring& str = format.ApplyInlineCase(srcStr, buffer);

	if (!format.CreateLayout(
		m_Target.Get(),
		str,
		rect.right - rect.left,
		rect.bottom - rect.top,
		!m_AccurateText && m_TextAntiAliasing)) return false;

	drawPosition = GetTextDrawPosition(format, rect);
	strLen = (UINT32)str.length();
	return true;
}

bool Canvas::HitTestTextPoint(const std::wstring& srcStr, TextFormat& format,
	const D2D1_RECT_F& rect, const D2D1_POINT_2F& point, UINT32& caretIndex, bool& isTrailing)
{
	D2D1_POINT_2F drawPosition = D2D1::Point2F();
	UINT32 strLen = 0U;
	if (!PrepareTextLayout(srcStr, format, rect, drawPosition, strLen)) return false;

	BOOL isTrailingHit = FALSE;
	BOOL isInside = FALSE;
	DWRITE_HIT_TEST_METRICS metrics = { 0 };
	const HRESULT hr = format.m_TextLayout->HitTestPoint(
		point.x - drawPosition.x, point.y - drawPosition.y, &isTrailingHit, &isInside, &metrics);
	if (FAILED(hr)) return false;

	// Snapping to the far side of the hit cluster (rather than to the hit code unit) is what keeps
	// the caret from landing inside a surrogate pair or between a base character and its
	// combining marks.
	caretIndex = isTrailingHit ? metrics.textPosition + metrics.length : metrics.textPosition;
	caretIndex = min(caretIndex, strLen);
	isTrailing = isTrailingHit != FALSE;
	return true;
}

bool Canvas::GetCaretRect(const std::wstring& srcStr, TextFormat& format, const D2D1_RECT_F& rect,
	UINT32 caretIndex, bool trailing, FLOAT width, D2D1_RECT_F& caretRect)
{
	D2D1_POINT_2F drawPosition = D2D1::Point2F();
	UINT32 strLen = 0U;
	if (!PrepareTextLayout(srcStr, format, rect, drawPosition, strLen)) return false;

	UINT32 position = min(caretIndex, strLen);
	BOOL isTrailingHit = FALSE;

	// A caret just after a hard line break belongs to the start of the next line, with no affinity
	// to decide. Asking for the trailing edge of the break itself would report the end of the
	// previous line, leaving the caret looking as though the newline never happened.
	const bool afterLineBreak =
		position > 0U && (srcStr[position - 1U] == L'\n' || srcStr[position - 1U] == L'\r');

	if (afterLineBreak)
	{
		// Leading edge at |position|, which DirectWrite places on the new line.
	}
	else if (trailing && position > 0U)
	{
		// The caret belongs to the text before it, so it goes at the trailing edge of the cluster
		// ending at |caretIndex|. Which visual side that is depends on the direction of the run
		// the cluster belongs to, which is what makes this differ from the leading edge below.
		FLOAT clusterX = 0.0f;
		FLOAT clusterY = 0.0f;
		DWRITE_HIT_TEST_METRICS cluster = { 0 };
		if (SUCCEEDED(format.m_TextLayout->HitTestTextPosition(
			position - 1U, FALSE, &clusterX, &clusterY, &cluster)))
		{
			position = cluster.textPosition;
			isTrailingHit = TRUE;
		}
	}
	else if (position >= strLen && strLen > 0U)
	{
		// DirectWrite has no leading edge for a position past the end of the text, so ask for the
		// trailing edge of the last cluster instead.
		position = strLen - 1U;
		isTrailingHit = TRUE;
	}

	FLOAT x = 0.0f;
	FLOAT y = 0.0f;
	DWRITE_HIT_TEST_METRICS metrics = { 0 };
	const HRESULT hr = format.m_TextLayout->HitTestTextPosition(
		position, isTrailingHit, &x, &y, &metrics);
	if (FAILED(hr)) return false;

	// An empty layout can report a zero-height line, which would make the caret invisible.
	FLOAT height = format.m_TextFormat->GetFontSize();
	if (metrics.height > 0.0f)
	{
		// A caret taller than the measured text does not just look wrong: an auto-sized meter is
		// sized from that measurement, so the overhang is scrolled into view and drags the text
		// up to make room for a caret that already fits.
		height = metrics.height - format.GetLineGapAdjustment(srcStr);
		if (height < 1.0f) height = 1.0f;
	}

	caretRect.left = drawPosition.x + x;
	caretRect.top = drawPosition.y + y;
	caretRect.right = caretRect.left + width;
	caretRect.bottom = caretRect.top + height;
	return true;
}

bool Canvas::GetAdjacentCaretIndex(const std::wstring& srcStr, TextFormat& format,
	const D2D1_RECT_F& rect, UINT32 caretIndex, bool forward, UINT32& adjacentIndex)
{
	D2D1_POINT_2F drawPosition = D2D1::Point2F();
	UINT32 strLen = 0U;
	if (!PrepareTextLayout(srcStr, format, rect, drawPosition, strLen)) return false;

	if (forward ? caretIndex >= strLen : caretIndex == 0U)
	{
		adjacentIndex = caretIndex;
		return true;
	}

	// Hit-testing a position reports the cluster containing it, so the neighbouring caret index is
	// that cluster's far side: forward from the cluster at |caretIndex|, backward from the one
	// holding the code unit just before it.
	FLOAT x = 0.0f;
	FLOAT y = 0.0f;
	DWRITE_HIT_TEST_METRICS metrics = { 0 };
	const HRESULT hr = format.m_TextLayout->HitTestTextPosition(
		forward ? caretIndex : caretIndex - 1U, FALSE, &x, &y, &metrics);
	if (FAILED(hr)) return false;

	adjacentIndex = forward ? metrics.textPosition + metrics.length : metrics.textPosition;

	// A cluster that somehow does not straddle the starting point would leave the caret stuck.
	if (forward ? adjacentIndex <= caretIndex : adjacentIndex >= caretIndex)
	{
		adjacentIndex = forward ? caretIndex + 1U : caretIndex - 1U;
	}

	adjacentIndex = min(adjacentIndex, strLen);
	return true;
}

bool Canvas::GetLineRange(const std::wstring& srcStr, TextFormat& format, const D2D1_RECT_F& rect,
	UINT32 caretIndex, UINT32& lineStart, UINT32& lineEnd)
{
	D2D1_POINT_2F drawPosition = D2D1::Point2F();
	UINT32 strLen = 0U;
	if (!PrepareTextLayout(srcStr, format, rect, drawPosition, strLen)) return false;

	// The first call reports how many lines there are; E_NOT_SUFFICIENT_BUFFER is expected here
	// rather than a failure.
	UINT32 lineCount = 0U;
	format.m_TextLayout->GetLineMetrics(nullptr, 0U, &lineCount);
	if (lineCount == 0U) return false;

	std::vector<DWRITE_LINE_METRICS> lines(lineCount);
	const HRESULT hr = format.m_TextLayout->GetLineMetrics(lines.data(), lineCount, &lineCount);
	if (FAILED(hr)) return false;

	caretIndex = min(caretIndex, strLen);

	UINT32 start = 0U;
	for (const auto& line : lines)
	{
		// length includes the newline; newlineLength is how much of it to exclude.
		const UINT32 end = start + line.length;
		if (caretIndex < end || end >= strLen)
		{
			lineStart = start;
			lineEnd = end - line.newlineLength;
			return true;
		}

		start = end;
	}

	lineStart = start;
	lineEnd = strLen;
	return true;
}

bool Canvas::GetTextRangeRects(const std::wstring& srcStr, TextFormat& format,
	const D2D1_RECT_F& rect, UINT32 start, UINT32 length, std::vector<D2D1_RECT_F>& rects)
{
	rects.clear();
	if (length == 0U) return true;

	D2D1_POINT_2F drawPosition = D2D1::Point2F();
	UINT32 strLen = 0U;
	if (!PrepareTextLayout(srcStr, format, rect, drawPosition, strLen)) return false;

	// The first call reports how many metrics the range needs; E_NOT_SUFFICIENT_BUFFER is the
	// expected result rather than a failure.
	UINT32 count = 0U;
	format.m_TextLayout->HitTestTextRange(start, length, drawPosition.x, drawPosition.y,
		nullptr, 0U, &count);
	if (count == 0U) return true;

	std::vector<DWRITE_HIT_TEST_METRICS> metrics(count);
	const HRESULT hr = format.m_TextLayout->HitTestTextRange(start, length,
		drawPosition.x, drawPosition.y, metrics.data(), count, &count);
	if (FAILED(hr)) return false;

	// Trimmed as the caret is, so the highlight and the caret inside it are the same height.
	const FLOAT lineGap = format.GetLineGapAdjustment(srcStr);

	rects.reserve(count);
	for (UINT32 i = 0U; i < count; ++i)
	{
		const auto& m = metrics[i];
		if (m.width <= 0.0f || m.height <= 0.0f) continue;

		const FLOAT height = m.height > lineGap ? m.height - lineGap : m.height;
		rects.push_back(D2D1::RectF(m.left, m.top, m.left + m.width, m.top + height));
	}

	return true;
}

void Canvas::DrawTextW(const std::wstring& srcStr, TextFormat& format, const D2D1_RECT_F& rect,
	const D2D1_COLOR_F& color, bool applyInlineFormatting)
{
	auto solidBrush = GetCachedSolidColorBrush(color);
	if (!solidBrush) return;

	D2D1_POINT_2F drawPosition = D2D1::Point2F();
	UINT32 strLen = 0U;
	if (!PrepareTextLayout(srcStr, format, rect, drawPosition, strLen)) return;

	// When different "effects" are used with inline coloring options, we need to
	// remove the previous inline coloring, then reapply them (if needed) - instead
	// of destroying/recreating the text layout.
	format.ResetInlineColoring(solidBrush.Get(), strLen);
	if (applyInlineFormatting)
	{
		format.ApplyInlineColoring(m_Target.Get(), &drawPosition);

		// Draw any 'shadow' effects
		const D2D1_RECT_F drawRect = D2D1::RectF(
			drawPosition.x, drawPosition.y, rect.right - rect.left, rect.bottom - rect.top);
		format.ApplyInlineShadow(m_Target.Get(), solidBrush.Get(), strLen, drawRect);
	}

	if (format.m_Trimming)
	{
		PushClip(rect);
	}

	m_Target->DrawTextLayout(drawPosition, format.m_TextLayout.Get(), solidBrush.Get());

	if (format.m_Trimming)
	{
		PopClip();
	}

	if (applyInlineFormatting)
	{
		// Inline gradients require the drawing position, so in case that position
		// changes, we need a way to reset it after drawing time so on the next
		// iteration it will know the correct position.
		format.ResetGradientPosition(&drawPosition);
	}
}

bool Canvas::MeasureTextW(const std::wstring& str, TextFormat& format, D2D1_SIZE_F& size)
{
	std::wstring buffer;
	const auto metrics = format.GetMetrics(format.ApplyInlineCase(str, buffer), !m_AccurateText);
	size.width = metrics.width;
	size.height = metrics.height;
	return true;
}

bool Canvas::MeasureTextLinesW(const std::wstring& str, TextFormat& format, D2D1_SIZE_F& size, UINT32& lines)
{
	format.m_TextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

	std::wstring buffer;
	const auto metrics = format.GetMetrics(format.ApplyInlineCase(str, buffer), !m_AccurateText, size.width);
	size.width = metrics.width;
	size.height = metrics.height;
	lines = metrics.lineCount;

	if (size.height > 0.0f)
	{
		// GDI+ draws multi-line text even though the last line may be clipped slightly at the
		// bottom. This is a workaround to emulate that behaviour.
		size.height += 1.0f;
	}
	else
	{
		// GDI+ compatibility: Zero height text has no visible lines.
		lines = 0;
	}
	return true;
}

void Canvas::DrawBitmap(Bitmap* bitmap, const D2D1_RECT_F& dstRect, const D2D1_RECT_F& srcRect)
{
	for (auto& segment : bitmap->m_Segments)
	{
		const auto& rSeg = segment.GetRect();
		D2D1_RECT_F rSrc = (rSeg.left < rSeg.right && rSeg.top < rSeg.bottom) ?
			D2D1::RectF(
				max(rSeg.left, srcRect.left),
				max(rSeg.top, srcRect.top),
				min(rSeg.right + rSeg.left, srcRect.right),
				min(rSeg.bottom + rSeg.top, srcRect.bottom)) :
			D2D1::RectF();
		if (rSrc.left == rSrc.right || rSrc.top == rSrc.bottom) continue;

		const D2D1_RECT_F rDst = D2D1::RectF(
			(rSrc.left   - srcRect.left) / (srcRect.right  - srcRect.left) * (dstRect.right  - dstRect.left) + dstRect.left,
			(rSrc.top    - srcRect.top)  / (srcRect.bottom - srcRect.top)  * (dstRect.bottom - dstRect.top)  + dstRect.top,
			(rSrc.right  - srcRect.left) / (srcRect.right  - srcRect.left) * (dstRect.right  - dstRect.left) + dstRect.left,
			(rSrc.bottom - srcRect.top)  / (srcRect.bottom - srcRect.top)  * (dstRect.bottom - dstRect.top)  + dstRect.top);

		while (rSrc.top >= m_MaxBitmapSize)
		{
			rSrc.bottom -= m_MaxBitmapSize;
			rSrc.top -= m_MaxBitmapSize;
		}

		while (rSrc.left >= m_MaxBitmapSize)
		{
			rSrc.right -= m_MaxBitmapSize;
			rSrc.left -= m_MaxBitmapSize;
		}

		m_Target->DrawBitmap(segment.GetBitmap(), rDst, 1.0f, D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC, &rSrc);
	}
}

void Canvas::DrawTiledBitmap(Bitmap* bitmap, const D2D1_RECT_F& dstRect, const D2D1_RECT_F& srcRect)
{
	const FLOAT width = (FLOAT)bitmap->m_Width;
	const FLOAT height = (FLOAT)bitmap->m_Height;

	FLOAT x = dstRect.left;
	FLOAT y = dstRect.top;

	while (y < dstRect.bottom)
	{
		const FLOAT w = (dstRect.right - x) > width ? width : (dstRect.right - x);
		const FLOAT h = (dstRect.bottom - y) > height ? height : (dstRect.bottom - y);

		const auto dst = D2D1::RectF(x, y, x + w, y + h);
		const auto src = D2D1::RectF(0.0f, 0.0f, w, h);
		DrawBitmap(bitmap, dst, src);

		x += width;
		if (x >= dstRect.right && y < dstRect.bottom)
		{
			x = dstRect.left;
			y += height;
		}
	}
}

void Canvas::DrawMaskedBitmap(Bitmap* bitmap, Bitmap* maskBitmap, const D2D1_RECT_F& dstRect,
	const D2D1_RECT_F& srcRect, const D2D1_RECT_F& srcRect2)
{
	if (!bitmap || !maskBitmap) return;

	// Create bitmap brush from original |bitmap|.
	Microsoft::WRL::ComPtr<ID2D1BitmapBrush1> brush;
	D2D1_BITMAP_BRUSH_PROPERTIES1 propertiesXClampYClamp = D2D1::BitmapBrushProperties1(
		D2D1_EXTEND_MODE_CLAMP,
		D2D1_EXTEND_MODE_CLAMP,
		D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC);

	const FLOAT width = (FLOAT)bitmap->m_Width;
	const FLOAT height = (FLOAT)bitmap->m_Height;

	auto getRectSubRegion = [&width, &height](const D2D1_RECT_F& r1, const D2D1_RECT_F& r2) -> D2D1_RECT_F
	{
		return D2D1::RectF(
			r1.left / width * r2.right + r2.left,
			r1.top / height * r2.bottom + r2.top,
			(r1.right - r1.left) / width * r2.right,
			(r1.bottom - r1.top) / height * r2.bottom);
	};

	for (auto& bseg : bitmap->m_Segments)
	{
		const auto rSeg = bseg.GetRect();
		const auto rDst = getRectSubRegion(rSeg, dstRect);
		const auto rSrc = getRectSubRegion(rSeg, srcRect);

		FLOAT s2Width = srcRect2.right - srcRect2.left;
		FLOAT s2Height = srcRect2.bottom - srcRect2.top;

		// "Move" and "scale" the |bitmap| to match the destination.
		D2D1_MATRIX_3X2_F translateMask = D2D1::Matrix3x2F::Translation(-srcRect2.left, -srcRect2.top);
		D2D1_MATRIX_3X2_F translate = D2D1::Matrix3x2F::Translation(rDst.left, rDst.top);
		D2D1_MATRIX_3X2_F scale = D2D1::Matrix3x2F::Scale(
			D2D1::SizeF((rDst.right - rDst.left) / s2Width, (rDst.bottom - rDst.top) / s2Height));
		D2D1_BRUSH_PROPERTIES brushProps = D2D1::BrushProperties(1.0f, translateMask * scale * translate);

		HRESULT hr = m_Target->CreateBitmapBrush(
			bseg.GetBitmap(),
			propertiesXClampYClamp,
			brushProps,
			brush.ReleaseAndGetAddressOf());
		if (FAILED(hr)) return;

		const auto aaMode = m_Target->GetAntialiasMode();
		m_Target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // required

		for (auto& mseg : maskBitmap->m_Segments)
		{
			const auto rmSeg = mseg.GetRect();
			const auto rmDst = getRectSubRegion(rmSeg, dstRect);
			const auto rmSrc = getRectSubRegion(rmSeg, srcRect);

			// If no overlap, don't draw
			if (!(rmDst.left < (rDst.left + rDst.right) &&
				(rmDst.left + rmDst.right) > rDst.left &&
				rmDst.top < (rDst.top + rDst.bottom) &&
				(rmDst.top + rmDst.bottom) > rDst.top)) continue;

			m_Target->FillOpacityMask(
				mseg.GetBitmap(),
				brush.Get(),
				D2D1_OPACITY_MASK_CONTENT_GRAPHICS,
				&rDst,
				&rSrc);
		}

		m_Target->SetAntialiasMode(aaMode);
	}
}

bool Canvas::DrawSvg(Svg* svg, const D2D1_RECT_F& dstRect, const D2D1_RECT_F* clipRect)
{
	if (!svg || !svg->m_Document || svg->m_Size.width <= 0.0f || svg->m_Size.height <= 0.0f) return false;

	D2D1_MATRIX_3X2_F oldTransform;
	m_Target->GetTransform(&oldTransform);

	if (clipRect)
	{
		if (m_CanUseAxisAlignClip)
		{
			m_Target->PushAxisAlignedClip(*clipRect, D2D1_ANTIALIAS_MODE_ALIASED);
		}
		else
		{
			const D2D1_LAYER_PARAMETERS1 layerParams =
				D2D1::LayerParameters1(*clipRect, nullptr, D2D1_ANTIALIAS_MODE_ALIASED);
			m_Target->PushLayer(layerParams, nullptr);
		}
	}

	const D2D1_MATRIX_3X2_F transform =
		D2D1::Matrix3x2F::Scale(
			(dstRect.right - dstRect.left) / svg->m_Size.width,
			(dstRect.bottom - dstRect.top) / svg->m_Size.height) *
		D2D1::Matrix3x2F::Translation(dstRect.left, dstRect.top) *
		oldTransform;
	m_Target->SetTransform(transform);
	m_Target->DrawSvgDocument(svg->m_Document.Get());
	m_Target->SetTransform(oldTransform);

	if (clipRect)
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

	return true;
}

void Canvas::FillRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color)
{
	auto solidBrush = GetCachedSolidColorBrush(color);
	if (!solidBrush) return;

	m_Target->FillRectangle(rect, solidBrush.Get());
}

void Canvas::FillGradientRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color1, const D2D1_COLOR_F& color2, const FLOAT& angle)
{
	// D2D requires 2 points to draw the gradient along where GDI+ just requires a rectangle. To
	// mimic GDI+ for SolidColor2, we have to find and swap the starting and ending points of where
	// the gradient touches edge of the bounding rectangle. Normally we would offset the ending
	// point by 180, but we do this on starting point for SolidColor2. This differs from the other
	// D2D gradient options below:
	//  Gfx::BuildInlineGradientBrushes
	//  Gfx::Shape::CreateLinearGradient
	D2D1_POINT_2F start = Util::FindEdgePoint(angle + 180.0f, rect.left, rect.top, rect.right, rect.bottom);
	D2D1_POINT_2F end = Util::FindEdgePoint(angle, rect.left, rect.top, rect.right, rect.bottom);

	Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> pGradientStops;

	D2D1_GRADIENT_STOP gradientStops[2] = { 0 };
	gradientStops[0].color = color1;
	gradientStops[0].position = 0.0f;
	gradientStops[1].color = color2;
	gradientStops[1].position = 1.0f;

	HRESULT hr = m_Target->CreateGradientStopCollection(
		gradientStops,
		2,
		D2D1_GAMMA_2_2,
		D2D1_EXTEND_MODE_CLAMP,
		pGradientStops.GetAddressOf());
	if (FAILED(hr)) return;

	Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> brush;
	hr = m_Target->CreateLinearGradientBrush(
		D2D1::LinearGradientBrushProperties(start, end),
		pGradientStops.Get(),
		brush.GetAddressOf());
	if (FAILED(hr)) return;

	m_Target->FillRectangle(rect, brush.Get());
}

void Canvas::DrawLine(const D2D1_COLOR_F& color, FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, FLOAT strokeWidth)
{
	auto solidBrush = GetCachedSolidColorBrush(color);
	if (!solidBrush) return;

	m_Target->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), solidBrush.Get(), strokeWidth);
}

void Canvas::DrawGeometry(Shape& shape, int xPos, int yPos)
{
	D2D1_MATRIX_3X2_F worldTransform;
	m_Target->GetTransform(&worldTransform);
	m_Target->SetTransform(
		shape.GetShapeMatrix() *
		D2D1::Matrix3x2F::Translation((FLOAT)xPos, (FLOAT)yPos) *
		worldTransform);

	auto fill = shape.GetFillBrush(m_Target.Get());
	if (fill)
	{
		m_Target->FillGeometry(shape.m_Geometry.Get(), fill.Get());
	}

	Microsoft::WRL::ComPtr<ID2D1Brush> stroke;
	switch (shape.m_StrokeType)
	{
	case Shape::StrokeType::Default:
		stroke = GetCachedSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black));
		break;

	case Shape::StrokeType::Custom:
		stroke = shape.GetStrokeFillBrush(m_Target.Get());
		break;

	case Shape::StrokeType::Disabled:
		break;
	}

	if (stroke)
	{
		m_Target->DrawGeometry(shape.m_Geometry.Get(), stroke.Get(), shape.GetStrokeWidth(), shape.GetStrokeStyle());
	}

	m_Target->SetTransform(worldTransform);
}

Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> Canvas::GetCachedSolidColorBrush(const D2D1_COLOR_F& color)
{
	auto iter = m_SolidColorBrushCache.find(color);
	if (iter != m_SolidColorBrushCache.end()) return iter->second;

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
	HRESULT hr = m_Target->CreateSolidColorBrush(color, brush.GetAddressOf());
	if (FAILED(hr)) return nullptr;

	const size_t maxCacheSize = 64;
	if (m_SolidColorBrushCache.size() <= maxCacheSize)
	{
		m_SolidColorBrushCache.emplace(color, brush);
	}

	return brush;
}

}  // namespace Gfx
