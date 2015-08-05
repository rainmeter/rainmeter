/*
  Copyright (C) 2015 Brian Ferguson

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "TextInlineFormatWeight.h"

namespace Gfx {

TextInlineFormat_Weight::TextInlineFormat_Weight(const std::wstring pattern, const DWRITE_FONT_WEIGHT weight) :
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
		if (range.length > 0)
		{
			layout->SetFontWeight(m_Weight, range);
		}
	}
}

bool TextInlineFormat_Weight::CompareAndUpdateProperties(const std::wstring pattern, const DWRITE_FONT_WEIGHT weight)
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
