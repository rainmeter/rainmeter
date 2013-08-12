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
	m_ExtraHeight(),
	m_LineGap(),
	m_Trimming()
{
}

TextFormatD2D::~TextFormatD2D()
{
}

void TextFormatD2D::Dispose()
{
	m_TextFormat.Reset();
	m_TextLayout.Reset();
	m_InlineEllipsis.Reset();

	m_ExtraHeight = 0.0f;
	m_LineGap = 0.0f;
}

void TextFormatD2D::CreateLayout(
	const WCHAR* str, UINT strLen, float maxW, float maxH, bool gdiEmulation)
{
	bool strChanged = false;
	if (strLen != m_LastString.length() ||
		memcmp(str, m_LastString.c_str(), (strLen + 1) * sizeof(WCHAR)) != 0)
	{
		strChanged = true;
		m_LastString.assign(str, strLen);
	}

	if (m_Trimming)
	{
		// GDI+ compatibility: If we trimming (i.e. clipping), GDI+ draws text lines even if they
		// would be clipped. This is arguably a bad 'feature', but some in some cases the height
		// might be just a pixel or two too small. In order to render those cases correctly (but
		// still clipped as CanvasD2D::DrawTextW() will clip), we'll increase the max height of
		// the layout.
		maxH += 2.0f;
	}

	if (m_TextLayout && !strChanged)
	{
		if (maxW != m_TextLayout->GetMaxWidth())
		{
			m_TextLayout->SetMaxWidth(maxW);
		}

		if (maxH != m_TextLayout->GetMaxHeight())
		{
			m_TextLayout->SetMaxHeight(maxH);
		}
	}
	else
	{
		CanvasD2D::c_DWFactory->CreateTextLayout(
			str, strLen, m_TextFormat.Get(), maxW, maxH, m_TextLayout.ReleaseAndGetAddressOf());

		if (m_TextLayout)
		{
			if (gdiEmulation)
			{
				Microsoft::WRL::ComPtr<IDWriteTextLayout1> textLayout1;
				m_TextLayout.As(&textLayout1);

				const float xOffset = m_TextFormat->GetFontSize() / 6.0f;
				const float emOffset = xOffset / 24.0f;
				const DWRITE_TEXT_RANGE range = {0, strLen};
				textLayout1->SetCharacterSpacing(emOffset, emOffset, 0.0f, range);
			}

			UINT32 lineCount = 0;
			DWRITE_LINE_METRICS lineMetrics[2];
			HRESULT hr = m_TextLayout->GetLineMetrics(lineMetrics, _countof(lineMetrics), &lineCount);
			if (SUCCEEDED(hr))
			{
				// If only one line is visible, disable wrapping so that as much text as possible is shown
				// after trimming.
				// TODO: Fix this for when more than one line is visible.
				if (lineCount >= 2 &&
					lineMetrics[0].isTrimmed &&
					lineMetrics[1].isTrimmed &&
					lineMetrics[1].height == 0.0f)
				{
					m_TextLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
				}
			}
		}
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
	const float dwriteFontSize = size * (4.0f / 3.0f);

	// |fontFamily| uses the GDI/GDI+ font naming convention so try to create DirectWrite font
	// using the GDI family name and then create a text format using the DirectWrite family name
	// obtained from it.
	HRESULT hr = Util::GetDWritePropertiesFromGDIProperties(
		CanvasD2D::c_DWFactory.Get(), fontFamily, bold, italic, dwriteFontWeight, dwriteFontStyle,
		dwriteFontStretch, dwriteFamilyName, _countof(dwriteFamilyName));
	if (SUCCEEDED(hr))
	{
		hr = CanvasD2D::c_DWFactory->CreateTextFormat(
			dwriteFamilyName,
			nullptr,
			dwriteFontWeight,
			dwriteFontStyle,
			dwriteFontStretch,
			dwriteFontSize,
			L"",
			&m_TextFormat);
	}

	if (FAILED(hr))
	{
		IDWriteFontCollection* dwriteFontCollection = nullptr;

		// If |fontFamily| is not in the system collection, use the font collection from
		// |fontCollectionD2D| if possible.
		if (!Util::IsFamilyInSystemFontCollection(CanvasD2D::c_DWFactory.Get(), fontFamily) &&
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
			dwriteFontSize,
			L"",
			&m_TextFormat);
	}

	if (SUCCEEDED(hr))
	{
		SetHorizontalAlignment(GetHorizontalAlignment());
		SetVerticalAlignment(GetVerticalAlignment());

		// Get the family name to in case CreateTextFormat() fallbacked on some other family name.
		hr = m_TextFormat->GetFontFamilyName(dwriteFamilyName, _countof(dwriteFamilyName));
		if (FAILED(hr)) return;

		Microsoft::WRL::ComPtr<IDWriteFontCollection> collection;
		Microsoft::WRL::ComPtr<IDWriteFontFamily> fontFamily;
		UINT32 familyNameIndex;
		BOOL exists;
		if (FAILED(m_TextFormat->GetFontCollection(collection.GetAddressOf())) ||
			FAILED(collection->FindFamilyName(dwriteFamilyName, &familyNameIndex, &exists)) ||
			FAILED(collection->GetFontFamily(familyNameIndex, fontFamily.GetAddressOf())))
		{
			return;
		}

		Microsoft::WRL::ComPtr<IDWriteFont> font;
		hr = fontFamily->GetFirstMatchingFont(
			m_TextFormat->GetFontWeight(),
			m_TextFormat->GetFontStretch(),
			m_TextFormat->GetFontStyle(),
			font.GetAddressOf());
		if (FAILED(hr)) return;

		DWRITE_FONT_METRICS fmetrics;
		font->GetMetrics(&fmetrics);

		// GDI+ compatibility: GDI+ adds extra padding below the string when |m_AccurateText| is
		// |false|. The bottom padding seems to be based on the font metrics so we can calculate it
		// once and keep using it regardless of the actual string. In some cases, GDI+ also adds
		// the line gap to the overall height so we will store it as well.
		const float pixelsPerDesignUnit =  dwriteFontSize / (float)fmetrics.designUnitsPerEm;
		m_ExtraHeight =
			(((float)fmetrics.designUnitsPerEm / 8.0f) - fmetrics.lineGap) * pixelsPerDesignUnit;
		m_LineGap = fmetrics.lineGap * pixelsPerDesignUnit;
	}
	else
	{
		Dispose();
	}
}

DWRITE_TEXT_METRICS TextFormatD2D::GetMetrics(
	const WCHAR* str, UINT strLen, bool gdiEmulation, float maxWidth)
{
	// GDI+ compatibility: If the last character is a newline, GDI+ measurements seem to ignore it.
	bool strippedLastNewLine = false;
	if (strLen > 2 && str[strLen - 1] == L'\n')
	{
		strippedLastNewLine = true;
		--strLen;

		if (str[strLen - 1] == L'\r')
		{
			--strLen;
		}
	}

	DWRITE_TEXT_METRICS metrics = {0};
	Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
	HRESULT hr = CanvasD2D::c_DWFactory->CreateTextLayout(
		str,
		strLen,
		m_TextFormat.Get(),
		maxWidth,
		10000,
		textLayout.GetAddressOf());
	if (SUCCEEDED(hr))
	{
		const float xOffset = m_TextFormat->GetFontSize() / 6.0f;
		if (gdiEmulation)
		{
			Microsoft::WRL::ComPtr<IDWriteTextLayout1> textLayout1;
			textLayout.As(&textLayout1);

			const float emOffset = xOffset / 24.0f;
			const DWRITE_TEXT_RANGE range = {0, strLen};
			textLayout1->SetCharacterSpacing(emOffset, emOffset, 0.0f, range);
		}

		textLayout->GetMetrics(&metrics);
		if (metrics.width > 0.0f)
		{
			if (gdiEmulation)
			{
				metrics.width += xOffset * 2;
				metrics.height += m_ExtraHeight;

				// GDI+ compatibility: If the string contains a newline (even if it is the
				// stripped last character), GDI+ adds the line gap to the overall height.
				if (strippedLastNewLine || wmemchr(str, L'\n', strLen) != nullptr)
				{
					metrics.height += m_LineGap;
				}
			}
			else
			{
				// GDI+ compatibility: With accurate metrics, the line gap needs to be subtracted
				// from the overall height if the string does not contain newlines.
				if (!strippedLastNewLine && wmemchr(str, L'\n', strLen) == nullptr)
				{
					metrics.height -= m_LineGap;
				}
			}
		}
		else
		{
			// GDI+ compatibility: Get rid of the height that DirectWrite assigns to zero-width
			// strings.
			metrics.height = 0.0f;
		}
	}

	return metrics;
}

void TextFormatD2D::SetTrimming(bool trim)
{
	m_Trimming = trim;
	IDWriteInlineObject* inlineObject = nullptr;
	DWRITE_TRIMMING trimming = {};
	DWRITE_WORD_WRAPPING wordWrapping = DWRITE_WORD_WRAPPING_NO_WRAP;
	if (trim)
	{
		if (!m_InlineEllipsis)
		{
			CanvasD2D::c_DWFactory->CreateEllipsisTrimmingSign(
				m_TextFormat.Get(), m_InlineEllipsis.GetAddressOf());
		}

		inlineObject = m_InlineEllipsis.Get();
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
