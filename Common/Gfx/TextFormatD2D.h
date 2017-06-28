/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_TEXTFORMATD2D_H_
#define RM_GFX_TEXTFORMATD2D_H_

#include "TextInlineFormat.h"
#include "TextFormat.h"
#include <memory>
#include <string>
#include <dwrite_1.h>
#include <wrl/client.h>

namespace Gfx {

enum class CaseType : BYTE;

// Provides a Direct2D/DirectWrite implementation of TextFormat for use with CanvasD2D.
class TextFormatD2D : public TextFormat
{
public:
	TextFormatD2D();
	virtual ~TextFormatD2D();

	TextFormatD2D(const TextFormatD2D& other) = delete;
	TextFormatD2D& operator=(TextFormatD2D other) = delete;

	virtual bool IsInitialized() const override { return m_TextFormat != nullptr; }

	virtual void SetProperties(
		const WCHAR* fontFamily, int size, bool bold, bool italic,
		const FontCollection* fontCollection) override;

	virtual void SetFontWeight(int weight) override;

	virtual void SetTrimming(bool trim) override;

	virtual void SetHorizontalAlignment(HorizontalAlignment alignment) override;
	virtual void SetVerticalAlignment(VerticalAlignment alignment) override;

	virtual void ReadInlineOptions(ConfigParser& parser, const WCHAR* section) override;
	virtual void FindInlineRanges(const std::wstring& str) override;

private:
	friend class Canvas;

	friend class Common_Gfx_TextFormatD2D_Test;

	void Dispose();

	// Creates a new DirectWrite text layout if |str| has changed since last call. Since creating
	// the layout is costly, it is more efficient to keep reusing the text layout until the text
	// changes. Returns true if the layout is valid for use.
	bool CreateLayout(ID2D1RenderTarget* target, const std::wstring& srcStr, float maxW, float maxH, bool gdiEmulation);

	DWRITE_TEXT_METRICS GetMetrics(const std::wstring& srcStr, bool gdiEmulation, float maxWidth = 10000.0f);

	// These functions create/modify any inline options.
	bool CreateInlineOption(const size_t index, const std::wstring pattern, std::vector<std::wstring> options);
	void UpdateInlineCase(const size_t& index, const std::wstring pattern, const Gfx::CaseType type);
	void UpdateInlineCharacterSpacing(const size_t& index, const std::wstring pattern, const FLOAT leading,
		const FLOAT trailing, const FLOAT advanceWidth);
	void UpdateInlineColor(const size_t& index, const std::wstring pattern, const Gdiplus::Color color);
	void UpdateInlineFace(const size_t& index, const std::wstring pattern, const WCHAR* face);
	void UpdateInlineGradientColor(const size_t& index, const std::wstring pattern,
		const std::vector<std::wstring> args, const bool altGamma);
	void UpdateInlineItalic(const size_t& index, const std::wstring pattern);
	void UpdateInlineOblique(const size_t& index, const std::wstring pattern);
	void UpdateInlineShadow(const size_t& index, const std::wstring pattern, const FLOAT blur,
		const D2D1_POINT_2F offset, const Gdiplus::Color color);
	void UpdateInlineSize(const size_t& index, const std::wstring pattern, const FLOAT size);
	void UpdateInlineStretch(const size_t& index, const std::wstring pattern, const DWRITE_FONT_STRETCH stretch);
	void UpdateInlineStrikethrough(const size_t& index, const std::wstring pattern);
	void UpdateInlineTypography(const size_t& index, const std::wstring pattern,
		const DWRITE_FONT_FEATURE_TAG tag, const UINT32 parameter);
	void UpdateInlineUnderline(const size_t& index, const std::wstring pattern);
	void UpdateInlineWeight(const size_t& index, const std::wstring pattern, const DWRITE_FONT_WEIGHT weight);
	void ApplyInlineFormatting(IDWriteTextLayout* layout);
	void ApplyInlineColoring(ID2D1RenderTarget* target, const D2D1_POINT_2F* point);
	void ApplyInlineCase(std::wstring& str);
	void ApplyInlineShadow(ID2D1RenderTarget* target, ID2D1SolidColorBrush* solidBrush,
		const UINT32 strLen, const D2D1_POINT_2F& drawPosition);
	void ResetGradientPosition(const D2D1_POINT_2F* point);
	void ResetInlineColoring(ID2D1SolidColorBrush* solidColor, const UINT32 strLen);

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

#endif
