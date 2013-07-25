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
	m_DIBSection(),
	m_DIBSectionPixels()
{
}

CanvasGDIP::~CanvasGDIP()
{
	Dispose();
}

void CanvasGDIP::Dispose()
{
	if (m_DIBSection)
	{
		DeleteObject(m_DIBSection);
		m_DIBSection = nullptr;
		m_DIBSectionPixels = nullptr;
	}
}

void CanvasGDIP::Resize(int w, int h)
{
	__super::Resize(w, h);

	Dispose();

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

	m_DIBSection = CreateDIBSection(
		nullptr,
		(BITMAPINFO*)&bh,
		DIB_RGB_COLORS,
		(void**)&m_DIBSectionPixels,
		nullptr,
		0);

	// Create GDI+ bitmap from the DIBSection pixels
	m_Bitmap.reset(new Gdiplus::Bitmap(w, h, w * 4, PixelFormat32bppPARGB, (BYTE*)m_DIBSectionPixels));
	m_Graphics.reset(new Gdiplus::Graphics(m_Bitmap.get()));
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
	SelectObject(dcMemory, m_DIBSection);
	return dcMemory;
}

void CanvasGDIP::ReleaseDC(HDC dc)
{
	DeleteDC(dc);
}

bool CanvasGDIP::IsTransparentPixel(int x, int y)
{
	if (m_DIBSectionPixels && x >= 0 && y >= 0 && x < m_W && y < m_H)
	{
		DWORD pixel = m_DIBSectionPixels[y * m_W + x];  // top-down DIB
		return ((pixel & 0xFF000000) != 0);
	}

	return false;
}

void CanvasGDIP::SetTransform(const Gdiplus::Matrix& matrix)
{
	m_Graphics->SetTransform(&matrix);
}

void CanvasGDIP::ResetTransform()
{
	m_Graphics->ResetTransform();
}

void CanvasGDIP::RotateTransform(float angle, float x, float y, float dx, float dy)
{
	m_Graphics->TranslateTransform(x, y);
	m_Graphics->RotateTransform(angle);
	m_Graphics->TranslateTransform(dx, dy);
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
		memset(m_DIBSectionPixels, 0, m_W * m_H * 4);
	}
	else
	{
		m_Graphics->Clear(color);
	}
}

void CanvasGDIP::DrawTextW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect, const Gdiplus::SolidBrush& brush)
{
	Gdiplus::StringFormat& stringFormat = ((TextFormatGDIP&)format).m_StringFormat;
	Gdiplus::StringFormat tStringFormat = Gdiplus::StringFormat::GenericTypographic();

	if (m_AccurateText)
	{
		tStringFormat.SetTrimming(stringFormat.GetTrimming());
		tStringFormat.SetFormatFlags(stringFormat.GetFormatFlags());
		tStringFormat.SetAlignment(stringFormat.GetAlignment());
		tStringFormat.SetLineAlignment(stringFormat.GetLineAlignment());
	}

	m_Graphics->DrawString(
		str, (INT)strLen, ((TextFormatGDIP&)format).m_Font.get(), rect,
		m_AccurateText ? &tStringFormat : &stringFormat, &brush);
}

bool CanvasGDIP::MeasureTextW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect)
{
	Gdiplus::StringFormat& stringFormat = ((TextFormatGDIP&)format).m_StringFormat;
	Gdiplus::StringFormat tStringFormat = Gdiplus::StringFormat::GenericTypographic();

	if (m_AccurateText)
	{
		tStringFormat.SetTrimming(stringFormat.GetTrimming());
		tStringFormat.SetFormatFlags(stringFormat.GetFormatFlags());
		tStringFormat.SetAlignment(stringFormat.GetAlignment());
		tStringFormat.SetLineAlignment(stringFormat.GetLineAlignment());
	}

	const Gdiplus::Status status = m_Graphics->MeasureString(
		str, (INT)strLen, ((TextFormatGDIP&)format).m_Font.get(), rect,
		m_AccurateText ? &tStringFormat : &stringFormat, &rect);

	return status == Gdiplus::Ok;
}

bool CanvasGDIP::MeasureTextLinesW(const WCHAR* str, UINT strLen, const TextFormat& format, Gdiplus::RectF& rect, UINT& lines)
{
	Gdiplus::StringFormat& stringFormat = ((TextFormatGDIP&)format).m_StringFormat;
	Gdiplus::StringFormat tStringFormat = Gdiplus::StringFormat::GenericTypographic();

	// Set trimming and format temporarily.
	const Gdiplus::StringTrimming stringTrimming = stringFormat.GetTrimming();
	stringFormat.SetTrimming(Gdiplus::StringTrimmingNone);

	const INT stringFormatFlags = stringFormat.GetFormatFlags();
	stringFormat.SetFormatFlags(Gdiplus::StringFormatFlagsNoClip);

	if (m_AccurateText)
	{
		tStringFormat.SetTrimming(stringFormat.GetTrimming());
		tStringFormat.SetFormatFlags(stringFormat.GetFormatFlags());
		tStringFormat.SetAlignment(stringFormat.GetAlignment());
		tStringFormat.SetLineAlignment(stringFormat.GetLineAlignment());
	}

	INT linesFilled = 0;
	const Gdiplus::Status status = m_Graphics->MeasureString(
		str, (INT)strLen, ((TextFormatGDIP&)format).m_Font.get(), rect,
		m_AccurateText ? &tStringFormat : &stringFormat, &rect, nullptr, &linesFilled);
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
