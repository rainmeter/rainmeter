// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "TextFormat.h"
#include "Canvas.h"
#include "Util/D2DUtil.h"
#include "Util/DWriteHelpers.h"
#include "TextInlineFormat/TextInlineFormatCase.h"
#include "TextInlineFormat/TextInlineFormatCharacterSpacing.h"
#include "TextInlineFormat/TextInlineFormatColor.h"
#include "TextInlineFormat/TextInlineFormatFace.h"
#include "TextInlineFormat/TextInlineFormatGradientColor.h"
#include "TextInlineFormat/TextInlineFormatItalic.h"
#include "TextInlineFormat/TextInlineFormatNone.h"
#include "TextInlineFormat/TextInlineFormatOblique.h"
#include "TextInlineFormat/TextInlineFormatShadow.h"
#include "TextInlineFormat/TextInlineFormatSize.h"
#include "TextInlineFormat/TextInlineFormatStretch.h"
#include "TextInlineFormat/TextInlineFormatStrikethrough.h"
#include "TextInlineFormat/TextInlineFormatTypography.h"
#include "TextInlineFormat/TextInlineFormatUnderline.h"
#include "TextInlineFormat/TextInlineFormatWeight.h"
#include "../ParseUtil.h"
#include "../StringParser.h"

namespace {

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

TextFormat::TextFormat(const MathParser& mathParser) :
	m_MathParser(mathParser),
	m_HorizontalAlignment(HorizontalAlignment::Left),
	m_VerticalAlignment(VerticalAlignment::Top),
	m_FontWeight(-1),
	m_HasWeightChanged(false),
	m_ExtraHeight(),
	m_LineGap(),
	m_Trimming(),
	m_HasInlineOptionsChanged(false)
{
}

TextFormat::~TextFormat()
{
	m_TextInlineFormat.clear();
}

void TextFormat::Dispose()
{
	m_TextFormat.Reset();
	m_TextLayout.Reset();
	m_InlineEllipsis.Reset();
	m_Font.Reset();

	m_ExtraHeight = 0.0f;
	m_LineGap = 0.0f;
}

void TextFormat::InvalidateDeviceResources()
{
	m_TextLayout.Reset();
	m_LastString.clear();
	m_HasInlineOptionsChanged = true;

	for (const auto& fmt : m_TextInlineFormat)
	{
		fmt->InvalidateDeviceResources();
	}
}

bool TextFormat::CreateLayout(ID2D1DeviceContext* target, const std::wstring& srcStr, float maxW, float maxH, bool gdiEmulation)
{
	UINT32 strLen = (UINT32)srcStr.length();
	const WCHAR* str = srcStr.c_str();

	bool strChanged = false;
	if (strLen != m_LastString.length() ||
		memcmp(str, m_LastString.c_str(), ((size_t)strLen + 1) * sizeof(WCHAR)) != 0)
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
		// still clipped as Canvas::DrawTextW() will clip), we'll increase the max height of
		// the layout.
		maxH += 2.0f;
	}

	// Inline gradients need to be created/recreated not only when the text changes,
	// but also when the dimensions of the meter changes.
	auto CreateGradientBrushes = [&]()
	{
		// Build gradient brushes (if any)
		for (const auto& fmt : m_TextInlineFormat)
		{
			if (fmt->GetType() == Gfx::InlineType::GradientColor)
			{
				auto option = (TextInlineFormat_GradientColor*)fmt.get();
				option->BuildGradientBrushes(target, m_TextLayout.Get());
			}
		}

		// Because the text layout can be created without any changes to any
		// 'color' inline options, we need a way to update any color changes
		// at drawing time.
		m_HasInlineOptionsChanged = true;
	};

	if (m_TextLayout && !strChanged && !m_HasInlineOptionsChanged && !m_HasWeightChanged)
	{
		bool hasChanged = false;
		if (maxW != m_TextLayout->GetMaxWidth())
		{
			m_TextLayout->SetMaxWidth(maxW);
			hasChanged = true;
		}

		if (maxH != m_TextLayout->GetMaxHeight())
		{
			m_TextLayout->SetMaxHeight(maxH);
			hasChanged = true;
		}

		if (hasChanged)
		{
			// Meter dimensions have changed, so recreate any inline gradient brushes
			CreateGradientBrushes();
		}
	}
	else
	{
		Canvas::c_DWFactory->CreateTextLayout(
			str, strLen, m_TextFormat.Get(), maxW, maxH, m_TextLayout.ReleaseAndGetAddressOf());
		if (!m_TextLayout) return false;

		// Set the font weight if valid
		const DWRITE_TEXT_RANGE range = { 0, strLen };
		if (m_FontWeight > 0 && m_FontWeight < 1000)
		{
			m_TextLayout->SetFontWeight((DWRITE_FONT_WEIGHT)m_FontWeight, range);
			m_HasWeightChanged = false;
		}

		if (gdiEmulation)
		{
			Microsoft::WRL::ComPtr<IDWriteTextLayout1> textLayout1;
			m_TextLayout.As(&textLayout1);

			const float xOffset = m_TextFormat->GetFontSize() / 6.0f;
			const float emOffset = xOffset / 24.0f;
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

		// Create any inline gradients
		CreateGradientBrushes();
	}

	return true;
}

Microsoft::WRL::ComPtr<IDWriteFont> TextFormat::ResolveFont() const
{
	// There is no way from a format to the font it resolved to, so the family name it ended up
	// with - not the one it was asked for - goes back through the collection it came from.
	WCHAR familyName[LF_FACESIZE];
	Microsoft::WRL::ComPtr<IDWriteFontCollection> collection;
	Microsoft::WRL::ComPtr<IDWriteFontFamily> family;
	Microsoft::WRL::ComPtr<IDWriteFont> font;
	UINT32 familyIndex = 0U;
	BOOL exists = FALSE;

	if (SUCCEEDED(m_TextFormat->GetFontFamilyName(familyName, _countof(familyName))) &&
		SUCCEEDED(m_TextFormat->GetFontCollection(collection.GetAddressOf())) &&
		SUCCEEDED(collection->FindFamilyName(familyName, &familyIndex, &exists)) && exists &&
		SUCCEEDED(collection->GetFontFamily(familyIndex, family.GetAddressOf())))
	{
		family->GetFirstMatchingFont(
			m_TextFormat->GetFontWeight(),
			m_TextFormat->GetFontStretch(),
			m_TextFormat->GetFontStyle(),
			font.GetAddressOf());
	}

	return font;
}

bool TextFormat::HasCharacter(UINT32 ch) const
{
	if (!m_Font) return false;

	BOOL exists = FALSE;
	return SUCCEEDED(m_Font->HasCharacter(ch, &exists)) && exists != FALSE;
}

void TextFormat::SetProperties(
	const WCHAR* fontFamily, FLOAT size, bool bold, bool italic,
	FontCollection* fontCollection)
{
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
		Canvas::c_DWFactory.Get(), fontFamily, bold, italic, dwriteFontWeight, dwriteFontStyle,
		dwriteFontStretch, dwriteFamilyName, _countof(dwriteFamilyName));
	if (SUCCEEDED(hr))
	{
		hr = Canvas::c_DWFactory->CreateTextFormat(
			dwriteFamilyName,
			nullptr,
			dwriteFontWeight,
			dwriteFontStyle,
			dwriteFontStretch,
			dwriteFontSize,
			L"",
			m_TextFormat.GetAddressOf());
	}

	if (FAILED(hr))
	{
		IDWriteFontCollection* dwriteFontCollection = nullptr;

		// If |fontFamily| is not in the system collection, use the font collection from
		// |fontCollection| if possible.
		if (!Util::IsFamilyInSystemFontCollection(Canvas::c_DWFactory.Get(), fontFamily) &&
			(fontCollection && fontCollection->InitializeCollection()))
		{
			IDWriteFont* dwriteFont = Util::FindDWriteFontInFontCollectionByGDIFamilyName(
				fontCollection->m_Collection, fontFamily);
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

			dwriteFontCollection = fontCollection->m_Collection;
		}

		// Fallback in case above fails.
		hr = Canvas::c_DWFactory->CreateTextFormat(
			fontFamily,
			dwriteFontCollection,
			dwriteFontWeight,
			dwriteFontStyle,
			dwriteFontStretch,
			dwriteFontSize,
			L"",
			m_TextFormat.GetAddressOf());
	}

	if (SUCCEEDED(hr))
	{
		SetHorizontalAlignment(GetHorizontalAlignment());
		SetVerticalAlignment(GetVerticalAlignment());

		// Resolved from the format rather than from what was asked for, since CreateTextFormat()
		// may have fallen back on some other family name.
		m_Font = ResolveFont();
		if (!m_Font) return;

		DWRITE_FONT_METRICS fmetrics;
		m_Font->GetMetrics(&fmetrics);

		// GDI+ compatibility: GDI+ adds extra padding below the string when |m_AccurateText| is
		// |false|. The bottom padding seems to be based on the font metrics so we can calculate it
		// once and keep using it regardless of the actual string. In some cases, GDI+ also adds
		// the line gap to the overall height so we will store it as well.
		const float pixelsPerDesignUnit = dwriteFontSize / (float)fmetrics.designUnitsPerEm;
		m_ExtraHeight =
			(((float)fmetrics.designUnitsPerEm / 8.0f) - fmetrics.lineGap) * pixelsPerDesignUnit;
		m_LineGap = fmetrics.lineGap * pixelsPerDesignUnit;

		// 'Face' inline objects need access to the font collection.
		for (auto& fmt : m_TextInlineFormat)
		{
			if (fmt->GetType() == Gfx::InlineType::Face)
			{
				auto face = (TextInlineFormat_Face*)fmt.get();
				face->SetFontCollection(fontCollection);
			}
		}
	}
	else
	{
		Dispose();
	}
}

void TextFormat::SetFontWeight(int weight)
{
	if (weight < 1 || weight > 999 || weight == m_FontWeight) return;

	m_FontWeight = weight;

	// Signal to recreate the layout
	m_HasWeightChanged = true;
}

DWRITE_TEXT_METRICS TextFormat::GetMetrics(const std::wstring& srcStr, bool gdiEmulation, float maxWidth)
{
	UINT32 strLen = (UINT32)srcStr.length();
	const WCHAR* str = srcStr.c_str();

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

	DWRITE_TEXT_METRICS metrics = { 0 };
	Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
	HRESULT hr = Canvas::c_DWFactory->CreateTextLayout(
		str,
		strLen,
		m_TextFormat.Get(),
		maxWidth,
		10000.0f,
		textLayout.GetAddressOf());
	if (SUCCEEDED(hr))
	{
		// Set the font weight if valid
		if (m_FontWeight > 0 && m_FontWeight < 1000)
		{
			const DWRITE_TEXT_RANGE range = { 0, strLen };
			textLayout->SetFontWeight((DWRITE_FONT_WEIGHT)m_FontWeight, range);
		}

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
				metrics.width += xOffset * 2.0f;
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
				// Make sure the fractional part of the width/height metrics are included
				// in the calculation.
				metrics.width = std::ceil(metrics.width);
				metrics.height = std::ceil(metrics.height);

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

void TextFormat::SetTrimming(bool trim)
{
	m_Trimming = trim;
	IDWriteInlineObject* inlineObject = nullptr;
	DWRITE_TRIMMING trimming = {};
	if (trim)
	{
		if (!m_InlineEllipsis)
		{
			Canvas::c_DWFactory->CreateEllipsisTrimmingSign(
				m_TextFormat.Get(), m_InlineEllipsis.GetAddressOf());
		}

		inlineObject = m_InlineEllipsis.Get();
		trimming.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
	}

	m_TextFormat->SetTrimming(&trimming, inlineObject);
	UpdateWordWrapping();
}

void TextFormat::SetHorizontalAlignment(HorizontalAlignment alignment)
{
	m_HorizontalAlignment = alignment;

	if (m_TextFormat)
	{
		m_TextFormat->SetTextAlignment(
			(alignment == HorizontalAlignment::Left) ? DWRITE_TEXT_ALIGNMENT_LEADING :
			(alignment == HorizontalAlignment::Center) ? DWRITE_TEXT_ALIGNMENT_CENTER :
			(alignment == HorizontalAlignment::Justify) ? DWRITE_TEXT_ALIGNMENT_JUSTIFIED :
			DWRITE_TEXT_ALIGNMENT_TRAILING);

		UpdateWordWrapping();
	}
}

void TextFormat::UpdateWordWrapping()
{
	if (!m_TextFormat) return;

	// Justified text has nothing to stretch unless lines are allowed to wrap.
	const bool wrap = m_Trimming || m_HorizontalAlignment == HorizontalAlignment::Justify;
	m_TextFormat->SetWordWrapping(wrap ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
}

void TextFormat::SetVerticalAlignment(VerticalAlignment alignment)
{
	m_VerticalAlignment = alignment;

	if (m_TextFormat)
	{
		m_TextFormat->SetParagraphAlignment(
			(alignment == VerticalAlignment::Top) ? DWRITE_PARAGRAPH_ALIGNMENT_NEAR :
			(alignment == VerticalAlignment::Center) ? DWRITE_PARAGRAPH_ALIGNMENT_CENTER :
			DWRITE_PARAGRAPH_ALIGNMENT_FAR);
	}
}

void TextFormat::SetInlineOptions(const std::vector<TextInlineOption>& options)
{
	size_t i = 0;
	for (; i < options.size(); ++i)
	{
		std::wstring pattern = options[i].pattern.empty() ? L".*" : options[i].pattern;
		if (!CreateInlineOption(i, pattern, options[i].settings)) break;
	}

	// Remove any previous options that do not exist anymore
	if (i < m_TextInlineFormat.size())
	{
		m_HasInlineOptionsChanged = true;
		m_TextInlineFormat.erase(m_TextInlineFormat.begin() + i, m_TextInlineFormat.end());
	}
}

std::vector<std::wstring> TextFormat::GetInlinePatterns()
{
	std::vector<std::wstring> patterns;
	patterns.reserve(m_TextInlineFormat.size());
	for (auto& fmt : m_TextInlineFormat)
	{
		patterns.push_back(fmt->GetPattern());
	}

	return patterns;
}

void TextFormat::SetInlineRanges(const std::vector<std::vector<TextInlineRange>>& ranges)
{
	const size_t count = min(m_TextInlineFormat.size(), ranges.size());
	for (size_t i = 0; i < count; ++i)
	{
		std::vector<DWRITE_TEXT_RANGE> dwriteRanges;
		dwriteRanges.reserve(ranges[i].size());
		for (const auto& range : ranges[i])
		{
			DWRITE_TEXT_RANGE dwriteRange = { range.start, range.length };
			dwriteRanges.push_back(dwriteRange);
		}

		// Gradients are set up differently then other options because they require 'inner ranges'
		// when text is split between multiple lines - otherwise set the range.
		if (m_TextInlineFormat[i]->GetType() == InlineType::GradientColor)
		{
			auto linearGradient = (TextInlineFormat_GradientColor*)m_TextInlineFormat[i].get();
			size_t index = 0;
			for (const auto& range : dwriteRanges)
			{
				linearGradient->UpdateSubOptions(index, range);
				++index;
			}
		}
		else
		{
			m_TextInlineFormat[i]->SetRanges(dwriteRanges);
		}
	}
}

bool TextFormat::CreateInlineOption(const size_t index, const std::wstring pattern, std::vector<std::wstring> options)
{
	if (options.empty()) return false;

	const size_t optSize = options.size();
	const WCHAR* option = options[0].c_str();
	if (_wcsnicmp(option, L"NONE", 4) == 0)
	{
		UpdateInlineNone(index, pattern);
		return true;
	}
	else if (_wcsicmp(option, L"CASE") == 0)
	{
		if (optSize > 1)
		{
			const WCHAR* strCase = options[1].c_str();
			CaseType type = CaseType::None;

			if (_wcsicmp(strCase, L"LOWER") == 0) type = Gfx::CaseType::Lower;
			else if (_wcsicmp(strCase, L"UPPER") == 0) type = Gfx::CaseType::Upper;
			else if (_wcsicmp(strCase, L"PROPER") == 0) type = Gfx::CaseType::Proper;
			else if (_wcsicmp(strCase, L"SENTENCE") == 0) type = Gfx::CaseType::Sentence;

			// Only allow the above options.
			if (type == Gfx::CaseType::None) return false;

			UpdateInlineCase(index, pattern, type);
			return true;
		}
	}
	else if (_wcsicmp(option, L"CHARACTERSPACING") == 0)
	{
		if (optSize > 1)
		{
			const MathParser& mathParser = m_MathParser;
			auto parseOptional = [&mathParser](const WCHAR* value) -> FLOAT
			{
				if (_wcsnicmp(value, L"*", 1) == 0) return FLT_MAX;
				return (FLOAT)ParseUtil::ParseDouble(value, FLT_MAX, mathParser);
			};

			FLOAT leading = parseOptional(options[1].c_str());
			FLOAT trailing = FLT_MAX;
			FLOAT advanceWidth = -1.0f;

			if (optSize > 2)
			{
				trailing = parseOptional(options[2].c_str());
			}

			if (optSize > 3)
			{
				advanceWidth = (FLOAT)ParseUtil::ParseDouble(options[3].c_str(), -1.0f, m_MathParser);
			}

			UpdateInlineCharacterSpacing(index, pattern, leading, trailing, advanceWidth);
			return true;
		}
	}
	else if (_wcsicmp(option, L"COLOR") == 0)
	{
		if (optSize > 1)
		{
			D2D1_COLOR_F newColor = ParseUtil::ParseColor(options[1].c_str(), m_MathParser);
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
			bool altGamma = ParseUtil::ParseInt(option + 13, 0, m_MathParser) != 0;
			options.erase(options.begin());
			UpdateInlineGradientColor(index, pattern, options, altGamma);
			return true;
		}
	}
	else if (_wcsicmp(option, L"ITALIC") == 0)
	{
		UpdateInlineItalic(index, pattern);
		return true;
	}
	else if (_wcsicmp(option, L"OBLIQUE") == 0)
	{
		UpdateInlineOblique(index, pattern);
		return true;
	}
	else if (_wcsicmp(option, L"SHADOW") == 0)
	{
		if (optSize >= 5)
		{
			D2D1_POINT_2F offset = {
				(FLOAT)ParseUtil::ParseDouble(options[1].c_str(), 1.0, m_MathParser),
				(FLOAT)ParseUtil::ParseDouble(options[2].c_str(), 1.0, m_MathParser) };

			FLOAT blur = (FLOAT)ParseUtil::ParseDouble(options[3].c_str(), 3.0, m_MathParser);
			D2D1_COLOR_F color = ParseUtil::ParseColor(options[4].c_str(), m_MathParser);
			UpdateInlineShadow(index, pattern, blur, offset, color);
			return true;
		}
	}
	else if (_wcsicmp(option, L"SIZE") == 0)
	{
		if (optSize > 1)
		{
			FLOAT size = (FLOAT)ParseUtil::ParseDouble(options[1].c_str(), 10.0, m_MathParser);
			UpdateInlineSize(index, pattern, size);
			return true;
		}
	}
	else if (_wcsicmp(option, L"STRETCH") == 0)
	{
		if (optSize > 1)
		{
			// DirectWrite supports 9 different stretch properties.
			DWRITE_FONT_STRETCH stretch = (DWRITE_FONT_STRETCH)
				Clamp(ParseUtil::ParseInt(options[1].c_str(), -1, m_MathParser),
				(int)DWRITE_FONT_STRETCH_ULTRA_CONDENSED,
				(int)DWRITE_FONT_STRETCH_ULTRA_EXPANDED);
			UpdateInlineStretch(index, pattern, stretch);
			return true;
		}
	}
	else if (_wcsicmp(option, L"STRIKETHROUGH") == 0)
	{
		UpdateInlineStrikethrough(index, pattern);
		return true;
	}
	else if (_wcsicmp(option, L"TYPOGRAPHY") == 0)
	{
		// Typography 'tags' need to be extactly 4 characters.
		if (optSize > 1 && options[1].size() == 4)
		{
			UINT32 parameter = 1;
			DWRITE_FONT_FEATURE_TAG tag = (DWRITE_FONT_FEATURE_TAG)
				DWRITE_MAKE_OPENTYPE_TAG(options[1][0], options[1][1], options[1][2], options[1][3]);

			if (optSize > 2)
			{
				parameter = ParseUtil::ParseUInt(options[2].c_str(), 1u, m_MathParser);
			}

			UpdateInlineTypography(index, pattern, tag, parameter);
			return true;
		}
	}
	else if (_wcsicmp(option, L"UNDERLINE") == 0)
	{
		UpdateInlineUnderline(index, pattern);
		return true;
	}
	else if (_wcsicmp(option, L"WEIGHT") == 0)
	{
		if (optSize > 1)
		{
			// DirectWrite supports weight from 1 to 999.
			DWRITE_FONT_WEIGHT weight = (DWRITE_FONT_WEIGHT)
				Clamp(ParseUtil::ParseInt(options[1].c_str(), -1, m_MathParser), 1, 999);
			UpdateInlineWeight(index, pattern, weight);
			return true;
		}
	}

	return false;
}

void TextFormat::UpdateInlineCase(const size_t& index, const std::wstring pattern, const Gfx::CaseType type)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Case(pattern, type));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Case)
	{
		auto option = (TextInlineFormat_Case*)m_TextInlineFormat[index].get();
		if (option->CompareAndUpdateProperties(pattern, type))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		m_TextInlineFormat[index].reset(new TextInlineFormat_Case(pattern, type));
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormat::UpdateInlineCharacterSpacing(const size_t& index, const std::wstring pattern,
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

		auto option = (TextInlineFormat_CharacterSpacing*)m_TextInlineFormat[index].get();
		if (option->CompareAndUpdateProperties(pattern, leading, trailing, advanceWidth))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		// |index| is within range, but the types of objects do not match, thus destroy
		// the previous object and replace it with a new 'CharacterSpacing' object.

		m_TextInlineFormat[index].reset(new TextInlineFormat_CharacterSpacing(pattern, leading, trailing, advanceWidth));
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormat::UpdateInlineColor(const size_t& index, const std::wstring pattern, const D2D1_COLOR_F& color)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Color(pattern, color));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Color)
	{
		auto option = (TextInlineFormat_Color*)m_TextInlineFormat[index].get();
		if (option->CompareAndUpdateProperties(pattern, color))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		m_TextInlineFormat[index].reset(new TextInlineFormat_Color(pattern, color));
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormat::UpdateInlineFace(const size_t& index, const std::wstring pattern, const WCHAR* face)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Face(pattern, face));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Face)
	{
		auto option = (TextInlineFormat_Face*)m_TextInlineFormat[index].get();
		if (option->CompareAndUpdateProperties(pattern, face))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		m_TextInlineFormat[index].reset(new TextInlineFormat_Face(pattern, face));
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormat::UpdateInlineGradientColor(const size_t& index, const std::wstring pattern,
	const std::vector<std::wstring> args, const bool altGamma)
{
	const FLOAT angle = (FLOAT)fmod((360.0 + fmod(ParseUtil::ParseDouble(args[0].c_str(), 0.0, m_MathParser), 360.0)), 360.0);

	std::vector<D2D1_GRADIENT_STOP> stops(args.size() - 1);
	for (size_t i = 1; i < args.size(); ++i)
	{
		const auto consumeOption = StringParser::SkipWhitespace | StringParser::SkipNestedParentheses;

		// A stop is a color and a position, "Color;Position".
		StringParser values(args[i]);
		const auto color = values.ConsumeUntil(L';', consumeOption);
		values.ConsumeWhitespace();
		if (values.IsConsumed()) continue;

		const auto position = values.ConsumeUntilOrRest(L';', consumeOption);
		values.ConsumeWhitespace();
		if (!values.IsConsumed()) continue;

		stops[i - 1].color = ParseUtil::ParseColor(color, m_MathParser);
		stops[i - 1].position = (FLOAT)ParseUtil::ParseDouble(position, 0.0, m_MathParser);
	}

	// If gradient only has 1 stop, add a transparent stop at appropriate place
	if (stops.size() == 1)
	{
		D2D1::ColorF color = { 0.0f, 0.0f, 0.0f, 0.0f };
		D2D1_GRADIENT_STOP stop = { 0.0f, color };
		if (stops[0].position < 0.5f)
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
		auto option = (TextInlineFormat_GradientColor*)m_TextInlineFormat[index].get();
		if (option->CompareAndUpdateProperties(pattern, angle, stops, altGamma))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		m_TextInlineFormat[index].reset(new TextInlineFormat_GradientColor(pattern, angle, stops, altGamma));
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormat::UpdateInlineItalic(const size_t& index, const std::wstring pattern)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Italic(pattern));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Italic)
	{
		auto option = (TextInlineFormat_Italic*)m_TextInlineFormat[index].get();
		if (option->CompareAndUpdateProperties(pattern))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		m_TextInlineFormat[index].reset(new TextInlineFormat_Italic(pattern));
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormat::UpdateInlineNone(const size_t & index, const std::wstring pattern)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_None(pattern));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::None)
	{
		auto option = (TextInlineFormat_None*)m_TextInlineFormat[index].get();
		if (option->CompareAndUpdateProperties(pattern))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		m_TextInlineFormat[index].reset(new TextInlineFormat_None(pattern));
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormat::UpdateInlineOblique(const size_t& index, const std::wstring pattern)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Oblique(pattern));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Oblique)
	{
		auto option = (TextInlineFormat_Oblique*)m_TextInlineFormat[index].get();
		if (option->CompareAndUpdateProperties(pattern))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		m_TextInlineFormat[index].reset(new TextInlineFormat_Oblique(pattern));
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormat::UpdateInlineShadow(const size_t& index, const std::wstring pattern,
	const FLOAT blur, const D2D1_POINT_2F offset, const D2D1_COLOR_F& color)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Shadow(pattern, blur, offset, color));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Shadow)
	{
		auto option = (TextInlineFormat_Shadow*)m_TextInlineFormat[index].get();
		if (option->CompareAndUpdateProperties(pattern, blur, offset, color))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		m_TextInlineFormat[index].reset(new TextInlineFormat_Shadow(pattern, blur, offset, color));
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormat::UpdateInlineSize(const size_t& index, const std::wstring pattern, const FLOAT size)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Size(pattern, size));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Size)
	{
		auto option = (TextInlineFormat_Size*)m_TextInlineFormat[index].get();
		if (option->CompareAndUpdateProperties(pattern, size))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		m_TextInlineFormat[index].reset(new TextInlineFormat_Size(pattern, size));
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormat::UpdateInlineStretch(const size_t& index, const std::wstring pattern, const DWRITE_FONT_STRETCH stretch)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Stretch(pattern, stretch));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Stretch)
	{
		auto option = (TextInlineFormat_Stretch*)m_TextInlineFormat[index].get();
		if (option->CompareAndUpdateProperties(pattern, stretch))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		m_TextInlineFormat[index].reset(new TextInlineFormat_Stretch(pattern, stretch));
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormat::UpdateInlineStrikethrough(const size_t& index, const std::wstring pattern)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Strikethrough(pattern));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Strikethrough)
	{
		auto option = (TextInlineFormat_Strikethrough*)m_TextInlineFormat[index].get();
		if (option->CompareAndUpdateProperties(pattern))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		m_TextInlineFormat[index].reset(new TextInlineFormat_Strikethrough(pattern));
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormat::UpdateInlineTypography(const size_t& index, const std::wstring pattern,
	const DWRITE_FONT_FEATURE_TAG tag, const UINT32 parameter)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Typography(pattern, tag, parameter));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Typography)
	{
		auto option = (TextInlineFormat_Typography*)m_TextInlineFormat[index].get();
		if (option->CompareAndUpdateProperties(pattern, tag, parameter))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		m_TextInlineFormat[index].reset(new TextInlineFormat_Typography(pattern, tag, parameter));
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormat::UpdateInlineUnderline(const size_t& index, const std::wstring pattern)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Underline(pattern));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Underline)
	{
		auto option = (TextInlineFormat_Underline*)m_TextInlineFormat[index].get();
		if (option->CompareAndUpdateProperties(pattern))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		m_TextInlineFormat[index].reset(new TextInlineFormat_Underline(pattern));
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormat::UpdateInlineWeight(const size_t& index, const std::wstring pattern, const DWRITE_FONT_WEIGHT weight)
{
	if (index >= m_TextInlineFormat.size())
	{
		m_TextInlineFormat.emplace_back(new TextInlineFormat_Weight(pattern, weight));
		m_HasInlineOptionsChanged = true;
	}
	else if (m_TextInlineFormat[index]->GetType() == Gfx::InlineType::Weight)
	{
		auto option = (TextInlineFormat_Weight*)m_TextInlineFormat[index].get();
		if (option->CompareAndUpdateProperties(pattern, weight))
		{
			m_HasInlineOptionsChanged = true;
		}
	}
	else
	{
		m_TextInlineFormat[index].reset(new TextInlineFormat_Weight(pattern, weight));
		m_HasInlineOptionsChanged = true;
	}
}

void TextFormat::ApplyInlineFormatting(IDWriteTextLayout* layout)
{
	for (const auto& fmt : m_TextInlineFormat)
	{
		Gfx::InlineType type = fmt->GetType();
		if (type != Gfx::InlineType::Color &&
			type != Gfx::InlineType::GradientColor &&
			type != Gfx::InlineType::Case &&
			type != Gfx::InlineType::Shadow)
		{
			fmt->ApplyInlineFormat(layout);
		}
	}
}

void TextFormat::ApplyInlineColoring(ID2D1DeviceContext* target, const D2D1_POINT_2F* point)
{
	// Color option
	for (const auto& fmt : m_TextInlineFormat)
	{
		if (fmt->GetType() == Gfx::InlineType::Color)
		{
			auto option = (TextInlineFormat_Color*)fmt.get();
			option->ApplyInlineFormat(target, m_TextLayout.Get());
		}
		else if (fmt->GetType() == Gfx::InlineType::GradientColor)
		{
			auto option = (TextInlineFormat_GradientColor*)fmt.get();
			option->ApplyInlineFormat(m_TextLayout.Get(), point);
		}
	}

	// Because it is expensive to recreate the text layout, we need some sort of way
	// to tell the 'format' that the inline options have changed. Here, we reset that
	// flag to false because the coloring of the text happen just before drawing.
	m_HasInlineOptionsChanged = false;
}

const std::wstring& TextFormat::ApplyInlineCase(const std::wstring& srcStr, std::wstring& buffer)
{
	// Most strings have no case option at all, so |buffer| is only filled (and |srcStr| only
	// copied) once one is actually found.
	const std::wstring* str = &srcStr;

	for (const auto& fmt : m_TextInlineFormat)
	{
		if (fmt->GetType() == Gfx::InlineType::Case)
		{
			if (str != &buffer)
			{
				buffer.assign(srcStr);
				str = &buffer;
			}

			auto option = (TextInlineFormat_Case*)fmt.get();
			option->ApplyInlineFormat(buffer);
		}
	}

	return *str;
}

void TextFormat::ApplyInlineShadow(ID2D1DeviceContext* target, ID2D1SolidColorBrush* solidBrush,
	const UINT32 strLen, const D2D1_RECT_F& drawRect)
{
	for (const auto& fmt : m_TextInlineFormat)
	{
		if (fmt->GetType() == Gfx::InlineType::Shadow)
		{
			auto option = (TextInlineFormat_Shadow*)fmt.get();
			option->ApplyInlineFormat(target, m_TextLayout.Get(), solidBrush, strLen, drawRect);

			// We need to reset the color options after the shadow effect because the shadow effect
			// can turn some characters invisible.
			D2D1_POINT_2F drawPosition = D2D1::Point2F(drawRect.left, drawRect.top);

			ResetInlineColoring(solidBrush, strLen);
			ResetGradientPosition(&drawPosition);
			ApplyInlineColoring(target, &drawPosition);
		}
	}
}

void TextFormat::ResetGradientPosition(const D2D1_POINT_2F* point)
{
	for (const auto& fmt : m_TextInlineFormat)
	{
		if (fmt->GetType() == Gfx::InlineType::GradientColor)
		{
			auto option = (TextInlineFormat_GradientColor*)fmt.get();
			option->ApplyInlineFormat(nullptr, point, false);
		}
	}
}

void TextFormat::ResetInlineColoring(ID2D1SolidColorBrush* solidColor, const UINT32 strLen)
{
	DWRITE_TEXT_RANGE range = { 0, strLen };
	m_TextLayout->SetDrawingEffect(solidColor, range);
}

}  // namespace Gfx
