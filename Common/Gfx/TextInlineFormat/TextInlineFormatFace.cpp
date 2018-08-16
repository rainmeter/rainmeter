/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "TextInlineFormatFace.h"
#include "../Canvas.h"

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
		if (range.length <= 0) continue;

		// Search for the font family name in font collection. Since the
		// font collection might not have been built yet, build it. If the
		// font is not in the font collection, assume it is available to
		// the system.
		if (m_FontCollection && m_FontCollection->InitializeCollection())
		{
			UINT32 index = UINT_MAX;
			BOOL exists = FALSE;
			HRESULT hr = m_FontCollection->m_Collection->FindFamilyName(m_Face.c_str(), &index, &exists);
			if (SUCCEEDED(hr))
			{
				if (exists)
				{
					// Use the custom font collection (LocalFont, @Resources\Fonts)
					layout->SetFontCollection(m_FontCollection->m_Collection, range);
				}
				else
				{
					// Use the system font collection
					layout->SetFontCollection(m_FontCollection->c_SystemCollection.Get(), range);
				}
			}
		}

		layout->SetFontFamilyName(m_Face.c_str(), range);
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
