// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "TextFormat.h"
#include "Canvas.h"
#include "Util/D2DUtil.h"
#include "Util/DWriteHelpers.h"
#include "FontCollection.h"
#include "../StringUtil.h"

namespace {

template<typename... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };

}  // namespace

namespace Gfx {

namespace {

// The ranges an option applies to. Empty ranges are not applied to anything.
template<typename Func>
void ForEachRange(const std::vector<DWRITE_TEXT_RANGE>& ranges, Func&& func)
{
	for (const auto& range : ranges)
	{
		if (range.length <= 0) continue;

		func(range);
	}
}

// The device resources |state| draws with, built on first use.
template<typename T>
T& GetCache(TextInlineOptionState& state)
{
	if (!std::holds_alternative<T>(state.cache))
	{
		state.cache.emplace<T>();
	}

	return std::get<T>(state.cache);
}

HRESULT GetHitTestMetrics(IDWriteTextLayout* layout, std::vector<DWRITE_HIT_TEST_METRICS>& metrics,
	const DWRITE_TEXT_RANGE& range)
{
	UINT32 count = 0;
	HRESULT hr = layout->HitTestTextRange(range.startPosition, range.length, 0, 0, nullptr, 0, &count);
	if (FAILED(hr))
	{
		// The first call fails to tell how many metrics there are to ask for.
		if (count == 0) return hr;

		metrics.resize(count);
		hr = layout->HitTestTextRange(range.startPosition, range.length, 0, 0, &metrics[0],
			(UINT32)metrics.size(), &count);
	}

	return hr;
}

void BuildInlineGradientBrushes(const InlineSetting::GradientColor& setting,
	const std::vector<DWRITE_TEXT_RANGE>& ranges, InlineGradientCache& cache,
	ID2D1DeviceContext* target, IDWriteTextLayout* layout)
{
	// The brushes belong to the layout they were built from, so they all go before new ones are
	// built, rather than being kept for the ranges that happen to be unchanged.
	cache.subs.clear();

	if (!target || !layout || setting.stops.empty()) return;

	cache.subs.resize(ranges.size());

	for (size_t i = 0; i < ranges.size(); ++i)
	{
		if (ranges[i].length <= 0) continue;

		std::vector<DWRITE_HIT_TEST_METRICS> metrics;
		if (FAILED(GetHitTestMetrics(layout, metrics, ranges[i]))) continue;

		auto& sub = cache.subs[i];
		for (const auto& hit : metrics)
		{
			Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> collection;
			HRESULT hr = target->CreateGradientStopCollection(
				&setting.stops[0],
				(UINT32)setting.stops.size(),
				setting.altGamma ? D2D1_GAMMA_1_0 : D2D1_GAMMA_2_2,
				D2D1_EXTEND_MODE_CLAMP,
				collection.GetAddressOf());

			if (FAILED(hr)) continue;

			const D2D1_POINT_2F start = Util::FindEdgePoint(
				setting.angle, hit.left, hit.top, hit.width + hit.left, hit.height + hit.top);
			const D2D1_POINT_2F end = Util::FindEdgePoint(
				setting.angle + 180.0f, hit.left, hit.top, hit.width + hit.left, hit.height + hit.top);

			Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> gradientBrush;
			hr = target->CreateLinearGradientBrush(
				D2D1::LinearGradientBrushProperties(start, end),
				collection.Get(),
				gradientBrush.GetAddressOf());

			if (FAILED(hr)) continue;

			sub.innerRanges.push_back({ hit.textPosition, hit.length });
			sub.brushes.push_back(gradientBrush);
		}
	}
}

void ApplyInlineGradient(InlineGradientCache& cache, IDWriteTextLayout* layout,
	const D2D1_POINT_2F* point, bool beforeDrawing = true)
{
	if (!point || (beforeDrawing && !layout)) return;

	// Because the gradient needs to know the drawing position, we need a way to set that position
	// before drawing time, and then remove that same position after drawing time in case the
	// position changes on the next iteration.
	const FLOAT sign = beforeDrawing ? 1.0f : -1.0f;

	for (auto& sub : cache.subs)
	{
		for (size_t i = 0; i < sub.brushes.size(); ++i)
		{
			const auto& brush = sub.brushes[i];
			if (!brush) continue;

			D2D1_POINT_2F start = brush->GetStartPoint();
			D2D1_POINT_2F end = brush->GetEndPoint();

			start.x += sign * point->x;
			start.y += sign * point->y;
			end.x += sign * point->x;
			end.y += sign * point->y;

			brush->SetStartPoint(start);
			brush->SetEndPoint(end);

			if (beforeDrawing)
			{
				layout->SetDrawingEffect(brush.Get(), sub.innerRanges[i]);
			}
		}
	}
}

D2D1_VECTOR_4F ToVector4F(const D2D1_COLOR_F& color)
{
	return D2D1::Vector4F(color.r, color.g, color.b, color.a);
}

void DrawInlineShadow(const InlineSetting::Shadow& setting, const std::vector<DWRITE_TEXT_RANGE>& ranges,
	InlineShadowCache& cache, ID2D1DeviceContext* target, IDWriteTextLayout* layout,
	ID2D1SolidColorBrush* solidBrush, const UINT32 strLen, const D2D1_RECT_F& drawRect)
{
	if (!target || !layout) return;

	// In order to make a shadow effect using the built-in D2D effect, we first need to make
	// certain parts of the string transparent. We then draw only the parts of the string we
	// we want a shadow for onto a memory bitmap. From this bitmap we can create the shadow
	// effect and draw it.

	const D2D1_COLOR_F& color = Util::c_Transparent_Color_F;

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> transparent;
	HRESULT hr = target->CreateSolidColorBrush(color, transparent.GetAddressOf());
	if (FAILED(hr)) return;

	// Only change characters outside of the range(s) transparent
	for (UINT32 i = 0; i < strLen; ++i)
	{
		bool found = false;
		for (const auto& range : ranges)
		{
			if (range.length <= 0) continue;

			if (i >= range.startPosition && i < (range.startPosition + range.length))
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			DWRITE_TEXT_RANGE temp = { i, 1 };
			layout->SetDrawingEffect(transparent.Get(), temp);
		}
	}

	const D2D1_POINT_2F drawPosition = D2D1::Point2F(drawRect.left, drawRect.top);
	const D2D1_SIZE_F drawSize = D2D1::SizeF(drawRect.right, drawRect.bottom);
	FLOAT dpiX = 0.0f, dpiY = 0.0f;
	target->GetDpi(&dpiX, &dpiY);

	// Reset the shadow bitmap if the drawing position, size, or DPI of target has changed.
	if (cache.bitmapTarget && (
		drawRect.left != cache.previousPosition.left ||
		drawRect.top != cache.previousPosition.top ||
		drawRect.right != cache.previousPosition.right ||
		drawRect.bottom != cache.previousPosition.bottom))
	{
		cache.bitmapTarget.Reset();
	}
	else if (cache.bitmapTarget)
	{
		FLOAT bitmapDpiX = 0.0f, bitmapDpiY = 0.0f;
		cache.bitmapTarget->GetDpi(&bitmapDpiX, &bitmapDpiY);
		if (dpiX != bitmapDpiX || dpiY != bitmapDpiY)
		{
			cache.bitmapTarget.Reset();
		}
	}

	cache.bitmap.Reset();

	if (!cache.bitmapTarget)
	{
		hr = target->CreateCompatibleRenderTarget(drawSize, cache.bitmapTarget.GetAddressOf());
		if (FAILED(hr)) return;
		cache.previousPosition = drawRect;
	}

	// Draw onto memory bitmap target
	// Note: Hardware acceleration seems to keep the bitmap render target in memory
	// even though it is cleared, so manually "Clear" with a transparent color.
	cache.bitmapTarget->BeginDraw();
	cache.bitmapTarget->Clear(color);
	cache.bitmapTarget->DrawTextLayout(D2D1::Point2F(0.0f, 0.0f), layout, solidBrush);
	cache.bitmapTarget->EndDraw();

	hr = cache.bitmapTarget->GetBitmap(cache.bitmap.GetAddressOf());
	if (FAILED(hr)) return;

	// Create shadow effect
	Microsoft::WRL::ComPtr<ID2D1Effect> shadow;
	hr = target->CreateEffect(CLSID_D2D1Shadow, shadow.GetAddressOf());
	if (FAILED(hr)) return;

	// Load shadow options to effect
	shadow->SetInput(0, cache.bitmap.Get());
	shadow->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, setting.blur);
	shadow->SetValue(D2D1_SHADOW_PROP_COLOR, ToVector4F(setting.color));
	shadow->SetValue(D2D1_SHADOW_PROP_OPTIMIZATION, D2D1_SHADOW_OPTIMIZATION_SPEED);

	// Draw effect
	target->DrawImage(shadow.Get(), Util::AddPoint2F(drawPosition, setting.offset));
}

void ApplyCharacterSpacing(IDWriteTextLayout* layout, const std::vector<DWRITE_TEXT_RANGE>& ranges,
	const InlineSetting::CharacterSpacing& setting)
{
	ForEachRange(ranges, [&](const DWRITE_TEXT_RANGE& range)
	{
		Microsoft::WRL::ComPtr<IDWriteTextLayout1> textLayout1;
		HRESULT hr = layout->QueryInterface(__uuidof(IDWriteTextLayout1), &textLayout1);
		if (FAILED(hr)) return;

		// Whatever the option leaves out stays at the value the layout already has.
		FLOAT leading = FLT_MAX, trailing = FLT_MAX, advanceWidth = -1.0f;
		hr = textLayout1->GetCharacterSpacing(range.startPosition, &leading, &trailing, &advanceWidth);
		if (FAILED(hr)) return;

		if (setting.leading != FLT_MAX) leading = setting.leading;
		if (setting.trailing != FLT_MAX) trailing = setting.trailing;
		if (setting.advanceWidth >= 0.0f) advanceWidth = setting.advanceWidth;

		textLayout1->SetCharacterSpacing(leading * (4.0f / 3.0f), trailing * (4.0f / 3.0f),
			advanceWidth * (4.0f / 3.0f), range);
	});
}

void ApplyColor(ID2D1DeviceContext* target, IDWriteTextLayout* layout,
	const std::vector<DWRITE_TEXT_RANGE>& ranges, const InlineSetting::Color& setting)
{
	if (!target || !layout) return;

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> solidBrush;
	HRESULT hr = target->CreateSolidColorBrush(setting.color, solidBrush.GetAddressOf());
	if (FAILED(hr)) return;

	ForEachRange(ranges, [&](const DWRITE_TEXT_RANGE& range)
	{
		layout->SetDrawingEffect(solidBrush.Get(), range);
	});
}

void ApplyCase(std::wstring& str, const std::vector<DWRITE_TEXT_RANGE>& ranges,
	const InlineSetting::Case& setting)
{
	for (const auto& range : ranges)
	{
		// The ranges were found in the string as it was when the inline options were last updated,
		// which is not necessarily the string being formatted here.
		if (range.startPosition >= str.length()) continue;

		const size_t count = min((size_t)range.length, str.length() - range.startPosition);
		if (count == 0) continue;

		WCHAR* text = &str[range.startPosition];
		switch (setting.type)
		{
		case CaseType::Lower: StringUtil::ToLowerCase(text, count); break;
		case CaseType::Upper: StringUtil::ToUpperCase(text, count); break;
		case CaseType::Proper: StringUtil::ToProperCase(text, count); break;
		case CaseType::Sentence: StringUtil::ToSentenceCase(text, count); break;
		}
	}
}

}  // namespace

TextFormat::TextFormat() :
	m_HorizontalAlignment(HorizontalAlignment::Left),
	m_VerticalAlignment(VerticalAlignment::Top),
	m_FontWeight(-1),
	m_HasWeightChanged(false),
	m_ExtraHeight(),
	m_LineGap(),
	m_Trimming(),
	m_WordWrap(),
	m_HasInlineOptionsChanged(false),
	m_FontCollection()
{
}

TextFormat::~TextFormat()
{
	m_InlineOptions.clear();
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

	// The brushes and bitmaps were built on the device that is going away.
	for (auto& state : m_InlineOptions)
	{
		state.cache = {};
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
		for (auto& state : m_InlineOptions)
		{
			if (const auto* setting = std::get_if<InlineSetting::GradientColor>(&state.option.setting))
			{
				BuildInlineGradientBrushes(*setting, state.ranges, GetCache<InlineGradientCache>(state),
					target, m_TextLayout.Get());
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

		// The 'Face' option needs access to the font collection.
		m_FontCollection = fontCollection;
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

float TextFormat::GetLineGapAdjustment(std::wstring_view str) const
{
	return str.find(L'\n') == std::wstring_view::npos ? m_LineGap : 0.0f;
}

DWRITE_TEXT_METRICS TextFormat::GetMetrics(std::wstring_view str, bool gdiEmulation, float maxWidth)
{
	const float lineGap = GetLineGapAdjustment(str);

	// GDI+ compatibility: If the last character is a newline, GDI+ measurements seem to ignore it.
	if (str.length() > 2 && str.back() == L'\n')
	{
		str.remove_suffix(1);
		if (str.back() == L'\r') str.remove_suffix(1);
	}

	const DWRITE_TEXT_RANGE range = { 0, (UINT32)str.length() };
	DWRITE_TEXT_METRICS metrics = { 0 };
	Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
	HRESULT hr = Canvas::c_DWFactory->CreateTextLayout(
		str.data(),
		range.length,
		m_TextFormat.Get(),
		maxWidth,
		10000.0f,
		textLayout.GetAddressOf());
	if (SUCCEEDED(hr))
	{
		// Set the font weight if valid
		if (m_FontWeight > 0 && m_FontWeight < 1000)
		{
			textLayout->SetFontWeight((DWRITE_FONT_WEIGHT)m_FontWeight, range);
		}

		ApplyInlineFormatting(textLayout.Get());

		const float xOffset = m_TextFormat->GetFontSize() / 6.0f;
		if (gdiEmulation)
		{
			Microsoft::WRL::ComPtr<IDWriteTextLayout1> textLayout1;
			textLayout.As(&textLayout1);

			const float emOffset = xOffset / 24.0f;
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
				metrics.height += m_LineGap - lineGap;
			}
			else
			{
				// Make sure the fractional part of the width/height metrics are included
				// in the calculation.
				metrics.width = std::ceil(metrics.width);
				metrics.height = std::ceil(metrics.height);

				// GDI+ compatibility: With accurate metrics, the line gap needs to be subtracted
				// from the overall height if the string does not contain newlines.
				metrics.height -= lineGap;
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

void TextFormat::SetTrimming(bool trim, bool wrap)
{
	m_Trimming = trim;
	m_WordWrap = wrap;

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
	const bool wrap = m_WordWrap || m_HorizontalAlignment == HorizontalAlignment::Justify;
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
	const bool isSame = m_InlineOptions.size() == options.size() &&
		std::equal(options.begin(), options.end(), m_InlineOptions.begin(),
			[](const TextInlineOption& option, const TextInlineOptionState& current)
			{
				return option == current.option;
			});

	if (isSame) return;

	// The device resources were built from the options that are being replaced, so they go with
	// them. They are built again when the layout the options changed is recreated.
	m_InlineOptions.clear();
	m_InlineOptions.reserve(options.size());
	for (const auto& option : options)
	{
		m_InlineOptions.push_back({ option });
	}

	m_HasInlineOptionsChanged = true;
}

void TextFormat::SetInlineRanges(const std::vector<std::vector<TextInlineRange>>& ranges)
{
	const size_t count = min(m_InlineOptions.size(), ranges.size());
	for (size_t i = 0; i < count; ++i)
	{
		auto& state = m_InlineOptions[i];
		state.ranges.clear();
		state.ranges.reserve(ranges[i].size());

		for (const auto& range : ranges[i])
		{
			state.ranges.push_back({ range.start, range.length });
		}
	}
}

void TextFormat::ApplyInlineFace(IDWriteTextLayout* layout, const std::vector<DWRITE_TEXT_RANGE>& ranges,
	const InlineSetting::Face& setting) const
{
	ForEachRange(ranges, [&](const DWRITE_TEXT_RANGE& range)
	{
		// Search for the font family name in font collection. Since the font collection might not
		// have been built yet, build it. If the font is not in the font collection, assume it is
		// available to the system.
		if (m_FontCollection && m_FontCollection->InitializeCollection())
		{
			UINT32 index = UINT_MAX;
			BOOL exists = FALSE;
			HRESULT hr = m_FontCollection->m_Collection->FindFamilyName(setting.face.c_str(), &index, &exists);
			if (SUCCEEDED(hr))
			{
				// Use the custom font collection (LocalFont, @Resources\Fonts) when it has the font,
				// and the system collection when it does not.
				layout->SetFontCollection(exists ?
					m_FontCollection->m_Collection : m_FontCollection->c_SystemCollection.Get(), range);
			}
		}

		layout->SetFontFamilyName(setting.face.c_str(), range);
	});
}

void TextFormat::ApplyInlineTypography(IDWriteTextLayout* layout, const std::vector<DWRITE_TEXT_RANGE>& ranges,
	const InlineSetting::Typography& setting) const
{
	ForEachRange(ranges, [&](const DWRITE_TEXT_RANGE& range)
	{
		Microsoft::WRL::ComPtr<IDWriteTypography> typography;
		HRESULT hr = Canvas::c_DWFactory->CreateTypography(typography.GetAddressOf());
		if (FAILED(hr)) return;

		DWRITE_FONT_FEATURE feature = { setting.tag, setting.parameter };
		hr = typography->AddFontFeature(feature);
		if (FAILED(hr)) return;

		layout->SetTypography(typography.Get(), range);
	});
}

void TextFormat::ApplyInlineFormatting(IDWriteTextLayout* layout)
{
	if (!layout) return;

	for (const auto& state : m_InlineOptions)
	{
		const auto& ranges = state.ranges;
		std::visit(Overloaded{
			[&](const InlineSetting::CharacterSpacing& s) { ApplyCharacterSpacing(layout, ranges, s); },
			[&](const InlineSetting::Face& s) { ApplyInlineFace(layout, ranges, s); },
			[&](const InlineSetting::Italic&)
			{
				ForEachRange(ranges, [&](const DWRITE_TEXT_RANGE& r) { layout->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, r); });
			},
			[&](const InlineSetting::Oblique&)
			{
				ForEachRange(ranges, [&](const DWRITE_TEXT_RANGE& r) { layout->SetFontStyle(DWRITE_FONT_STYLE_OBLIQUE, r); });
			},
			[&](const InlineSetting::Size& s)
			{
				FLOAT size = s.size * (4.0f / 3.0f);
				if (size <= 0.0f) size = 0.000001f;

				ForEachRange(ranges, [&](const DWRITE_TEXT_RANGE& r) { layout->SetFontSize(size, r); });
			},
			[&](const InlineSetting::Stretch& s)
			{
				ForEachRange(ranges, [&](const DWRITE_TEXT_RANGE& r) { layout->SetFontStretch(s.stretch, r); });
			},
			[&](const InlineSetting::Strikethrough&)
			{
				ForEachRange(ranges, [&](const DWRITE_TEXT_RANGE& r) { layout->SetStrikethrough(TRUE, r); });
			},
			[&](const InlineSetting::Typography& s) { ApplyInlineTypography(layout, ranges, s); },
			[&](const InlineSetting::Underline&)
			{
				ForEachRange(ranges, [&](const DWRITE_TEXT_RANGE& r) { layout->SetUnderline(TRUE, r); });
			},
			[&](const InlineSetting::Weight& s)
			{
				ForEachRange(ranges, [&](const DWRITE_TEXT_RANGE& r) { layout->SetFontWeight(s.weight, r); });
			},

			// These are applied when the text is drawn, or to the text itself, rather than to the
			// layout - see ApplyInlineColoring(), ApplyInlineShadow() and ApplyInlineCase().
			[](const InlineSetting::Case&) {},
			[](const InlineSetting::Color&) {},
			[](const InlineSetting::GradientColor&) {},
			[](const InlineSetting::Shadow&) {},

			// Draws the text as it would be without any option at all.
			[](const InlineSetting::None&) {}
		}, state.option.setting);
	}
}

void TextFormat::ApplyInlineColoring(ID2D1DeviceContext* target, const D2D1_POINT_2F* point)
{
	for (auto& state : m_InlineOptions)
	{
		if (const auto* setting = std::get_if<InlineSetting::Color>(&state.option.setting))
		{
			ApplyColor(target, m_TextLayout.Get(), state.ranges, *setting);
		}
		else if (std::holds_alternative<InlineSetting::GradientColor>(state.option.setting))
		{
			ApplyInlineGradient(GetCache<InlineGradientCache>(state), m_TextLayout.Get(), point);
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

	for (const auto& state : m_InlineOptions)
	{
		if (const auto* setting = std::get_if<InlineSetting::Case>(&state.option.setting))
		{
			if (str != &buffer)
			{
				buffer.assign(srcStr);
				str = &buffer;
			}

			ApplyCase(buffer, state.ranges, *setting);
		}
	}

	return *str;
}

void TextFormat::ApplyInlineShadow(ID2D1DeviceContext* target, ID2D1SolidColorBrush* solidBrush,
	const UINT32 strLen, const D2D1_RECT_F& drawRect)
{
	for (auto& state : m_InlineOptions)
	{
		const auto* setting = std::get_if<InlineSetting::Shadow>(&state.option.setting);
		if (!setting) continue;

		DrawInlineShadow(*setting, state.ranges, GetCache<InlineShadowCache>(state), target,
			m_TextLayout.Get(), solidBrush, strLen, drawRect);

		// We need to reset the color options after the shadow effect because the shadow effect
		// can turn some characters invisible.
		const D2D1_POINT_2F drawPosition = D2D1::Point2F(drawRect.left, drawRect.top);

		ResetInlineColoring(solidBrush, strLen);
		ResetGradientPosition(&drawPosition);
		ApplyInlineColoring(target, &drawPosition);
	}
}

void TextFormat::ResetGradientPosition(const D2D1_POINT_2F* point)
{
	for (auto& state : m_InlineOptions)
	{
		if (std::holds_alternative<InlineSetting::GradientColor>(state.option.setting))
		{
			ApplyInlineGradient(GetCache<InlineGradientCache>(state), nullptr, point, false);
		}
	}
}

void TextFormat::ResetInlineColoring(ID2D1SolidColorBrush* solidColor, const UINT32 strLen)
{
	DWRITE_TEXT_RANGE range = { 0, strLen };
	m_TextLayout->SetDrawingEffect(solidColor, range);
}

}  // namespace Gfx
