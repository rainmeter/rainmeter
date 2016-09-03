/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_TEXTINLINEFORMAT_CHARACTERSPACING_H_
#define RM_GFX_TEXTINLINEFORMAT_CHARACTERSPACING_H_

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the spacing values for each character for a select range.
class TextInlineFormat_CharacterSpacing final : public TextInlineFormat
{
public:
	TextInlineFormat_CharacterSpacing(const std::wstring& pattern, FLOAT leading, FLOAT trailing, FLOAT advanceWidth);
	virtual ~TextInlineFormat_CharacterSpacing();
	virtual InlineType GetType() override { return InlineType::CharacterSpacing; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring& pattern, FLOAT leading,
		FLOAT trailing, FLOAT advanceWidth);

private:
	TextInlineFormat_CharacterSpacing();
	TextInlineFormat_CharacterSpacing(const TextInlineFormat_CharacterSpacing& other) = delete;

	FLOAT m_Leading;
	FLOAT m_Trailing;
	FLOAT m_AdvanceWidth;
};

}  // namespace Gfx

#endif
