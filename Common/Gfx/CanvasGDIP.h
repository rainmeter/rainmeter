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

#ifndef RM_GFX_CANVASGDIP_H_
#define RM_GFX_CANVASGDIP_H_

#include "Canvas.h"
#include "FontCollectionGDIP.h"
#include "TextFormatGDIP.h"
#include <memory>
#include <string>
#include <GdiPlus.h>

namespace Gfx {

// Provides a GDI+ implementation of Canvas.
class CanvasGDIP : public Canvas
{
public:
	virtual void Resize(int w, int h);

	virtual bool BeginDraw();
	virtual void EndDraw();

	virtual Gdiplus::Graphics& BeginGdiplusContext() override;
	virtual void EndGdiplusContext() override;

	virtual HDC GetDC() override;
	virtual void ReleaseDC(HDC dc) override;

	virtual FontCollection* CreateFontCollection() override { return new FontCollectionGDIP(); }
	virtual TextFormat* CreateTextFormat() override { return new TextFormatGDIP(); }

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

	CanvasGDIP();
	~CanvasGDIP();
	CanvasGDIP(const CanvasGDIP& other) {}

	void Dispose();

	std::unique_ptr<Gdiplus::Graphics> m_Graphics;
	std::unique_ptr<Gdiplus::Bitmap> m_Bitmap;
	HBITMAP m_DIBSection;
	LPDWORD m_DIBSectionPixels;

	//static ULONG_PTR c_GdiToken;
};

}  // namespace Gfx

#endif