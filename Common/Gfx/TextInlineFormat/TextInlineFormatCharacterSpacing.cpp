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
#include "TextInlineFormatCharacterSpacing.h"

namespace Gfx {

TextInlineFormat_CharacterSpacing::TextInlineFormat_CharacterSpacing(const std::wstring& pattern,
	FLOAT leading, FLOAT trailing, FLOAT advanceWidth) :
		TextInlineFormat(pattern),
		m_Leading(leading),
		m_Trailing(trailing),
		m_AdvanceWidth(advanceWidth)
{
}

TextInlineFormat_CharacterSpacing::~TextInlineFormat_CharacterSpacing()
{
}

void TextInlineFormat_CharacterSpacing::ApplyInlineFormat(IDWriteTextLayout* layout)
{
	if (!layout) return;

	for (const auto& range : GetRanges())
	{
		if (range.length > 0)
		{
			Microsoft::WRL::ComPtr<IDWriteTextLayout1> textLayout1;
			HRESULT hr = layout->QueryInterface(__uuidof(IDWriteTextLayout1), &textLayout1);
			if (SUCCEEDED(hr))
			{
				// Query current values
				FLOAT leading = FLT_MAX, trailing = FLT_MAX, advanceWidth = -1.0f;
				hr = textLayout1->GetCharacterSpacing(range.startPosition, &leading, &trailing, &advanceWidth);
				if (SUCCEEDED(hr))
				{
					if (m_Leading == FLT_MAX) m_Leading = leading;
					if (m_Trailing == FLT_MAX) m_Trailing = trailing;
					if (m_AdvanceWidth < 0.0f) m_AdvanceWidth = advanceWidth;

					textLayout1->SetCharacterSpacing(m_Leading * (4.0f / 3.0f), m_Trailing * (4.0f / 3.0f),
						m_AdvanceWidth * (4.0f / 3.0f), range);
				}
			}
		}
	}
}

bool TextInlineFormat_CharacterSpacing::CompareAndUpdateProperties(const std::wstring& pattern, FLOAT leading,
	FLOAT trailing, FLOAT advanceWidth)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0 || m_Leading != leading ||
		m_Trailing != trailing || m_AdvanceWidth != advanceWidth)
	{
		SetPattern(pattern);
		m_Leading = leading;
		m_Trailing = trailing;
		m_AdvanceWidth = advanceWidth;
		return true;
	}

	return false;
}

}  // namespace Gfx
