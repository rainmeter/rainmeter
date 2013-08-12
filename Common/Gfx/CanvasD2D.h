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

#ifndef RM_GFX_CANVASD2D_H_
#define RM_GFX_CANVASD2D_H_

#include "Canvas.h"
#include "FontCollectionD2D.h"
#include "TextFormatD2D.h"
#include "Util/WICBitmapDIB.h"
#include <memory>
#include <string>
#include <GdiPlus.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <dwrite_1.h>
#include <wincodec.h>
#include <wrl/client.h>

namespace Gfx {

// Provides a Direct2D/DirectWrite implementation of Canvas.
class CanvasD2D : public Canvas
{
public:
	virtual void Resize(int w, int h);

	virtual bool BeginDraw();
	virtual void EndDraw();

	virtual Gdiplus::Graphics& BeginGdiplusContext() override;
	virtual void EndGdiplusContext() override;

	virtual HDC GetDC() override;
	virtual void ReleaseDC(HDC dc) override;
	
	virtual FontCollection* CreateFontCollection() override { return new FontCollectionD2D(); }
	virtual TextFormat* CreateTextFormat() override { return new TextFormatD2D(); }

	virtual bool IsTransparentPixel(int x, int y) override;

	virtual void SetTransform(const Gdiplus::Matrix& matrix) override;
	virtual void ResetTransform() override;
	virtual void RotateTransform(float angle, float x, float y, float dx, float dy) override;

	virtual void SetAntiAliasing(bool enable) override;
	virtual void SetTextAntiAliasing(bool enable) override;

	virtual void Clear(const Gdiplus::Color& color) override;

	virtual void DrawTextW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect, const Gdiplus::SolidBrush& brush) override;
	virtual bool MeasureTextW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect) override;
	virtual bool MeasureTextLinesW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect, UINT& lines) override;

	virtual void DrawBitmap(Gdiplus::Bitmap* bitmap, const Gdiplus::Rect& dstRect, const Gdiplus::Rect& srcRect) override;

	virtual void FillRectangle(Gdiplus::Rect& rect, const Gdiplus::SolidBrush& brush) override;

private:
	friend class Canvas;
	friend class FontCollectionD2D;
	friend class TextFormatD2D;

	CanvasD2D();
	~CanvasD2D();
	CanvasD2D(const CanvasD2D& other) {}

	static bool Initialize();
	static void Finalize();

	bool BeginTargetDraw();
	void EndTargetDraw();

	// Sets the |m_Target| transformation to be equal to that of |m_GdipGraphics|.
	void UpdateTargetTransform();

	Microsoft::WRL::ComPtr<ID2D1RenderTarget> m_Target;
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