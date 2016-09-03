/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_TEXTINLINEFORMAT_OBLIQUE_H_
#define RM_GFX_TEXTINLINEFORMAT_OBLIQUE_H_

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the font oblique property for a select range.
class TextInlineFormat_Oblique final : public TextInlineFormat
{
public:
	TextInlineFormat_Oblique(const std::wstring& pattern);
	virtual ~TextInlineFormat_Oblique();
	virtual InlineType GetType() override { return InlineType::Oblique; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring& pattern);

private:
	TextInlineFormat_Oblique();
	TextInlineFormat_Oblique(const TextInlineFormat_Oblique& other) = delete;
};

}  // namespace Gfx

#endif
