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

#include "CanvasGDIP.h"

namespace Gfx {

CanvasGDIP::CanvasGDIP() : Canvas(),
	m_Graphics(),
	m_DoubleBuffer(),
	m_DIBSectionBuffer(),
	m_DIBSectionBufferPixels()
{
}

CanvasGDIP::~CanvasGDIP()
{
	delete m_DoubleBuffer;
	if (m_DIBSectionBuffer) DeleteObject(m_DIBSectionBuffer);
}

void CanvasGDIP::Resize(int w, int h)
{
	__super::Resize(w, h);

	delete m_Graphics;

	if (m_DIBSectionBuffer)
	{
		delete m_DoubleBuffer;
		DeleteObject(m_DIBSectionBuffer);
		m_DIBSectionBufferPixels = nullptr;
	}

	BITMAPV4HEADER bh = {sizeof(BITMAPV4HEADER)};
	bh.bV4Width = w;
	bh.bV4Height = -h;	// Top-down DIB
	bh.bV4Planes = 1;
	bh.bV4BitCount = 32;
	bh.bV4V4Compression = BI_BITFIELDS;
	bh.bV4RedMask = 0x00FF0000;
	bh.bV4GreenMask = 0x0000FF00;
	bh.bV4BlueMask = 0x000000FF;
	bh.bV4AlphaMask = 0xFF000000;

	m_DIBSectionBuffer = CreateDIBSection(
		nullptr,
		(BITMAPINFO*)&bh,
		DIB_RGB_COLORS,
		(void**)&m_DIBSectionBufferPixels,
		nullptr,
		0);

	// Create GDI+ bitmap from the DIBSection pixels
	m_DoubleBuffer = new Gdiplus::Bitmap(
		w,
		h,
		w * 4,
		PixelFormat32bppPARGB,
		(BYTE*)m_DIBSectionBufferPixels);

	m_Graphics = new Gdiplus::Graphics(m_DoubleBuffer);
}

bool CanvasGDIP::BeginDraw()
{
	m_Graphics->SetInterpolationMode(Gdiplus::InterpolationModeDefault);
	m_Graphics->SetCompositingQuality(Gdiplus::CompositingQualityDefault);
	return true;
}

void CanvasGDIP::EndDraw()
{
}

Gdiplus::Graphics& CanvasGDIP::BeginGdiplusContext()
{
	return *m_Graphics;
}

void CanvasGDIP::EndGdiplusContext()
{
}

HDC CanvasGDIP::GetDC()
{
	HDC dcMemory = CreateCompatibleDC(nullptr);
	SelectObject(dcMemory, m_DIBSectionBuffer);
	return dcMemory;
}

void CanvasGDIP::ReleaseDC(HDC dc)
{
	DeleteDC(dc);
}

bool CanvasGDIP::IsTransparentPixel(int x, int y)
{
	if (m_DIBSectionBufferPixels && x >= 0 && y >= 0 && x < m_W && y < m_H)
	{
		DWORD pixel = m_DIBSectionBufferPixels[y * m_W + x];  // top-down DIB
		return ((pixel & 0xFF000000) != 0);
	}

	return false;
}

void CanvasGDIP::SetAntiAliasing(bool enable)
{
	m_Graphics->SetSmoothingMode(
		enable ? Gdiplus::SmoothingModeHighQuality : Gdiplus::SmoothingModeNone);
	m_Graphics->SetPixelOffsetMode(
		enable ? Gdiplus::PixelOffsetModeHighQuality : Gdiplus::PixelOffsetModeDefault);
}

void CanvasGDIP::SetTextAntiAliasing(bool enable)
{
	m_Graphics->SetTextRenderingHint(
		enable ? Gdiplus::TextRenderingHintAntiAlias : Gdiplus::TextRenderingHintSingleBitPerPixelGridFit);
}

void CanvasGDIP::Clear(const Gdiplus::Color& color)
{
	if (color.GetValue() == 0x00000000)
	{
		memset(m_DIBSectionBufferPixels, 0, m_W * m_H * 4);
	}
	else
	{
		m_Graphics->Clear(color);
	}
}

void CanvasGDIP::DrawTextW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect, const Gdiplus::SolidBrush& brush)
{
	Gdiplus::StringFormat& stringFormat = ((TextFormatGDIP&)format).m_StringFormat;
	m_Graphics->DrawString(str, (INT)strLen, ((TextFormatGDIP&)format).m_Font, rect, &stringFormat, &brush);
}

bool CanvasGDIP::MeasureTextW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect)
{
	Gdiplus::StringFormat& stringFormat = ((TextFormatGDIP&)format).m_StringFormat;
	const Gdiplus::Status status = m_Graphics->MeasureString(str, (INT)strLen, ((TextFormatGDIP&)format).m_Font, rect, &stringFormat, &rect);
	return status == Gdiplus::Ok;
}

bool CanvasGDIP::MeasureTextLinesW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect, UINT& lines)
{
	Gdiplus::StringFormat& stringFormat = ((TextFormatGDIP&)format).m_StringFormat;

	// Set trimming and format temporarily.
	const Gdiplus::StringTrimming stringTrimming = stringFormat.GetTrimming();
	stringFormat.SetTrimming(Gdiplus::StringTrimmingNone);

	const INT stringFormatFlags = stringFormat.GetFormatFlags();
	stringFormat.SetFormatFlags(Gdiplus::StringFormatFlagsNoClip);

	INT linesFilled = 0;
	const Gdiplus::Status status = m_Graphics->MeasureString(str, (INT)strLen, ((TextFormatGDIP&)format).m_Font, rect, &stringFormat, &rect, nullptr, &linesFilled);
	lines = linesFilled;

	// Restore old options.
	stringFormat.SetTrimming(stringTrimming);
	stringFormat.SetFormatFlags(stringFormatFlags);

	return status == Gdiplus::Ok;
}

void CanvasGDIP::DrawBitmap(Gdiplus::Bitmap* bitmap, const Gdiplus::Rect& dstRect, const Gdiplus::Rect& srcRect)
{
	m_Graphics->DrawImage(bitmap, dstRect, srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height, Gdiplus::UnitPixel);
}

void CanvasGDIP::FillRectangle(Gdiplus::Rect& rect, const Gdiplus::SolidBrush& brush)
{
	m_Graphics->FillRectangle(&brush, rect);
}

}  // namespace Gfx