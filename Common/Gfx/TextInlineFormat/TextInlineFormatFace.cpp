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
#include "TextInlineFormatFace.h"

namespace Gfx {

TextInlineFormat_Face::TextInlineFormat_Face(const std::wstring& pattern, const std::wstring& face) :
	TextInlineFormat(pattern),
	m_Face(face),
	m_FontCollection(nullptr)
{
}

TextInlineFormat_Face::~TextInlineFormat_Face()
{
}

void TextInlineFormat_Face::ApplyInlineFormat(IDWriteTextLayout* layout)
{
	if (!layout) return;

	for (const auto& range : GetRanges())
	{
		if (range.length > 0)
		{
			// Search for the font family name in font collection. Since the
			// font collection might not have been built yet, build it. If the
			// font is not in the font collection, assume it is available to
			// the system.
			if (m_FontCollection && m_FontCollection->InitializeCollection())
			{
				UINT32 index = UINT_MAX;
				BOOL exists = FALSE;
				HRESULT hr = m_FontCollection->m_Collection->FindFamilyName(m_Face.c_str(), &index, &exists);

				if (SUCCEEDED(hr) && exists)
				{
					layout->SetFontCollection(m_FontCollection->m_Collection, range);
				}
			}

			layout->SetFontFamilyName(m_Face.c_str(), range);
		}
	}
}

bool TextInlineFormat_Face::CompareAndUpdateProperties(const std::wstring& pattern, const std::wstring& face)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0 || _wcsicmp(m_Face.c_str(), face.c_str()) != 0)
	{
		SetPattern(pattern);
		m_Face = face;
		return true;
	}

	return false;
}

}  // namespace Gfx
