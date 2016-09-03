/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "TextInlineFormatOblique.h"

namespace Gfx {

TextInlineFormat_Oblique::TextInlineFormat_Oblique(const std::wstring& pattern) :
	TextInlineFormat(pattern)
{
}

TextInlineFormat_Oblique::~TextInlineFormat_Oblique()
{
}

void TextInlineFormat_Oblique::ApplyInlineFormat(IDWriteTextLayout* layout)
{
	if (!layout) return;

	for (const auto& range : GetRanges())
	{
		if (range.length <= 0) continue;

		layout->SetFontStyle(DWRITE_FONT_STYLE_OBLIQUE, range);
	}
}

bool TextInlineFormat_Oblique::CompareAndUpdateProperties(const std::wstring& pattern)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0)
	{
		SetPattern(pattern);
		return true;
	}

	return false;
}

}  // namespace Gfx
