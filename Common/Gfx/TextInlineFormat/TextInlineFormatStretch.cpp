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
		if (range.length > 0)
		{
			layout->SetFontStretch(m_Stretch, range);
		}
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
