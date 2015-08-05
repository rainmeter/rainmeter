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
#include "TextInlineFormatColor.h"

namespace {

D2D1_COLOR_F ToColorF(const Gdiplus::Color& color)
{
	return D2D1::ColorF(color.GetR() / 255.0f, color.GetG() / 255.0f, color.GetB() / 255.0f, color.GetA() / 255.0f);
}

}  // namespace

namespace Gfx {

TextInlineFormat_Color::TextInlineFormat_Color(const std::wstring& pattern, const Gdiplus::Color& color) :
	TextInlineFormat(pattern),
	m_Color(color)
{
}

TextInlineFormat_Color::~TextInlineFormat_Color()
{
}

void TextInlineFormat_Color::ApplyInlineFormat(ID2D1RenderTarget* target, IDWriteTextLayout* layout)
{
	if (!target || !layout) return;

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> solidBrush;
	HRESULT hr = target->CreateSolidColorBrush(ToColorF(m_Color), solidBrush.GetAddressOf());
	if (SUCCEEDED(hr))
	{
		for (const auto& range : GetRanges())
		{
			if (range.length > 0)
			{
				layout->SetDrawingEffect(solidBrush.Get(), range);
			}
		}
	}
}

bool TextInlineFormat_Color::CompareAndUpdateProperties(const std::wstring& pattern, const Gdiplus::Color& color)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0 || m_Color.GetValue() != color.GetValue())
	{
		SetPattern(pattern);
		m_Color = color;
		return true;
	}

	return false;
}

}  // namespace Gfx
