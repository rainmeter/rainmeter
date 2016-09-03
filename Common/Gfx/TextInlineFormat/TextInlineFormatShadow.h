/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_TEXTINLINEFORMAT_SHADOW_H_
#define RM_GFX_TEXTINLINEFORMAT_SHADOW_H_

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets a color for a select range.
class TextInlineFormat_Shadow final : public TextInlineFormat
{
public:
	TextInlineFormat_Shadow(const std::wstring& pattern, const FLOAT& blur,
		const D2D1_POINT_2F& offset, const Gdiplus::Color& color);
	virtual ~TextInlineFormat_Shadow();
	virtual InlineType GetType() override { return InlineType::Shadow; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override { }
	void ApplyInlineFormat(ID2D1RenderTarget* target, IDWriteTextLayout* layout,
		ID2D1SolidColorBrush* solidBrush, const UINT32& strLen, const D2D1_POINT_2F& drawPosition);

	bool CompareAndUpdateProperties(const std::wstring& pattern, const FLOAT& blur,
		const D2D1_POINT_2F& offset, const Gdiplus::Color& color);

private:
	TextInlineFormat_Shadow();
	TextInlineFormat_Shadow(const TextInlineFormat_Shadow& other) = delete;

	FLOAT m_Blur;
	D2D1_POINT_2F m_Offset;
	Gdiplus::Color m_Color;
};

}  // namespace Gfx

#endif
