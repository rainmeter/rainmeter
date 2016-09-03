/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_TEXTINLINEFORMAT_UNDERLINE_H_
#define RM_GFX_TEXTINLINEFORMAT_UNDERLINE_H_

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the underline property for a select range.
class TextInlineFormat_Underline final : public TextInlineFormat
{
public:
	TextInlineFormat_Underline(const std::wstring& pattern);
	virtual ~TextInlineFormat_Underline();
	virtual InlineType GetType() override { return InlineType::Underline; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring& pattern);

private:
	TextInlineFormat_Underline();
	TextInlineFormat_Underline(const TextInlineFormat_Underline& other) = delete;
};

}  // namespace Gfx

#endif
