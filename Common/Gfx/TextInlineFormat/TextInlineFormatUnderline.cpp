/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "TextInlineFormatUnderline.h"

namespace Gfx {

TextInlineFormat_Underline::TextInlineFormat_Underline(const std::wstring& pattern) :
	TextInlineFormat(pattern)
{
}

TextInlineFormat_Underline::~TextInlineFormat_Underline()
{
}

void TextInlineFormat_Underline::ApplyInlineFormat(IDWriteTextLayout* layout)
{
	if (!layout) return;

	for (const auto& range : GetRanges())
	{
		if (range.length <= 0) continue;

		layout->SetUnderline(TRUE, range);
	}
}

bool TextInlineFormat_Underline::CompareAndUpdateProperties(const std::wstring& pattern)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0)
	{
		SetPattern(pattern);
		return true;
	}

	return false;
}

}  // namespace Gfx
