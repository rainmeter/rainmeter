/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "TextInlineFormatStretch.h"

namespace Gfx {

TextInlineFormat_Stretch::TextInlineFormat_Stretch(const std::wstring& pattern, const DWRITE_FONT_STRETCH& stretch) :
	TextInlineFormat(pattern),
	m_Stretch(stretch)
{
}

TextInlineFormat_Stretch::~TextInlineFormat_Stretch()
{
}

void TextInlineFormat_Stretch::ApplyInlineFormat(IDWriteTextLayout* layout)
{
	if (!layout) return;

	for (const auto& range : GetRanges())
	{
		if (range.length <= 0) continue;

		layout->SetFontStretch(m_Stretch, range);
	}
}

bool TextInlineFormat_Stretch::CompareAndUpdateProperties(const std::wstring& pattern, const DWRITE_FONT_STRETCH& stretch)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0 || m_Stretch != stretch)
	{
		SetPattern(pattern);
		m_Stretch = stretch;
		return true;
	}

	return false;
}

}  // namespace Gfx
