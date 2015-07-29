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

#include "StdAfx.h"
#include "TextFormatD2D.h"
#include "CanvasD2D.h"
#include "Util/DWriteHelpers.h"
#include "TextInlineFormat/TextInlineFormatCharacterSpacing.h"
#include "TextInlineFormat/TextInlineFormatColor.h"
#include "TextInlineFormat/TextInlineFormatFace.h"
#include "TextInlineFormat/TextInlineFormatGradientColor.h"
#include "TextInlineFormat/TextInlineFormatItalic.h"
#include "TextInlineFormat/TextInlineFormatOblique.h"
#include "TextInlineFormat/TextInlineFormatSize.h"
#include "TextInlineFormat/TextInlineFormatStretch.h"
#include "TextInlineFormat/TextInlineFormatStrikethrough.h"
#include "TextInlineFormat/TextInlineFormatTypography.h"
#include "TextInlineFormat/TextInlineFormatUnderline.h"
#include "TextInlineFormat/TextInlineFormatWeight.h"
#include "../StringUtil.h"
#include "../../Library/ConfigParser.h"
#include "../../Library/pcre-8.10/config.h"
#include "../../Library/pcre-8.10/pcre.h"
#include <ole2.h>  // For Gdiplus.h.
#include <GdiPlus.h>

namespace {

D2D1_COLOR_F ToColorF(const Gdiplus::Color& color)
{
	return D2D1::ColorF(color.GetR() / 255.0f, color.GetG() / 255.0f, color.GetB() / 255.0f, color.GetA() / 255.0f);
}

int Clamp(int value, int _min, int _max)
{
	if (value < _min || value > _max)
	{
		value = max(_min, value);
		value = min(value, _max);
	}

	return value;
}

}  // namespace

namespace Gfx {

TextFormatD2D::TextFormatD2D() :
	m_ExtraHeight(),
	m_LineGap(),
	m_Trimming(),
	m_HasInlineOptionsChanged(false)
{
}

TextFormatD2D::~TextFormatD2D()
{
	for (auto& fmt : m_TextInlineFormat)
	{
		delete fmt;
		fmt = nullptr;
	}

	m_TextInlineFormat.clear();
}

void TextFormatD2D::Dispose()
{
	m_TextFormat.Reset();
	m_TextLayout.Reset();
	m_InlineEllipsis.Reset();

	m_ExtraHeight = 0.0f;
	m_LineGap = 0.0f;
}

bool TextFormatD2D::CreateLayout(ID2D1RenderTarget* target,
	const WCHAR* str, UINT strLen, float maxW, float maxH, bool gdiEmulation)
{
	bool strChanged = false;
	if (strLen != m_LastString.length() ||
		memcmp(str, m_LastString.c_str(), (strLen + 1) * sizeof(WCHAR)) != 0)
	{
		strChanged = true;
		m_LastString.assign(str, strLen);
	}

	// The width and height of a DirectWrite layout must be non-negative.
	maxW = max(0.0f, maxW);
	maxH = max(0.0f, maxH);

	if (m_Trimming)
	{
		// GDI+ compatibility: If we trimming (i.e. clipping), GDI+ draws text lines even if they
		// would be clipped. This is arguably a bad 'feature', but some in some cases the height
		// might be just a pixel or two too small. In order to render those cases correctly (but
		// still clipped as CanvasD2D::DrawTextW() will clip), we'll increase the max height of
		// the layout.
		maxH += 2.0f;
	}

	if (m_TextLayout && !strChanged && !m_HasInlineOptionsChanged)
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
		if (!m_TextLayout) return false;

		if (gdiEmulation)
		{
			Microsoft::WRL::ComPtr<IDWriteTextLayout1> textLayout1;
			m_TextLayout.As(&textLayout1);

			const float xOffset = m_TextFormat->GetFontSize() / 6.0f;
			const float emOffset = xOffset / 24.0f;
			const DWRITE_TEXT_RANGE range = {0, strLen};
			textLayout1->SetCharacterSpacing(emOffset, emOffset, 0.0f, range);
		}

		ApplyInlineFormatting(m_TextLayout.Get());

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

		// Build gradient brushes (if any)
		for (const auto& fmt : m_TextInlineFormat)
		{
			if (fmt->GetType() == Gfx::InlineType::GradientColor)
			{
				TextInlineFormat_GradientColor* option = (TextInlineFormat_GradientColor*)fmt;
				option->BuildGradientBrushes(target, m_TextLayout.Get());
			}
		}
	}

	return true;
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
				if (SUCCEEDED(hr))
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

		// 'Face' inline objects need access to the font collection.
		for (auto& fmt : m_TextInlineFormat)
		{
			if (fmt->GetType() == Gfx::InlineType::Face)
			{
				TextInlineFormat_Face* face = (TextInlineFormat_Face*)fmt;
				face->SetFontCollection(fontCollectionD2D);
			}
		}
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
		ApplyInlineFormatting(textLayout.Get());

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

void TextFormatD2D::ReadInlineOptions(ConfigParser& parser, const WCHAR* section)
{
	const std::wstring delimiter(1, L'|');
	std::wstring option = parser.ReadString(section, L"InlineSetting", L"");
	std::wstring pattern = parser.ReadString(section, L"InlinePattern", L"");

	size_t i = 1;
	if (!option.empty() && !pattern.empty())
	{
		do
		{
			std::vector<std::wstring> args = ConfigParser::Tokenize(option, delimiter);
			if (!CreateInlineOption(i - 1, pattern, args)) break;

			// Check for InlineOption2/InlineValue2 ... etc.
			const std::wstring num = std::to_wstring(++i);

			std::wstring key = L"InlineSetting" + num;
			option = parser.ReadString(section, key.c_str(), L"");
			if (option.empty()) break;

			key = L"InlinePattern" + num;
			pattern = parser.ReadString(section, key.c_str(), L"");
		} while (!pattern.empty());
	}

	// Remove any previous options that do not exist anymore
	if (i <= m_TextInlineFormat.size())
	{
		std::vector<TextInlineFormat*>::iterator iter = m_TextInlineFormat.begin() + (i - 1);
		for (; iter != m_TextInlineFormat.end(); ++iter)
		{
			delete (*iter);
			(*iter) = nullptr;

			m_HasInlineOptionsChanged = true;
		}
		m_TextInlineFormat.erase(m_TextInlineFormat.begin() + (i - 1), m_TextInlineFormat.end());
	}
}

void TextFormatD2D::FindInlineRanges(const std::wstring& str)
{
	const WCHAR* buffer = str.c_str();

	for (auto& fmt : m_TextInlineFormat)
	{
		std::vector<DWRITE_TEXT_RANGE> ranges;
		std::string utf8str = StringUtil::NarrowUTF8(buffer);

		int ovector[300];
		const char* error;
		int errorOffset = 0;
		int offset = 0;
		pcre* re = pcre_compile(
			StringUtil::NarrowUTF8(fmt->GetPattern()).c_str(),
			PCRE_UTF8,
			&error,
			&errorOffset,
			nullptr);  // Use default character tables.
		if (!re)
		{
			//LogNoticeF(this, L"InlinePattern%i error at offset %d: %S", errorOffset, error);
		}
		else
		{
			do
			{
				const int rc = pcre_exec(
					re,
					nullptr,                // No extra data - we didn't study the pattern
					utf8str.c_str(),        // The subject string
					(int)utf8str.length(),  // The length of the subject
					offset,
					PCRE_NOTEMPTY,          // Empty string is not a valid match
					ovector,
					(int)_countof(ovector));
				if (rc <= 0)
				{
					break;
				}

				const UINT32 start = ovector[0];
				const UINT32 length = ovector[1] - ovector[0];

				// No captures found, but the rest of the text is still 'found'.
				if (rc == 1)
				{
					DWRITE_TEXT_RANGE range = { start, length };
					ranges.push_back(range);
				}
				else if (rc > 1)	// Captures found.
				{
					for (int j = rc - 1; j > 0; --j)
					{
						const UINT32 newStart = ovector[2 * j];
						const UINT32 inLength = ovector[2 * j + 1] - ovector[2 * j];

						if (newStart < 0) break;	// Match was not found, so skip to the next item

						DWRITE_TEXT_RANGE range = { newStart, inLength };
						ranges.push_back(range);
					}
				}

				offset = start + length;

			} while (true);

			pcre_free(re);

			// Gradients are set up differently then other options because they require 'inner ranges'
			// when text is split between multiple lines - otherwise set the range.
			if (fmt->GetType() == InlineType::GradientColor)
			{
				TextInlineFormat_GradientColor* linearGradient = (TextInlineFormat_GradientColor*)fmt;
				size_t index = 0;
				for (const auto& range : ranges)
				{
					linearGradient->UpdateSubOptions(index, range);
					++index;
				}
			}
			else
			{
				fmt->SetRanges(ranges);
			}
		}
	}
}

bool TextFormatD2D::CreateInlineOption(const size_t index, const std::wstring pattern, std::vector<std::wstring> options)
{
	if (options.empty()) return false;

	const size_t optSize = options.size();
	const WCHAR* option = options[0].c_str();
	if (_wcsnicmp(option, L"NONE", 4) == 0)
	{
		if (index < m_TextInlineFormat.size())
		{
			// Special case to delete a specific index while keeping the rest of the options
			delete m_TextInlineFormat[index];
			m_TextInlineFormat[index] = nullptr;
			m_TextInlineFormat.erase(m_TextInlineFormat.begin() + index);

			m_HasInlineOptionsChanged = true;
			return true;
		}
	}
	else if (_wcsicmp(option, L"CHARACTERSPACING") == 0)
	{
		if (optSize > 1)
		{
			FLOAT leading = (FLOAT)ConfigParser::ParseDouble(options[1].c_str(), FLT_MAX);
			FLOAT trailing = FLT_MAX;
			FLOAT advanceWidth = -1.0f;

			if (optSize > 2)
			{
				trailing = (FLOAT)ConfigParser::ParseDouble(options[2].c_str(), FLT_MAX);
			}

			if (optSize > 3)
			{
				advanceWidth = (FLOAT)ConfigParser::ParseDouble(options[3].c_str(), -1.0f);
			}

			UpdateInlineCharacterSpacing(index, pattern, leading, trailing, advanceWidth);
			return true;
		}
	}
	else if (_wcsicmp(option, L"COLOR") == 0)
	{
		if (optSize > 1)
		{
			Gdiplus::Color newColor = ConfigParser::ParseColor(options[1].c_str());
			UpdateInlineColor(index, pattern, newColor);
			return true;
		}
	}
	else if (_wcsicmp(option, L"FACE") == 0)
	{
		if (optSize > 1)
		{
			UpdateInlineFace(index, pattern, options[1].c_str());
			return true;
		}
	}
	else if (_wcsnicmp(option, L"GRADIENTCOLOR", 13) == 0)
	{
		if (optSize >= 3)
		{
			bool altGamma = ConfigParser::ParseInt(option + 13, 0) != 0;
			options.erase(options.begin());
			UpdateInlineGradientColor(index, pattern, options, altGamma);
			return true;
		}
	}
	else if(_wcsicmp(option, L"ITALIC") == 0)
	{
		UpdateInlineItalic(index, pattern);
		return true;
	}
	else if(_wcsicmp(option, L"OBLIQUE") == 0)
	{
		UpdateInlineOblique(index, pattern);
		return true;
	}
	else if(_wcsicmp(option, L"SIZE") == 0)
	{
		if (optSize > 1)
		{
			FLOAT size = (FLOAT)ConfigParser::ParseInt(options[1].c_str(), 0);
			UpdateInlineSize(index, pattern, size);
			return true;
		}
	}
	else if(_wcsicmp(option, L"STRETCH") == 0)
	{
		if (optSize > 1)
		{
			// DirectWrite supports 9 different stretch properties.
			DWRITE_FONT_STRETCH stretch = (DWRITE_FONT_STRETCH)
				Clamp(ConfigParser::ParseInt(options[1].c_str(), -1),
				(int)DWRITE_FONT_STRETCH_ULTRA_CONDENSED,
				(int)DWRITE_FONT_STRETCH_ULTRA_EXPANDED);
			UpdateInlineStretch(index, pattern, stretch);
			return true;
		}
	}
	else if(_wcsicmp(option, L"STRIKETHROUGH") == 0)
	{
		UpdateInlineStrikethrough(index, pattern);
		return true;
	}
	else if(_wcsicmp(option, L"TYPOGRAPHY") == 0)
	{
		// Typography 'tags' need to be extactly 4 characters.
		if (optSize > 1 && options[1].size() == 4)
		{
			UINT32 parameter = 1;
			DWRITE_FONT_FEATURE_TAG tag = (DWRITE_FONT_FEATURE_TAG)
				DWRITE_MAKE_OPENTYPE_TAG(options[1][0], options[1][1], options[1][2], options[1][3]);

			if (optSize > 2)
			{
				parameter = ConfigParser::ParseUInt(options[2].c_str(), 1u);
			}

			UpdateInlineTypography(index, pattern, tag, parameter);
			return true;
		}
	}
	else if(_wcsicmp(option, L"UNDERLINE") == 0)
	{
		UpdateInlineUnderline(index, pattern);
		return true;
	}
	else if(_wcsicmp(option, L"WEIGHT") == 0)
	{
		if (optSize > 1)
		{
			// DirectWrite supports weight from 1 to 999.
			DWRITE_FONT_WEIGHT weight = (DWRITE_FONT_WEIGHT)
				Clamp(ConfigParser::ParseInt(options[1].c_str(), -1), 1, 999);
			UpdateInlineWeight(index, pattern, weight);
			return true;
		}
	}

	return false;
}

void TextFormatD2D::UpdateInlineCharacterSpacing(const size_t& index, const std::wstring pattern,
	const FLOAT leading, const FLOAT trailing, const FLOAT advanceWidth)
{
	if (index >= m_TextInlineFormat.size())
	{
		// The |index| is larger than the number items in the array, so build a new
		// 'CharacterSpacing' object (in place) at the end of the array.

		m_TextInlineFormat.emplace_back(new TextInlineFormat_CharacterSpacing(pattern, leading, trailing, advanceWidth));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::CharacterSpacing)
	{
		// |index| is within range, and the type of object is also a 'CharacterSpacing'
		// object, so just update the object if needed.

		TextInlineFormat_CharacterSpacing* option = (TextInlineFormat_CharacterSpacing*)m_TextInlineFormat[index];
		if (option->CompareAndUpdateProperties(pattern, leading, trailing, advanceWidth))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		// |index| is within range, but the types of objects do not match, thus destroy
		// the previous object and replace it with a new 'CharacterSpacing' object.

		delete m_TextInlineFormat[index];
		m_TextInlineFormat[index] = new TextInlineFormat_CharacterSpacing(pattern, leading, trailing, advanceWidth);
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormatD2D::UpdateInlineColor(const size_t& index, const std::wstring pattern, const Gdiplus::Color color)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Color(pattern, color));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Color)
	{
		TextInlineFormat_Color* option = (TextInlineFormat_Color*)m_TextInlineFormat[index];
		if (option->CompareAndUpdateProperties(pattern, color))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		delete m_TextInlineFormat[index];
		m_TextInlineFormat[index] = new TextInlineFormat_Color(pattern, color);
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormatD2D::UpdateInlineFace(const size_t& index, const std::wstring pattern, const WCHAR* face)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Face(pattern, face));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Face)
	{
		TextInlineFormat_Face* option = (TextInlineFormat_Face*)m_TextInlineFormat[index];
		if (option->CompareAndUpdateProperties(pattern, face))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		delete m_TextInlineFormat[index];
		m_TextInlineFormat[index] = new TextInlineFormat_Face(pattern, face);
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormatD2D::UpdateInlineGradientColor(const size_t& index, const std::wstring pattern,
	const std::vector<std::wstring> args, const bool altGamma)
{
	const UINT32 angle = (360 + (ConfigParser::ParseInt(args[0].c_str(), 0) % 360)) % 360;

	std::vector<std::wstring> tokens;
	std::vector<D2D1_GRADIENT_STOP> stops(args.size() - 1);
	for (size_t i = 1; i < args.size(); ++i)
	{
		tokens = ConfigParser::Tokenize(args[i], L";");
		if (tokens.size() == 2)
		{
			stops[i - 1].color = ToColorF(ConfigParser::ParseColor(tokens[0].c_str()));
			stops[i - 1].position = (float)ConfigParser::ParseDouble(tokens[1].c_str(), 0.0f);
		}
	}

	// If gradient only has 1 stop, add a transparent stop at appropriate place
	if (stops.size() == 1)
	{
		D2D1::ColorF color = { 0.0f, 0.0f, 0.0f, 0.0f };
		D2D1_GRADIENT_STOP stop = { 0.0f, color };
		if (stops[0].position < 0.5)
		{
			stop.position = 1.0f;
		}

		stops.push_back(stop);
	}

	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_GradientColor(pattern, angle, stops, altGamma));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::GradientColor)
	{
		TextInlineFormat_GradientColor* option = (TextInlineFormat_GradientColor*)m_TextInlineFormat[index];
		if (option->CompareAndUpdateProperties(pattern, angle, stops, altGamma))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		delete m_TextInlineFormat[index];
		m_TextInlineFormat[index] = new TextInlineFormat_GradientColor(pattern, angle, stops, altGamma);
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormatD2D::UpdateInlineItalic(const size_t& index, const std::wstring pattern)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Italic(pattern));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Italic)
	{
		TextInlineFormat_Italic* option = (TextInlineFormat_Italic*)m_TextInlineFormat[index];
		if (option->CompareAndUpdateProperties(pattern))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		delete m_TextInlineFormat[index];
		m_TextInlineFormat[index] = new TextInlineFormat_Italic(pattern);
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormatD2D::UpdateInlineOblique(const size_t& index, const std::wstring pattern)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Oblique(pattern));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Oblique)
	{
		TextInlineFormat_Oblique* option = (TextInlineFormat_Oblique*)m_TextInlineFormat[index];
		if (option->CompareAndUpdateProperties(pattern))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		delete m_TextInlineFormat[index];
		m_TextInlineFormat[index] = new TextInlineFormat_Oblique(pattern);
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormatD2D::UpdateInlineSize(const size_t& index, const std::wstring pattern, const FLOAT size)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Size(pattern, size));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Size)
	{
		TextInlineFormat_Size* option = (TextInlineFormat_Size*)m_TextInlineFormat[index];
		if (option->CompareAndUpdateProperties(pattern, size))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		delete m_TextInlineFormat[index];
		m_TextInlineFormat[index] = new TextInlineFormat_Size(pattern, size);
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormatD2D::UpdateInlineStretch(const size_t& index, const std::wstring pattern, const DWRITE_FONT_STRETCH stretch)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Stretch(pattern, stretch));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Stretch)
	{
		TextInlineFormat_Stretch* option = (TextInlineFormat_Stretch*)m_TextInlineFormat[index];
		if (option->CompareAndUpdateProperties(pattern, stretch))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		delete m_TextInlineFormat[index];
		m_TextInlineFormat[index] = new TextInlineFormat_Stretch(pattern, stretch);
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormatD2D::UpdateInlineStrikethrough(const size_t& index, const std::wstring pattern)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Strikethrough(pattern));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Strikethrough)
	{
		TextInlineFormat_Strikethrough* option = (TextInlineFormat_Strikethrough*)m_TextInlineFormat[index];
		if (option->CompareAndUpdateProperties(pattern))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		delete m_TextInlineFormat[index];
		m_TextInlineFormat[index] = new TextInlineFormat_Strikethrough(pattern);
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormatD2D::UpdateInlineTypography(const size_t& index, const std::wstring pattern,
	const DWRITE_FONT_FEATURE_TAG tag, const UINT32 parameter)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Typography(pattern, tag, parameter));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Typography)
	{
		TextInlineFormat_Typography* option = (TextInlineFormat_Typography*)m_TextInlineFormat[index];
		if (option->CompareAndUpdateProperties(pattern, tag, parameter))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		delete m_TextInlineFormat[index];
		m_TextInlineFormat[index] = new TextInlineFormat_Typography(pattern, tag, parameter);
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormatD2D::UpdateInlineUnderline(const size_t& index, const std::wstring pattern)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Underline(pattern));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Underline)
	{
		TextInlineFormat_Underline* option = (TextInlineFormat_Underline*)m_TextInlineFormat[index];
		if (option->CompareAndUpdateProperties(pattern))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		delete m_TextInlineFormat[index];
		m_TextInlineFormat[index] = new TextInlineFormat_Underline(pattern);
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormatD2D::UpdateInlineWeight(const size_t& index, const std::wstring pattern, const DWRITE_FONT_WEIGHT weight)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Weight(pattern, weight));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Weight)
	{
		TextInlineFormat_Weight* option = (TextInlineFormat_Weight*)m_TextInlineFormat[index];
		if (option->CompareAndUpdateProperties(pattern, weight))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		delete m_TextInlineFormat[index];
		m_TextInlineFormat[index] = new TextInlineFormat_Weight(pattern, weight);
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormatD2D::ApplyInlineFormatting(IDWriteTextLayout* layout)
{
	for (const auto& fmt : m_TextInlineFormat)
	{
		Gfx::InlineType type = fmt->GetType();
		if (type != Gfx::InlineType::Color && type != Gfx::InlineType::GradientColor)
		{
			fmt->ApplyInlineFormat(layout);
		}
	}
}

void TextFormatD2D::ApplyInlineColoring(ID2D1RenderTarget* target, const D2D1_POINT_2F* point)
{
	// Color option
	for (const auto& fmt : m_TextInlineFormat)
	{
		if (fmt->GetType() == Gfx::InlineType::Color)
		{
			TextInlineFormat_Color* option = (TextInlineFormat_Color*)fmt;
			option->ApplyInlineFormat(target, m_TextLayout.Get());
		}
		else if (fmt->GetType() == Gfx::InlineType::GradientColor)
		{
			TextInlineFormat_GradientColor* option = (TextInlineFormat_GradientColor*)fmt;
			option->ApplyInlineFormat(m_TextLayout.Get(), point, m_HasInlineOptionsChanged);
		}
	}

	// Because it is expensive to recreate the text layout, we need some sort of way
	// to tell the 'format' that the inline options have changed. Here, we reset that
	// flag to false because the coloring of the text happen just before drawing.
	m_HasInlineOptionsChanged = false;
}

void TextFormatD2D::ResetInlineColoring(ID2D1SolidColorBrush* solidColor, const UINT strLen)
{
	DWRITE_TEXT_RANGE range = { 0, strLen };
	m_TextLayout->SetDrawingEffect(solidColor, range);
}

}  // namespace Gfx
