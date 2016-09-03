/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "TextInlineFormatWeight.h"

namespace Gfx {

TextInlineFormat_Weight::TextInlineFormat_Weight(const std::wstring& pattern, const DWRITE_FONT_WEIGHT& weight) :
	TextInlineFormat(pattern),
	m_Weight(weight)
{
}

TextInlineFormat_Weight::~TextInlineFormat_Weight()
{
}

void TextInlineFormat_Weight::ApplyInlineFormat(IDWriteTextLayout* layout)
{
	if (!layout) return;

	for (const auto& range : GetRanges())
	{
		if (range.length <= 0) continue;

		layout->SetFontWeight(m_Weight, range);
	}
}

bool TextInlineFormat_Weight::CompareAndUpdateProperties(const std::wstring& pattern, const DWRITE_FONT_WEIGHT& weight)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0 || m_Weight != weight)
	{
		SetPattern(pattern);
		m_Weight = weight;
		return true;
	}

	return false;
}

}  // namespace Gfx
