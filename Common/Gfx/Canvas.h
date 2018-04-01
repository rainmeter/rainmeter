/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_CANVAS_H_
#define RM_GFX_CANVAS_H_

#include "FontCollectionD2D.h"
#include "Shape.h"
#include "TextFormatD2D.h"
#include <string>
#include <GdiPlus.h>
#include <d2d1_1.h>
#include <dwrite_1.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <DXGI1_2.h>
#include <stack>

namespace Gfx {

// Forward declaration
class D2DBitmap;

namespace Util {
	class D2DBitmapLoader;
	class D2DEffectStream;
}

// Wraps Direct2D/DirectWrite.
class Canvas
{
public:
	Canvas();
	~Canvas();

	static bool Initialize(bool hardwareAccelerated);
	static void Finalize();

	bool InitializeRenderTarget(HWND hwnd);

	int GetW() const { return m_W; }
	int GetH() const { return m_H; }

	void SetAccurateText(bool option) { m_AccurateText = option; }

	// Resize the draw area of the Canvas. This function must not be called if BeginDraw() has been
	// called and has not yet been matched by a correspoding call to EndDraw.
	void Resize(int w, int h);

	bool BeginDraw();
	void EndDraw();

	HDC GetDC();
	void ReleaseDC();

	FontCollection* CreateFontCollection() { return new FontCollectionD2D(); }
	TextFormat* CreateTextFormat() { return new TextFormatD2D(); }

	bool IsTransparentPixel(int x, int y);

	void SetTransform(const Gdiplus::Matrix& matrix);
	void ResetTransform();
	void RotateTransform(float angle, float x, float y, float dx, float dy);

	void PushClip(Gfx::Shape* clip);
	void PopClip();

	void SetAntiAliasing(bool enable);
	void SetTextAntiAliasing(bool enable);

	void Clear(const Gdiplus::Color& color = Gdiplus::Color(0, 0, 0, 0));

	void DrawTextW(const std::wstring& srcStr, const TextFormat& format, Gdiplus::RectF& rect,
		const Gdiplus::SolidBrush& brush, bool applyInlineFormatting = false);
	bool MeasureTextW(const std::wstring& srcStr, const TextFormat& format, Gdiplus::RectF& rect);
	bool MeasureTextLinesW(const std::wstring& srcStr, const TextFormat& format, Gdiplus::RectF& rect, UINT& lines);

	void DrawBitmap(const D2DBitmap* bitmap, const Gdiplus::Rect& dstRect, const Gdiplus::Rect& srcRect);
	void DrawTiledBitmap(const D2DBitmap* bitmap, const Gdiplus::Rect& dstRect, const Gdiplus::Rect& srcRect);
	void DrawMaskedBitmap(const D2DBitmap* bitmap, const D2DBitmap* maskBitmap, const Gdiplus::Rect& dstRect,
		const Gdiplus::Rect& srcRect, const Gdiplus::Rect& srcRect2);

	void FillRectangle(Gdiplus::Rect& rect, const Gdiplus::SolidBrush& brush);
	void FillGradientRectangle(Gdiplus::Rect& rect, const Gdiplus::Color& color1, const Gdiplus::Color& color2, const FLOAT& angle);

	void DrawLine(const Gdiplus::Color& color, FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, FLOAT strokeWidth = 1.0f);

	void DrawGeometry(Shape& shape, int x, int y);

private:
	friend class Canvas;
	friend class FontCollectionD2D;
	friend class TextFormatD2D;
	friend class TextInlineFormat_Typography;
	friend class Shape;
	friend class Rectangle;
	friend class RoundedRectangle;
	friend class Ellipse;
	friend class Line;
	friend class Arc;
	friend class Curve;
	friend class QuadraticCurve;
	friend class Path;
	friend class Gfx::Util::D2DBitmapLoader;
	friend class Gfx::Util::D2DEffectStream;
	friend class Gfx::D2DBitmap;

	Canvas(const Canvas& other) = delete;
	Canvas& operator=(Canvas other) = delete;

	HRESULT CreateRenderTarget();
	bool CreateTargetBitmap(UINT32 width, UINT32 height);

	Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_Target;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> m_SwapChain;
	Microsoft::WRL::ComPtr<IDXGISurface1> m_BackBuffer;
	Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_TargetBitmap;
	Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_BufferSnapshot;

	std::stack<Microsoft::WRL::ComPtr<ID2D1Layer>> m_Layers;

	int m_W;
	int m_H;
	UINT32 m_MaxBitmapSize;

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

	static UINT c_Instances;
	static D3D_FEATURE_LEVEL c_FeatureLevel;
	static Microsoft::WRL::ComPtr<ID3D11Device> c_D3DDevice;
	static Microsoft::WRL::ComPtr<ID3D11DeviceContext> c_D3DContext;
	static Microsoft::WRL::ComPtr<ID2D1Device> c_D2DDevice;
	static Microsoft::WRL::ComPtr<IDXGIDevice1> c_DxgiDevice;
	static Microsoft::WRL::ComPtr<ID2D1Factory1> c_D2DFactory;
	static Microsoft::WRL::ComPtr<IDWriteFactory1> c_DWFactory;
	static Microsoft::WRL::ComPtr<IWICImagingFactory> c_WICFactory;
};

}  // namespace Gfx

#endif
