/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_TEXTINLINEFORMAT_SIZE_H_
#define RM_GFX_TEXTINLINEFORMAT_SIZE_H_

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the font size property for a select range.
class TextInlineFormat_Size final : public TextInlineFormat
{
public:
	TextInlineFormat_Size(const std::wstring& pattern, FLOAT size);
	virtual ~TextInlineFormat_Size();
	virtual InlineType GetType() override { return InlineType::Size; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring& pattern, FLOAT size);

private:
	TextInlineFormat_Size();
	TextInlineFormat_Size(const TextInlineFormat_Size& other) = delete;

	FLOAT m_Size;
};

}  // namespace Gfx

#endif
