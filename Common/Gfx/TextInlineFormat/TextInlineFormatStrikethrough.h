/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_TEXTINLINEFORMAT_STRIKETHROUGH_H_
#define RM_GFX_TEXTINLINEFORMAT_STRIKETHROUGH_H_

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the strikethrough property for a select range.
class TextInlineFormat_Strikethrough final : public TextInlineFormat
{
public:
	TextInlineFormat_Strikethrough(const std::wstring& pattern);
	virtual ~TextInlineFormat_Strikethrough();
	virtual InlineType GetType() override { return InlineType::Strikethrough; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring& pattern);

private:
	TextInlineFormat_Strikethrough();
	TextInlineFormat_Strikethrough(const TextInlineFormat_Strikethrough& other) = delete;
};

}  // namespace Gfx

#endif
