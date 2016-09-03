/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "TextInlineFormatSize.h"

namespace Gfx {

TextInlineFormat_Size::TextInlineFormat_Size(const std::wstring& pattern, FLOAT size) :
	TextInlineFormat(pattern),
	m_Size(size)
{
}

TextInlineFormat_Size::~TextInlineFormat_Size()
{
}

void TextInlineFormat_Size::ApplyInlineFormat(IDWriteTextLayout* layout)
{
	if (!layout) return;

	for (const auto& range : GetRanges())
	{
		if (range.length <= 0) continue;

		layout->SetFontSize(m_Size * (4.0f / 3.0f), range);
	}
}

bool TextInlineFormat_Size::CompareAndUpdateProperties(const std::wstring& pattern, FLOAT size)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0 || m_Size != size)
	{
		SetPattern(pattern);
		m_Size = size;
		return true;
	}

	return false;
}

}  // namespace Gfx
