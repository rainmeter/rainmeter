/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_CANVASGDIP_H_
#define RM_GFX_CANVASGDIP_H_

#include "Canvas.h"
#include "FontCollectionGDIP.h"
#include "TextFormatGDIP.h"
#include <memory>
#include <string>
#include <ole2.h>  // For Gdiplus.h.
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

	virtual void DrawTextW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect,
		const Gdiplus::SolidBrush& brush, bool applyInlineFormatting = false) override;
	virtual bool MeasureTextW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect) override;
	virtual bool MeasureTextLinesW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect, UINT& lines) override;

	virtual void DrawBitmap(Gdiplus::Bitmap* bitmap, const Gdiplus::Rect& dstRect, const Gdiplus::Rect& srcRect) override;
	virtual void DrawMaskedBitmap(Gdiplus::Bitmap* bitmap, Gdiplus::Bitmap* maskBitmap, const Gdiplus::Rect& dstRect,
		const Gdiplus::Rect& srcRect, const Gdiplus::Rect& srcRect2) override;

	virtual void FillRectangle(Gdiplus::Rect& rect, const Gdiplus::SolidBrush& brush) override;

private:
	friend class Canvas;

	CanvasGDIP();
	~CanvasGDIP();

	CanvasGDIP(const CanvasGDIP& other) = delete;
	CanvasGDIP& operator=(CanvasGDIP other) = delete;

	void Dispose();

	std::unique_ptr<Gdiplus::Graphics> m_Graphics;
	std::unique_ptr<Gdiplus::Bitmap> m_Bitmap;
	HBITMAP m_DIBSection;
	LPDWORD m_DIBSectionPixels;

	//static ULONG_PTR c_GdiToken;
};

}  // namespace Gfx

#endif
