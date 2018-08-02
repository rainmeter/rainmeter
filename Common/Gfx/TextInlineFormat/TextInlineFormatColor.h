/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_TEXTINLINEFORMAT_COLOR_H_
#define RM_GFX_TEXTINLINEFORMAT_COLOR_H_

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets a color for a select range.
class TextInlineFormat_Color final : public TextInlineFormat
{
public:
	TextInlineFormat_Color(const std::wstring& pattern, const D2D1_COLOR_F& color);
	virtual ~TextInlineFormat_Color();
	virtual InlineType GetType() override { return InlineType::Color; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override { }
	void ApplyInlineFormat(ID2D1DeviceContext* target, IDWriteTextLayout* layout);

	bool CompareAndUpdateProperties(const std::wstring& pattern, const D2D1_COLOR_F& color);

private:
	TextInlineFormat_Color();
	TextInlineFormat_Color(const TextInlineFormat_Color& other) = delete;

	D2D1_COLOR_F m_Color;
};

}  // namespace Gfx

#endif
