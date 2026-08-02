// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "TextInlineFormatColor.h"
#include "Gfx/Util/D2DUtil.h"

namespace Gfx {

TextInlineFormat_Color::TextInlineFormat_Color(const std::wstring& pattern, const D2D1_COLOR_F& color) :
	TextInlineFormat(pattern),
	m_Color(color)
{
}

TextInlineFormat_Color::~TextInlineFormat_Color()
{
}

void TextInlineFormat_Color::ApplyInlineFormat(ID2D1DeviceContext* target, IDWriteTextLayout* layout)
{
	if (!target || !layout) return;

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> solidBrush;
	HRESULT hr = target->CreateSolidColorBrush(m_Color, solidBrush.GetAddressOf());
	if (FAILED(hr)) return;

	for (const auto& range : GetRanges())
	{
		if (range.length <= 0) continue;

		layout->SetDrawingEffect(solidBrush.Get(), range);
	}
}

bool TextInlineFormat_Color::CompareAndUpdateProperties(const std::wstring& pattern, const D2D1_COLOR_F& color)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0 || !Util::ColorFEquals(m_Color, color))
	{
		SetPattern(pattern);
		m_Color = color;
		return true;
	}

	return false;
}

}  // namespace Gfx
