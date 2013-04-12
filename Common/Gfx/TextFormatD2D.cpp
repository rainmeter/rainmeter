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
#include "Util/DWriteHelpers.h"

namespace Gfx {

TextFormatD2D::TextFormatD2D() :
	m_TextFormat(),
	m_TextLayout(),
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

	if (m_TextLayout)
	{
		m_TextLayout->Release();
		m_TextLayout = nullptr;
	}

	if (m_InlineEllipsis)
	{
		m_InlineEllipsis->Release();
		m_InlineEllipsis = nullptr;
	}
}

void TextFormatD2D::CreateLayout(const WCHAR* str, UINT strLen, float maxW, float maxH)
{
	bool strChanged = false;
	if (strLen != m_LastString.length() ||
		memcmp(str, m_LastString.c_str(), (strLen + 1) * sizeof(WCHAR)) != 0)
	{
		strChanged = true;
		m_LastString.assign(str, strLen);
	}

	if (m_TextLayout && !strChanged)
	{
		if (maxW != m_TextLayout->GetMaxWidth())
		{
			m_TextLayout->SetMaxWidth(maxW);
		}

		if (maxH != m_TextLayout->GetMaxHeight())
		{
			m_TextLayout->SetMaxWidth(maxH);
		}
	}
	else
	{
		if (m_TextLayout)
		{
			m_TextLayout->Release();
			m_TextLayout = nullptr;
		}

		CanvasD2D::c_DWFactory->CreateTextLayout(str, strLen, m_TextFormat, maxW, maxH, &m_TextLayout);
	}
}

void TextFormatD2D::SetProperties(
	const WCHAR* fontFamily, int size, bool bold, bool italic,
	const FontCollection* fontCollection)
{
	auto fontCollectionD2D = (FontCollectionD2D*)fontCollection;

	Dispose();

	WCHAR dwriteFamilyName[LF_FACESIZE];
	DWRITE_FONT_WEIGHT dwriteFontWeight =
		bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR;
	DWRITE_FONT_STYLE dwriteFontStyle =
		italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
	DWRITE_FONT_STRETCH dwriteFontStretch = DWRITE_FONT_STRETCH_NORMAL;

	// |fontFamily| uses the GDI/GDI+ font naming convention so try to create DirectWrite font
	// using the GDI family name and then create a text format using the DirectWrite family name
	// obtained from it.
	HRESULT hr = Util::GetDWritePropertiesFromGDIProperties(
		CanvasD2D::c_DWFactory, fontFamily, bold, italic, dwriteFontWeight, dwriteFontStyle,
		dwriteFontStretch, dwriteFamilyName, _countof(dwriteFamilyName));
	if (SUCCEEDED(hr))
	{
		hr = CanvasD2D::c_DWFactory->CreateTextFormat(
			dwriteFamilyName,
			nullptr,
			dwriteFontWeight,
			dwriteFontStyle,
			dwriteFontStretch,
			size * (4.0f / 3.0f),
			L"",
			&m_TextFormat);
	}

	if (FAILED(hr))
	{
		IDWriteFontCollection* dwriteFontCollection = nullptr;

		// If |fontFamily| is not in the system collection, use the font collection from
		// |fontCollectionD2D| if possible.
		if (!Util::IsFamilyInSystemFontCollection(CanvasD2D::c_DWFactory, fontFamily) &&
			(fontCollectionD2D && fontCollectionD2D->InitializeCollection()))
		{
			IDWriteFont* dwriteFont = Util::FindDWriteFontInFontCollectionByGDIFamilyName(
				fontCollectionD2D->m_Collection, fontFamily);
			if (dwriteFont)
			{
				hr = Util::GetFamilyNameFromDWriteFont(
					dwriteFont, dwriteFamilyName, _countof(dwriteFamilyName));
				{
					fontFamily = dwriteFamilyName;
					Util::GetPropertiesFromDWriteFont(
						dwriteFont, bold, italic, &dwriteFontWeight, &dwriteFontStyle,
						&dwriteFontStretch);
				}

				dwriteFont->Release();
			}

			dwriteFontCollection = fontCollectionD2D->m_Collection;
		}

		// Fallback in case above fails.
		hr = CanvasD2D::c_DWFactory->CreateTextFormat(
			fontFamily,
			dwriteFontCollection,
			dwriteFontWeight,
			dwriteFontStyle,
			dwriteFontStretch,
			size * (4.0f / 3.0f),
			L"",
			&m_TextFormat);
	}

	if (SUCCEEDED(hr))
	{
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
		if (!m_InlineEllipsis)
		{
			CanvasD2D::c_DWFactory->CreateEllipsisTrimmingSign(m_TextFormat, &m_InlineEllipsis);
		}

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
