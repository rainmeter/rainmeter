/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_CANVAS_H_
#define RM_GFX_CANVAS_H_

#include "FontCollection.h"
#include "Shape.h"
#include "TextFormat.h"
#include <bit>
#include <string>
#include <d2d1_3.h>
#include <dwrite_1.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <DXGI1_2.h>

template <>
struct ankerl::unordered_dense::hash<D2D1_COLOR_F>
{
	using is_avalanching = void;

	[[nodiscard]] auto operator()(const D2D1_COLOR_F& color) const noexcept -> uint64_t
	{
		const auto normalizeFloatBits = [](FLOAT value) -> uint32_t
		{
			// Handle positive and negative floating point zeros.
			return value == 0.0f ? 0 : std::bit_cast<uint32_t>(value);
		};

		const auto combineFloat = [&](FLOAT first, FLOAT second) -> uint64_t
		{
			return static_cast<uint64_t>(normalizeFloatBits(first)) | (static_cast<uint64_t>(normalizeFloatBits(second)) << 32);
		};

		uint64_t hash = ankerl::unordered_dense::detail::wyhash::hash(combineFloat(color.r, color.g));
		hash = ankerl::unordered_dense::detail::wyhash::mix(hash, combineFloat(color.b, color.a));
		return hash;
	}
};

struct D2D1ColorEqual
{
	bool operator()(const D2D1_COLOR_F& lhs, const D2D1_COLOR_F& rhs) const noexcept
	{
		return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
	}
};

namespace Gfx {

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

// Forward declaration
class Bitmap;
class GifImage;
class Svg;

class RenderTexture;

namespace Util {
	class BitmapLoader;
	class EffectStream;
}

// Wraps Direct2D/DirectWrite.
class Canvas
{
public:
	Canvas();
	~Canvas();

	using DeviceLostCallback = void (*)();
	static bool Initialize(bool hardwareAccelerated, DeviceLostCallback deviceLostCallback = nullptr);
	static void Finalize();
	static bool AttachDevice();

	static bool EnumerateInstalledFontFamilies(UINT32& familyCount, std::wstring& families);

	HRESULT InitializeDeviceContextForWindow(HWND window);

	int GetW() const { return m_W; }
	int GetH() const { return m_H; }

	void SetAccurateText(bool option) { m_AccurateText = option; }

	void SetDpiScale(float dpiScale);
	FLOAT SnapToPixel(FLOAT value) const;

	// Resize the draw area of the Canvas. This function must not be called if BeginDraw() has been
	// called and has not yet been matched by a correspoding call to EndDraw.
	bool Resize(int w, int h);

	bool BeginDraw();
	void EndDraw();
	void StartGpuTimer();
	void EndGpuTimer();
	double GetAverageGpuFrameTime() const { return m_AverageGpuFrameTime; }

	HDC GetDC();
	void ReleaseDC();

	FontCollection* CreateFontCollection() { return new FontCollection(); }
	TextFormat* CreateTextFormat(const MathParser& mathParser) { return new TextFormat(mathParser); }

	bool IsTransparentPixel(int x, int y);

	bool IsDrawing() { return m_IsDrawing; }

	void GetTransform(D2D1_MATRIX_3X2_F* matrix);
	void SetTransform(const D2D1_MATRIX_3X2_F& matrix);
	void ResetTransform();
	void PushOpacityLayer(FLOAT opacity);
	void PopLayer();

	bool SetTarget(Gfx::RenderTexture* texture);
	void ResetTarget();

	void SetAntiAliasing(bool enable);
	void SetTextAntiAliasing(bool enable);

	void Clear(const D2D1_COLOR_F& color = Util::c_Transparent_Color_F);

	void DrawTextW(const std::wstring& srcStr, TextFormat& format, const D2D1_RECT_F& rect,
		const D2D1_COLOR_F& color, bool applyInlineFormatting = false);
	bool MeasureTextW(const std::wstring& srcStr, TextFormat& format, D2D1_SIZE_F& size);
	bool MeasureTextLinesW(const std::wstring& srcStr, TextFormat& format, D2D1_SIZE_F& size, UINT32& lines);

	void DrawBitmap(Bitmap* bitmap, const D2D1_RECT_F& dstRect, const D2D1_RECT_F& srcRect);
	void DrawTiledBitmap(Bitmap* bitmap, const D2D1_RECT_F& dstRect, const D2D1_RECT_F& srcRect);
	void DrawMaskedBitmap(Bitmap* bitmap, Bitmap* maskBitmap, const D2D1_RECT_F& dstRect,
		const D2D1_RECT_F& srcRect, const D2D1_RECT_F& srcRect2);
	bool DrawSvg(Svg* svg, const D2D1_RECT_F& dstRect, const D2D1_RECT_F* clipRect = nullptr);

	void FillRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color);
	void FillGradientRectangle(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color1, const D2D1_COLOR_F& color2, const FLOAT& angle);

	void DrawLine(const D2D1_COLOR_F& color, FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, FLOAT strokeWidth = 1.0f);

	void DrawGeometry(Shape& shape, int x, int y);

private:
	friend class Canvas;
	friend class Bitmap;
	friend class GifImage;
	friend class Svg;
	friend class RenderTexture;
	friend class FontCollection;
	friend class TextFormat;
	friend class TextInlineFormat_Face;
	friend class TextInlineFormat_Typography;
	friend class Shape;
	friend class Util::BitmapLoader;
	friend class Util::EffectStream;

	Canvas(const Canvas& other) = delete;
	Canvas& operator=(Canvas other) = delete;

	static ComPtr<ID2D1DeviceContext5> CreateDeviceContext();
	void InitializeGpuTimer();

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> GetCachedSolidColorBrush(const D2D1_COLOR_F& color);

	Microsoft::WRL::ComPtr<ID2D1DeviceContext5> m_Target;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> m_SwapChain;
	Microsoft::WRL::ComPtr<IDXGISurface1> m_BackBuffer;
	Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_TargetBitmap;

	int m_W;
	int m_H;
	FLOAT m_Dpi;
	UINT32 m_MaxBitmapSize;

	bool m_ValidDeviceContext;
	bool m_IsDrawing;
	bool m_EnableDrawAfterGdi;

	// GDI+, by default, includes padding around the string and also has a larger character spacing
	// compared to DirectWrite. In order to minimize diffeences between the text renderers,
	// an option is provided to enable accurate (typographic) text rendering. If set to |true|,
	// it is expected that there is no padding around the text and that the output is similar to
	// the default DirectWrite output. Otherwise, the expected result should be similar to that of
	// non-typographic GDI+.
	bool m_AccurateText;

	bool m_TextAntiAliasing;

	// |true| if PushAxisAlignedClip()/PopAxisAlignedClip() can be used.
	bool m_CanUseAxisAlignClip;

	static constexpr UINT GPU_QUERY_COUNT = 5;
	static constexpr UINT GPU_TIME_HISTORY_COUNT = 64;
	Microsoft::WRL::ComPtr<ID3D11Query> m_GpuStartQuery[GPU_QUERY_COUNT];
	Microsoft::WRL::ComPtr<ID3D11Query> m_GpuEndQuery[GPU_QUERY_COUNT];
	Microsoft::WRL::ComPtr<ID3D11Query> m_GpuDisjointQuery[GPU_QUERY_COUNT];
	UINT m_CurrentGpuQuery;
	UINT m_GpuFrameCount;
	bool m_GpuTimerActive;
	UINT m_GpuTimeHistoryIndex;
	UINT m_GpuTimeHistorySize;
	double m_GpuTimeHistory[GPU_TIME_HISTORY_COUNT];
	double m_GpuTimeTotal;
	double m_AverageGpuFrameTime;

	ankerl::unordered_dense::map<D2D1_COLOR_F, Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>, ankerl::unordered_dense::hash<D2D1_COLOR_F>, D2D1ColorEqual> m_SolidColorBrushCache;

	static bool c_HardwareAccelerated;
	static DeviceLostCallback c_DeviceLostCallback;

	// Device-dependent.
	static Microsoft::WRL::ComPtr<ID3D11Device> c_D3DDevice;
	static Microsoft::WRL::ComPtr<ID3D11DeviceContext> c_D3DContext;
	static Microsoft::WRL::ComPtr<ID2D1Device> c_D2DDevice;
	static Microsoft::WRL::ComPtr<IDXGIDevice1> c_DxgiDevice;

	// Device-independent.
	static Microsoft::WRL::ComPtr<ID2D1Factory1> c_D2DFactory;
	static Microsoft::WRL::ComPtr<IDWriteFactory1> c_DWFactory;
	static Microsoft::WRL::ComPtr<IWICImagingFactory> c_WICFactory;

	// Always kept at the default DPI for use in temporary contexts.
	static Microsoft::WRL::ComPtr<ID2D1DeviceContext5> c_EffectTarget;
};

}  // namespace Gfx

#endif
