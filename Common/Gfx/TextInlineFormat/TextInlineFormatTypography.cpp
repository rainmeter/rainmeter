/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "TextInlineFormatTypography.h"
#include "../Canvas.h"

namespace Gfx {

TextInlineFormat_Typography::TextInlineFormat_Typography(const std::wstring& pattern,
	const DWRITE_FONT_FEATURE_TAG& tag, UINT32 parameter) :
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
		if (range.length <= 0) continue;

		Microsoft::WRL::ComPtr<IDWriteTypography> typography;
		HRESULT hr = Canvas::c_DWFactory->CreateTypography(typography.GetAddressOf());
		if (FAILED(hr)) continue;

		DWRITE_FONT_FEATURE feature = { m_Tag, m_Parameter };
		hr = typography->AddFontFeature(feature);
		if (FAILED(hr)) continue;

		layout->SetTypography(typography.Get(), range);
	}
}

bool TextInlineFormat_Typography::CompareAndUpdateProperties(
	const std::wstring& pattern, const DWRITE_FONT_FEATURE_TAG& tag, UINT32 parameter)
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
