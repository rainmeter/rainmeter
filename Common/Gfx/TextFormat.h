// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "TextInlineOption.h"
#include <Windows.h>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <dwrite_1.h>
#include <wrl/client.h>

namespace Gfx {

class FontCollection;

// The brushes a gradient option draws with. Since a range can be spread across several lines, it is
// split into the inner ranges the layout puts it on, each with a brush of its own.
struct InlineGradientCache
{
	struct Sub
	{
		std::vector<DWRITE_TEXT_RANGE> innerRanges;
		std::vector<Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush>> brushes;
	};

	// One entry per range the option applies to.
	std::vector<Sub> subs;
};

// The bitmap a shadow option is built from. It is kept between draws, and only recreated when the
// position or the DPI it was drawn at changes.
struct InlineShadowCache
{
	Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
	Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> bitmapTarget;
	D2D1_RECT_F previousPosition = D2D1::RectF(-1.0f, -1.0f, -1.0f, -1.0f);
};

// An inline option as it is used: the option itself, the ranges of the text it applies to, and the
// device resources built from it, for the two options that have any.
struct TextInlineOptionState
{
	TextInlineOption option;
	std::vector<DWRITE_TEXT_RANGE> ranges;
	std::variant<std::monostate, InlineGradientCache, InlineShadowCache> cache;
};

enum class HorizontalAlignment : BYTE
{
	Left,
	Center,
	Right,
	Justify
};

enum class VerticalAlignment : BYTE
{
	Top,
	Center,
	Bottom
};

// Provides text formatting through DirectWrite.
class TextFormat final
{
public:
	TextFormat();
	~TextFormat();

	TextFormat(const TextFormat& other) = delete;
	TextFormat& operator=(TextFormat other) = delete;

	bool IsInitialized() const { return m_TextFormat != nullptr; }
	void InvalidateDeviceResources();

	void SetProperties(
		const WCHAR* fontFamily, FLOAT size, bool bold, bool italic,
		FontCollection* fontCollection);

	void SetFontWeight(int weight);

	// |true| when the font this format resolved to can draw |ch| itself. Says nothing about what
	// DirectWrite would fall back to for it, which is the point: a caller choosing between
	// characters it could draw wants the one that does not send the run to another font.
	bool HasCharacter(UINT32 ch) const;

	// |trim| replaces the text that does not fit with an ellipsis, and |wrap| breaks the lines to
	// the layout width rather than letting them run past it. Trimmed text is always wrapped, but
	// wrapped text need not be trimmed: an editable meter wraps to its box and scrolls to the text
	// that does not fit, rather than cutting it short with an ellipsis it could not scroll past.
	void SetTrimming(bool trim, bool wrap);

	void SetHorizontalAlignment(HorizontalAlignment alignment);
	HorizontalAlignment GetHorizontalAlignment() const { return m_HorizontalAlignment; }

	void SetVerticalAlignment(VerticalAlignment alignment);
	VerticalAlignment GetVerticalAlignment() const { return m_VerticalAlignment; }

	// Return |true| when the options have changed.
	bool SetInlineOptions(const std::vector<TextInlineOption>& options);
	void SetInlineRanges(const std::vector<std::vector<TextInlineRange>>& ranges);

	size_t GetInlineOptionCount() const { return m_InlineOptions.size(); }
	const std::wstring& GetInlinePattern(const size_t index) const { return m_InlineOptions[index].option.pattern; }

private:
	friend class Canvas;

	friend class Common_Gfx_TextFormat_Test;

	void Dispose();

	Microsoft::WRL::ComPtr<IDWriteFont> ResolveFont() const;

	// Enables word wrapping if the last SetTrimming() or justified alignment requires it.
	void UpdateWordWrapping();

	// Creates a new DirectWrite text layout if |str| has changed since last call. Since creating
	// the layout is costly, it is more efficient to keep reusing the text layout until the text
	// changes. Returns true if the layout is valid for use.
	bool CreateLayout(ID2D1DeviceContext* target, const std::wstring& srcStr, float maxW, float maxH, bool gdiEmulation);

	DWRITE_TEXT_METRICS GetMetrics(std::wstring_view str, bool gdiEmulation, float maxWidth = 10000.0f);

	// DirectWrite puts the font's line gap in every line it reports, and GetMetrics() takes it back
	// off a single-line string for GDI+ compatibility. Anything drawn to match the height of the
	// text - the caret, or the highlight behind a selection - has to take it off as well.
	float GetLineGapAdjustment(std::wstring_view str) const;

	// Applies the options that are part of the layout. The rest are drawn (color, gradient and
	// shadow) or change the text itself (case), and are applied through the functions below.
	void ApplyInlineFormatting(IDWriteTextLayout* layout);

	// These two reach for a font collection and the DirectWrite factory, which is why they are not
	// free functions like the rest of the layout options.
	void ApplyInlineFace(IDWriteTextLayout* layout, const std::vector<DWRITE_TEXT_RANGE>& ranges,
		const InlineSetting::Face& setting) const;
	void ApplyInlineTypography(IDWriteTextLayout* layout, const std::vector<DWRITE_TEXT_RANGE>& ranges,
		const InlineSetting::Typography& setting) const;
	void ApplyInlineColoring(ID2D1DeviceContext* target, const D2D1_POINT_2F* point);

	// Returns |srcStr| as-is unless an inline case option applies, in which case the transformed
	// text is built in |buffer| and that is returned instead. The returned reference is only valid
	// for as long as both |srcStr| and |buffer| are.
	const std::wstring& ApplyInlineCase(const std::wstring& srcStr, std::wstring& buffer);

	void ApplyInlineShadow(ID2D1DeviceContext* target, ID2D1SolidColorBrush* solidBrush,
		const UINT32 strLen, const D2D1_RECT_F& drawRect);
	void ResetGradientPosition(const D2D1_POINT_2F* point);
	void ResetInlineColoring(ID2D1SolidColorBrush* solidColor, const UINT32 strLen);

	HorizontalAlignment m_HorizontalAlignment;
	VerticalAlignment m_VerticalAlignment;

	Microsoft::WRL::ComPtr<IDWriteTextFormat> m_TextFormat;
	Microsoft::WRL::ComPtr<IDWriteFont> m_Font;

	Microsoft::WRL::ComPtr<IDWriteTextLayout> m_TextLayout;
	Microsoft::WRL::ComPtr<IDWriteInlineObject> m_InlineEllipsis;

	std::wstring m_LastString;

	int m_FontWeight;
	bool m_HasWeightChanged;

	// Used to emulate GDI+ behaviour.
	float m_ExtraHeight;
	float m_LineGap;

	// Contain the values passed to the last call of SetTrimming().
	bool m_Trimming;
	bool m_WordWrap;

	// Contains all the inline options for the layout.
	std::vector<TextInlineOptionState> m_InlineOptions;
	bool m_HasInlineOptionsChanged;

	// Only used by the 'Face' option, to look a font family up before falling back to the system
	// collection. Owned by the skin, and set with the rest of the properties.
	FontCollection* m_FontCollection;
};

}  // namespace Gfx
