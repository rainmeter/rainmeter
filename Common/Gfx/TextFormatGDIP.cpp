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

#include "TextFormatGDIP.h"

namespace Gfx {

TextFormatGDIP::TextFormatGDIP() :
	m_Font(),
	m_FontFamily()
{
}

TextFormatGDIP::~TextFormatGDIP()
{
	Dispose();
}

void TextFormatGDIP::Dispose()
{
	delete m_FontFamily;
	m_FontFamily = nullptr;

	delete m_Font;
	m_Font = nullptr;
}

void TextFormatGDIP::SetProperties(const WCHAR* fontFamily, int size, bool bold, bool italic, Gdiplus::PrivateFontCollection* fontCollection)
{
	Dispose();

	m_FontFamily = new Gdiplus::FontFamily(fontFamily);
	if (m_FontFamily->GetLastStatus() != Gdiplus::Ok)
	{
		delete m_FontFamily;
		m_FontFamily = nullptr;

		// Not found in system collection so try the private collection.
		if (fontCollection)
		{
			m_FontFamily = new Gdiplus::FontFamily(fontFamily, fontCollection);
			if (m_FontFamily->GetLastStatus() != Gdiplus::Ok)
			{
				delete m_FontFamily;
				m_FontFamily = nullptr;
			}
		}
	}

	Gdiplus::FontStyle style = Gdiplus::FontStyleRegular;
	if (bold && italic)
	{
		style = Gdiplus::FontStyleBoldItalic;
	}
	else if (bold)
	{
		style = Gdiplus::FontStyleBold;
	}
	else if (italic)
	{
		style = Gdiplus::FontStyleItalic;
	}

	if (size != 0)
	{
		// Adjust the font size with screen DPI.
		HDC dc = GetDC(0);
		const int dpi = GetDeviceCaps(dc, LOGPIXELSX);
		ReleaseDC(0, dc);
		const Gdiplus::REAL fontSize = (Gdiplus::REAL)size * (96.0f / dpi);

		if (m_FontFamily)
		{
			m_Font = new Gdiplus::Font(m_FontFamily, fontSize, style);
			if (m_Font->GetLastStatus() != Gdiplus::Ok)
			{
				delete m_Font;
				m_Font = nullptr;
			}
		}

		if (!m_Font)
		{
			// Use default font ("Arial" or GenericSansSerif).
			m_Font = new Gdiplus::Font(L"Arial", fontSize, style);
			if (m_Font->GetLastStatus() != Gdiplus::Ok)
			{
				delete m_Font;
				m_Font = nullptr;
			}
		}
	}
}

void TextFormatGDIP::SetTrimming(bool trim)
{
	if (trim)
	{
		m_StringFormat.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
		m_StringFormat.SetFormatFlags(0x0);
	}
	else
	{
		m_StringFormat.SetTrimming(Gdiplus::StringTrimmingNone);
		m_StringFormat.SetFormatFlags(Gdiplus::StringFormatFlagsNoClip | Gdiplus::StringFormatFlagsNoWrap);
	}
}

void TextFormatGDIP::SetHorizontalAlignment(HorizontalAlignment alignment)
{
	static_assert(Gdiplus::StringAlignmentNear == (int)HorizontalAlignment::Left, "Enum mismatch");
	static_assert(Gdiplus::StringAlignmentCenter == (int)HorizontalAlignment::Center, "Enum mismatch");
	static_assert(Gdiplus::StringAlignmentFar == (int)HorizontalAlignment::Right, "Enum mismatch");

	__super::SetHorizontalAlignment(alignment);
	m_StringFormat.SetAlignment((Gdiplus::StringAlignment)alignment);
}

void TextFormatGDIP::SetVerticalAlignment(VerticalAlignment alignment)
{
	static_assert(Gdiplus::StringAlignmentNear == (int)VerticalAlignment::Top, "Enum mismatch");
	static_assert(Gdiplus::StringAlignmentCenter == (int)VerticalAlignment::Center, "Enum mismatch");
	static_assert(Gdiplus::StringAlignmentFar == (int)VerticalAlignment::Bottom, "Enum mismatch");

	__super::SetVerticalAlignment(alignment);
	m_StringFormat.SetLineAlignment((Gdiplus::StringAlignment)alignment);
}

}  // namespace Gfx