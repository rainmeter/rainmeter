// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "TextInlineFormatOblique.h"

namespace Gfx {

TextInlineFormat_Oblique::TextInlineFormat_Oblique(const std::wstring& pattern) :
	TextInlineFormat(pattern)
{
}

TextInlineFormat_Oblique::~TextInlineFormat_Oblique()
{
}

void TextInlineFormat_Oblique::ApplyInlineFormat(IDWriteTextLayout* layout)
{
	if (!layout) return;

	for (const auto& range : GetRanges())
	{
		if (range.length <= 0) continue;

		layout->SetFontStyle(DWRITE_FONT_STYLE_OBLIQUE, range);
	}
}

bool TextInlineFormat_Oblique::CompareAndUpdateProperties(const std::wstring& pattern)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0)
	{
		SetPattern(pattern);
		return true;
	}

	return false;
}

}  // namespace Gfx
