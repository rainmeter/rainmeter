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

#include "TextFormatD2D.h"
#include "CanvasD2D.h"

namespace Gfx {

TextFormatD2D::TextFormatD2D() :
	m_TextFormat(),
	m_InlineEllipsis()
{
}

TextFormatD2D::~TextFormatD2D()
{
	Dispose();
}

void TextFormatD2D::Dispose()
{
	if (m_TextFormat)
	{
		m_TextFormat->Release();
		m_TextFormat = nullptr;
	}

	if (m_InlineEllipsis)
	{
		m_InlineEllipsis->Release();
		m_InlineEllipsis = nullptr;
	}
}

void TextFormatD2D::SetProperties(const WCHAR* fontFamily, int size, bool bold, bool italic)
{
	Dispose();

	HRESULT hr = CanvasD2D::c_DW->CreateTextFormat(
		fontFamily,
		nullptr,
		bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR,
		italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		size * (4.0f / 3.0f),
		L"",
		&m_TextFormat);

	if (SUCCEEDED(hr))
	{
		CanvasD2D::c_DW->CreateEllipsisTrimmingSign(m_TextFormat, &m_InlineEllipsis);

		SetHorizontalAlignment(GetHorizontalAlignment());
		SetVerticalAlignment(GetVerticalAlignment());
	}
	else
	{
		Dispose();
	}
}

void TextFormatD2D::SetTrimming(bool trim)
{
	IDWriteInlineObject* inlineObject = nullptr;
	DWRITE_TRIMMING trimming = {};
	DWRITE_WORD_WRAPPING wordWrapping = DWRITE_WORD_WRAPPING_NO_WRAP;
	if (trim)
	{
		inlineObject = m_InlineEllipsis;
		trimming.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
		wordWrapping = DWRITE_WORD_WRAPPING_WRAP;
	}

	m_TextFormat->SetTrimming(&trimming, inlineObject);
	m_TextFormat->SetWordWrapping(wordWrapping);
}

void TextFormatD2D::SetHorizontalAlignment(HorizontalAlignment alignment)
{
	__super::SetHorizontalAlignment(alignment);

	if (m_TextFormat)
	{
		m_TextFormat->SetTextAlignment(
			(alignment == HorizontalAlignment::Left) ? DWRITE_TEXT_ALIGNMENT_LEADING :
			(alignment == HorizontalAlignment::Center) ? DWRITE_TEXT_ALIGNMENT_CENTER :
			DWRITE_TEXT_ALIGNMENT_TRAILING);
	}
}

void TextFormatD2D::SetVerticalAlignment(VerticalAlignment alignment)
{
	__super::SetVerticalAlignment(alignment);
	
	if (m_TextFormat)
	{
		m_TextFormat->SetParagraphAlignment(
			(alignment == VerticalAlignment::Top) ? DWRITE_PARAGRAPH_ALIGNMENT_NEAR :
			(alignment == VerticalAlignment::Center) ? DWRITE_PARAGRAPH_ALIGNMENT_CENTER :
			DWRITE_PARAGRAPH_ALIGNMENT_FAR);
	}
}

}  // namespace Gfx