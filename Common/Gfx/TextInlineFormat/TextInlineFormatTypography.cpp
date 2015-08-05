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
#include "TextInlineFormatTypography.h"
#include "../CanvasD2D.h"

namespace Gfx {

TextInlineFormat_Typography::TextInlineFormat_Typography(
	const std::wstring pattern, const DWRITE_FONT_FEATURE_TAG tag, const UINT32 parameter) :
		TextInlineFormat(pattern),
		m_Tag(tag),
		m_Parameter(parameter)
{
}

TextInlineFormat_Typography::~TextInlineFormat_Typography()
{
}

void TextInlineFormat_Typography::ApplyInlineFormat(IDWriteTextLayout* layout)
{
	if (!layout) return;

	for (const auto& range : GetRanges())
	{
		if (range.length > 0)
		{
			Microsoft::WRL::ComPtr<IDWriteTypography> typography;
			HRESULT hr = CanvasD2D::c_DWFactory->CreateTypography(typography.GetAddressOf());
			if (SUCCEEDED(hr))
			{
				DWRITE_FONT_FEATURE feature = { m_Tag, m_Parameter };
				hr = typography->AddFontFeature(feature);
				if (SUCCEEDED(hr))
				{
					hr = layout->SetTypography(typography.Get(), range);
				}
			}
		}
	}
}

bool TextInlineFormat_Typography::CompareAndUpdateProperties(
	const std::wstring pattern, const DWRITE_FONT_FEATURE_TAG tag, const UINT32 parameter)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0 || m_Tag != tag || m_Parameter != parameter)
	{
		SetPattern(pattern);
		m_Tag = tag;
		m_Parameter = parameter;
		return true;
	}

	return false;
}

}  // namespace Gfx
