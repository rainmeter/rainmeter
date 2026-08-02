// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "TextInlineFormat.h"
#include <Windows.h>
#include <memory>
#include <string>
#include <vector>
#include <dwrite_1.h>
#include <wrl/client.h>

class MathParser;

namespace Gfx {

enum class CaseType : BYTE;
class FontCollection;

struct TextInlineOption
{
	std::wstring pattern;
	std::vector<std::wstring> settings;
};

struct TextInlineRange
{
	UINT32 start;
	UINT32 length;
};

enum class HorizontalAlignment : BYTE
{
	Left,
	Center,
	Right
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
	TextFormat(const MathParser& mathParser);
	~TextFormat();

	TextFormat(const TextFormat& other) = delete;
	TextFormat& operator=(TextFormat other) = delete;

	bool IsInitialized() const { return m_TextFormat != nullptr; }
	void InvalidateDeviceResources();

	void SetProperties(
		const WCHAR* fontFamily, FLOAT size, bool bold, bool italic,
		FontCollection* fontCollection);

	void SetFontWeight(int weight);

	void SetTrimming(bool trim);

	void SetHorizontalAlignment(HorizontalAlignment alignment);
	HorizontalAlignment GetHorizontalAlignment() const { return m_HorizontalAlignment; }

	void SetVerticalAlignment(VerticalAlignment alignment);
	VerticalAlignment GetVerticalAlignment() const { return m_VerticalAlignment; }

	void SetInlineOptions(const std::vector<TextInlineOption>& options);
	std::vector<std::wstring> GetInlinePatterns();
	void SetInlineRanges(const std::vector<std::vector<TextInlineRange>>& ranges);

	const MathParser& GetMathParser() const { return m_MathParser; }

private:
	friend class Canvas;

	friend class Common_Gfx_TextFormat_Test;

	void Dispose();

	// Creates a new DirectWrite text layout if |str| has changed since last call. Since creating
	// the layout is costly, it is more efficient to keep reusing the text layout until the text
	// changes. Returns true if the layout is valid for use.
	bool CreateLayout(ID2D1DeviceContext* target, const std::wstring& srcStr, float maxW, float maxH, bool gdiEmulation);

	DWRITE_TEXT_METRICS GetMetrics(const std::wstring& srcStr, bool gdiEmulation, float maxWidth = 10000.0f);

	// These functions create/modify any inline options.
	bool CreateInlineOption(const size_t index, const std::wstring pattern, std::vector<std::wstring> options);
	void UpdateInlineCase(const size_t& index, const std::wstring pattern, const Gfx::CaseType type);
	void UpdateInlineCharacterSpacing(const size_t& index, const std::wstring pattern, const FLOAT leading,
		const FLOAT trailing, const FLOAT advanceWidth);
	void UpdateInlineColor(const size_t& index, const std::wstring pattern, const D2D1_COLOR_F& color);
	void UpdateInlineFace(const size_t& index, const std::wstring pattern, const WCHAR* face);
	void UpdateInlineGradientColor(const size_t& index, const std::wstring pattern,
		const std::vector<std::wstring> args, const bool altGamma);
	void UpdateInlineItalic(const size_t& index, const std::wstring pattern);
	void UpdateInlineNone(const size_t& index, const std::wstring pattern);
	void UpdateInlineOblique(const size_t& index, const std::wstring pattern);
	void UpdateInlineShadow(const size_t& index, const std::wstring pattern, const FLOAT blur,
		const D2D1_POINT_2F offset, const D2D1_COLOR_F& color);
	void UpdateInlineSize(const size_t& index, const std::wstring pattern, const FLOAT size);
	void UpdateInlineStretch(const size_t& index, const std::wstring pattern, const DWRITE_FONT_STRETCH stretch);
	void UpdateInlineStrikethrough(const size_t& index, const std::wstring pattern);
	void UpdateInlineTypography(const size_t& index, const std::wstring pattern,
		const DWRITE_FONT_FEATURE_TAG tag, const UINT32 parameter);
	void UpdateInlineUnderline(const size_t& index, const std::wstring pattern);
	void UpdateInlineWeight(const size_t& index, const std::wstring pattern, const DWRITE_FONT_WEIGHT weight);
	void ApplyInlineFormatting(IDWriteTextLayout* layout);
	void ApplyInlineColoring(ID2D1DeviceContext* target, const D2D1_POINT_2F* point);
	void ApplyInlineCase(std::wstring& str);
	void ApplyInlineShadow(ID2D1DeviceContext* target, ID2D1SolidColorBrush* solidBrush,
		const UINT32 strLen, const D2D1_RECT_F& drawRect);
	void ResetGradientPosition(const D2D1_POINT_2F* point);
	void ResetInlineColoring(ID2D1SolidColorBrush* solidColor, const UINT32 strLen);

	const MathParser& m_MathParser;
	HorizontalAlignment m_HorizontalAlignment;
	VerticalAlignment m_VerticalAlignment;

	Microsoft::WRL::ComPtr<IDWriteTextFormat> m_TextFormat;
	Microsoft::WRL::ComPtr<IDWriteTextLayout> m_TextLayout;
	Microsoft::WRL::ComPtr<IDWriteInlineObject> m_InlineEllipsis;

	std::wstring m_LastString;

	int m_FontWeight;
	bool m_HasWeightChanged;

	// Used to emulate GDI+ behaviour.
	float m_ExtraHeight;
	float m_LineGap;

	// Contains the value passed to the last call of SetTrimming().
	bool m_Trimming;

	// Contains all the inline options for the layout.
	std::vector<std::unique_ptr<TextInlineFormat>> m_TextInlineFormat;
	bool m_HasInlineOptionsChanged;
};

}  // namespace Gfx
