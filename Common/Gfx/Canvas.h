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
#include "Util/WICBitmapDIB.h"
#include <memory>
#include <string>
#include <ole2.h>  // For Gdiplus.h.
#include <GdiPlus.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <dwrite_1.h>
#include <wincodec.h>
#include <wrl/client.h>

namespace Gfx {

// Wraps Direct2D/DirectWrite.
class Canvas
{
public:
	Canvas();
	~Canvas();

	static bool Initialize();
	static void Finalize();

	int GetW() const { return m_W; }
	int GetH() const { return m_H; }

	void SetAccurateText(bool option) { m_AccurateText = option; }

	// Resize the draw area of the Canvas. This function must not be called if BeginDraw() has been
	// called and has not yet been matched by a correspoding call to EndDraw.
	void Resize(int w, int h);

	bool BeginDraw();
	void EndDraw();

	Gdiplus::Graphics& BeginGdiplusContext();
	void EndGdiplusContext();

	HDC GetDC();
	void ReleaseDC(HDC dc);
	
	FontCollection* CreateFontCollection() { return new FontCollectionD2D(); }
	TextFormat* CreateTextFormat() { return new TextFormatD2D(); }

	bool IsTransparentPixel(int x, int y);

	void SetTransform(const Gdiplus::Matrix& matrix);
	void ResetTransform();
	void RotateTransform(float angle, float x, float y, float dx, float dy);

	void SetAntiAliasing(bool enable);
	void SetTextAntiAliasing(bool enable);

	void Clear(const Gdiplus::Color& color = Gdiplus::Color(0, 0, 0, 0));

	void DrawTextW(const std::wstring& srcStr, const TextFormat& format, Gdiplus::RectF& rect,
		const Gdiplus::SolidBrush& brush, bool applyInlineFormatting = false);
	bool MeasureTextW(const std::wstring& srcStr, const TextFormat& format, Gdiplus::RectF& rect);
	bool MeasureTextLinesW(const std::wstring& srcStr, const TextFormat& format, Gdiplus::RectF& rect, UINT& lines);

	void DrawBitmap(Gdiplus::Bitmap* bitmap, const Gdiplus::Rect& dstRect, const Gdiplus::Rect& srcRect);
	void DrawMaskedBitmap(Gdiplus::Bitmap* bitmap, Gdiplus::Bitmap* maskBitmap, const Gdiplus::Rect& dstRect,
		const Gdiplus::Rect& srcRect, const Gdiplus::Rect& srcRect2);

	void FillRectangle(Gdiplus::Rect& rect, const Gdiplus::SolidBrush& brush);

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

	Canvas(const Canvas& other) = delete;
	Canvas& operator=(Canvas other) = delete;

	bool BeginTargetDraw();
	void EndTargetDraw();

	// Sets the |m_Target| transformation to be equal to that of |m_GdipGraphics|.
	void UpdateTargetTransform();

	int m_W;
	int m_H;

	// GDI+, by default, includes padding around the string and also has a larger character spacing
	// compared to DirectWrite. In order to minimize diffeences between the text renderers,
	// an option is provided to enable accurate (typographic) text rendering. If set to |true|,
	// it is expected that there is no padding around the text and that the output is similar to
	// the default DirectWrite output. Otherwise, the expected result should be similar to that of
	// non-typographic GDI+.
	bool m_AccurateText;

	Microsoft::WRL::ComPtr<ID2D1RenderTarget> m_Target;

	// Underlying pixel data shared by both m_Target and m_GdipBitmap.
	Util::WICBitmapDIB m_Bitmap;

	// GDI+ objects that share the pixel data of m_Bitmap.
	std::unique_ptr<Gdiplus::Graphics> m_GdipGraphics;
	std::unique_ptr<Gdiplus::Bitmap> m_GdipBitmap;

	bool m_TextAntiAliasing;

	// |true| if PushAxisAlignedClip()/PopAxisAlignedClip() can be used.
	bool m_CanUseAxisAlignClip;

	static UINT c_Instances;
	static Microsoft::WRL::ComPtr<ID2D1Factory1> c_D2DFactory;
	static Microsoft::WRL::ComPtr<IDWriteFactory1> c_DWFactory;
	static Microsoft::WRL::ComPtr<IDWriteGdiInterop> c_DWGDIInterop;
	static Microsoft::WRL::ComPtr<IWICImagingFactory> c_WICFactory;
};

}  // namespace Gfx

#endif
