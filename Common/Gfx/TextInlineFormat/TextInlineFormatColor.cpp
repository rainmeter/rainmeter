/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "TextInlineFormatColor.h"
#include "Gfx/Util/D2DUtil.h"

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
	HRESULT hr = target->CreateSolidColorBrush(Util::ToColorF(m_Color), solidBrush.GetAddressOf());
	if (FAILED(hr)) return;

	for (const auto& range : GetRanges())
	{
		if (range.length <= 0) continue;

		layout->SetDrawingEffect(solidBrush.Get(), range);
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
