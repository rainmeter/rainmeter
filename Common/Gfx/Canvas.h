// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "FontCollection.h"
#include "Shape.h"
#include "TextFormat.h"
#include <bit>
#include <string>
#include <vector>
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
	bool IsAccurateText() const { return m_AccurateText; }

	void SetDpiScale(float dpiScale);
	FLOAT SnapToPixel(FLOAT value) const;

	// Resize the draw area of the Canvas. This function must not be called if BeginDraw() has been
	// called and has not yet been matched by a correspoding call to EndDraw.
	bool Resize(int w, int h);

	bool BeginDraw();
	void EndDraw();

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

	// Clips drawing to |rect| until the matching PopClip().
	void PushClip(const D2D1_RECT_F& rect);
	void PopClip();

	bool SetTarget(Gfx::RenderTexture* texture);
	void ResetTarget();

	void SetAntiAliasing(bool enable);
	void SetTextAntiAliasing(bool enable);

	void Clear(const D2D1_COLOR_F& color = Util::c_Transparent_Color_F);

	void DrawTextW(const std::wstring& srcStr, TextFormat& format, const D2D1_RECT_F& rect,
		const D2D1_COLOR_F& color, bool applyInlineFormatting = false);
	bool MeasureTextW(const std::wstring& srcStr, TextFormat& format, D2D1_SIZE_F& size);
	bool MeasureTextLinesW(const std::wstring& srcStr, TextFormat& format, D2D1_SIZE_F& size, UINT32& lines);

	// Caret hit-testing. These reuse the text layout and the draw origin that DrawTextW() would
	// use for the same |srcStr|, |format| and |rect|, so the results line up exactly with the
	// rendered glyphs. Call them with the same arguments (and the same trimming/anti-aliasing
	// state) that were used to draw the text.

	// Returns the caret index nearest to |point|. The index is always a cluster boundary, so
	// surrogate pairs and combining marks are never split. |isTrailing| reports which side of the
	// index the hit fell on; it only matters where the two are visually apart, which is at a
	// boundary between text of opposing direction. See GetCaretRect().
	bool HitTestTextPoint(const std::wstring& srcStr, TextFormat& format, const D2D1_RECT_F& rect,
		const D2D1_POINT_2F& point, UINT32& caretIndex, bool& isTrailing);

	// Returns the rect the caret occupies at |caretIndex|, widened to |width|. A single index maps
	// to two visual positions where text of opposing direction meets, so |trailing| picks between
	// them: it selects the position adjacent to the text before |caretIndex| rather than after it.
	// Callers should set it from the direction the caret arrived from.
	bool GetCaretRect(const std::wstring& srcStr, TextFormat& format, const D2D1_RECT_F& rect,
		UINT32 caretIndex, bool trailing, FLOAT width, D2D1_RECT_F& caretRect);

	// Returns the caret index one cluster after (or before) |caretIndex|. Stepping by cluster
	// rather than by code unit is what keeps a caret out of the middle of a surrogate pair or of a
	// base character and its combining marks.
	bool GetAdjacentCaretIndex(const std::wstring& srcStr, TextFormat& format,
		const D2D1_RECT_F& rect, UINT32 caretIndex, bool forward, UINT32& adjacentIndex);

	// Returns the range of the line |caretIndex| sits on, excluding its newline.
	bool GetLineRange(const std::wstring& srcStr, TextFormat& format, const D2D1_RECT_F& rect,
		UINT32 caretIndex, UINT32& lineStart, UINT32& lineEnd);

	// Returns the rects covering [|start|, |start| + |length|), one per line the range spans.
	bool GetTextRangeRects(const std::wstring& srcStr, TextFormat& format, const D2D1_RECT_F& rect,
		UINT32 start, UINT32 length, std::vector<D2D1_RECT_F>& rects);

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

	// Computes the origin DrawTextW() passes to DrawTextLayout(). This is not |rect|'s top-left:
	// it carries the GDI+ compatibility offsets, so anything positioned against the rendered
	// glyphs (e.g. a caret) must go through here rather than recomputing them.
	D2D1_POINT_2F GetTextDrawPosition(TextFormat& format, const D2D1_RECT_F& rect) const;

	// Creates (or reuses) the text layout for |srcStr| exactly as DrawTextW() would, and reports
	// the draw origin along with the length of the string the layout was built over.
	bool PrepareTextLayout(const std::wstring& srcStr, TextFormat& format, const D2D1_RECT_F& rect,
		D2D1_POINT_2F& drawPosition, UINT32& strLen);

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
