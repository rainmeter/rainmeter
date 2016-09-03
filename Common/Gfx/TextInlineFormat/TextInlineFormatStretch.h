/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_TEXTINLINEFORMAT_STRETCH_H_
#define RM_GFX_TEXTINLINEFORMAT_STRETCH_H_

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the font stretch property for a select range.
class TextInlineFormat_Stretch final : public TextInlineFormat
{
public:
	TextInlineFormat_Stretch(const std::wstring& pattern, const DWRITE_FONT_STRETCH& stretch);
	virtual ~TextInlineFormat_Stretch();
	virtual InlineType GetType() override { return InlineType::Stretch; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring& pattern, const DWRITE_FONT_STRETCH& stretch);

private:
	TextInlineFormat_Stretch();
	TextInlineFormat_Stretch(const TextInlineFormat_Stretch& other) = delete;

	DWRITE_FONT_STRETCH m_Stretch;
};

}  // namespace Gfx

#endif
