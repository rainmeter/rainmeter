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

#ifndef RM_GFX_TEXTFORMATD2D_H_
#define RM_GFX_TEXTFORMATD2D_H_

#include "TextInlineFormat.h"
#include "TextFormat.h"
#include <string>
#include <dwrite_1.h>
#include <wrl/client.h>

namespace Gfx {

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

	virtual void SetTrimming(bool trim) override;

	virtual void SetHorizontalAlignment(HorizontalAlignment alignment) override;
	virtual void SetVerticalAlignment(VerticalAlignment alignment) override;

	virtual void ReadInlineOptions(ConfigParser& parser, const WCHAR* section) override;
	virtual void FindInlineRanges(const std::wstring& str) override;

private:
	friend class CanvasD2D;

	friend class Common_Gfx_TextFormatD2D_Test;

	void Dispose();

	// Creates a new DirectWrite text layout if |str| has changed since last call. Since creating
	// the layout is costly, it is more efficient to keep reusing the text layout until the text
	// changes. Returns true if the layout is valid for use.
	bool CreateLayout(ID2D1RenderTarget* target, const WCHAR* str, UINT strLen, float maxW, float maxH, bool gdiEmulation);

	DWRITE_TEXT_METRICS GetMetrics(
		const WCHAR* str, UINT strLen, bool gdiEmulation, float maxWidth = 10000.0f);

	// These functions create/modify any inline options.
	bool CreateInlineOption(const size_t index, const std::wstring pattern, std::vector<std::wstring> options);
	void UpdateInlineCharacterSpacing(const size_t& index, const std::wstring pattern, const FLOAT leading,
		const FLOAT trailing, const FLOAT advanceWidth);
	void UpdateInlineColor(const size_t& index, const std::wstring pattern, const Gdiplus::Color color);
	void UpdateInlineFace(const size_t& index, const std::wstring pattern, const WCHAR* face);
	void UpdateInlineGradientColor(const size_t& index, const std::wstring pattern,
		const std::vector<std::wstring> args, const bool altGamma);
	void UpdateInlineItalic(const size_t& index, const std::wstring pattern);
	void UpdateInlineOblique(const size_t& index, const std::wstring pattern);
	void UpdateInlineSize(const size_t& index, const std::wstring pattern, const FLOAT size);
	void UpdateInlineStretch(const size_t& index, const std::wstring pattern, const DWRITE_FONT_STRETCH stretch);
	void UpdateInlineStrikethrough(const size_t& index, const std::wstring pattern);
	void UpdateInlineTypography(const size_t& index, const std::wstring pattern,
		const DWRITE_FONT_FEATURE_TAG tag, const UINT32 parameter);
	void UpdateInlineUnderline(const size_t& index, const std::wstring pattern);
	void UpdateInlineWeight(const size_t& index, const std::wstring pattern, const DWRITE_FONT_WEIGHT weight);
	void ApplyInlineFormatting(IDWriteTextLayout* layout);
	void ApplyInlineColoring(ID2D1RenderTarget* target, const D2D1_POINT_2F* point);
	void ResetInlineColoring(ID2D1SolidColorBrush* solidColor, const UINT strLen);

	Microsoft::WRL::ComPtr<IDWriteTextFormat> m_TextFormat;
	Microsoft::WRL::ComPtr<IDWriteTextLayout> m_TextLayout;
	Microsoft::WRL::ComPtr<IDWriteInlineObject> m_InlineEllipsis;

	std::wstring m_LastString;

	// Used to emulate GDI+ behaviour.
	float m_ExtraHeight;
	float m_LineGap;

	// Contains the value passed to the last call of SetTrimming().
	bool m_Trimming;

	// Contains all the inline options for the layout.
	std::vector<TextInlineFormat*> m_TextInlineFormat;
	bool m_HasInlineOptionsChanged;
};

}  // namespace Gfx

#endif