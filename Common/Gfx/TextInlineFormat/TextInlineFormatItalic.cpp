// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "TextInlineFormatItalic.h"

namespace Gfx {

TextInlineFormat_Italic::TextInlineFormat_Italic(const std::wstring& pattern) :
	TextInlineFormat(pattern)
{
}

TextInlineFormat_Italic::~TextInlineFormat_Italic()
{
}

void TextInlineFormat_Italic::ApplyInlineFormat(IDWriteTextLayout* layout)
{
	if (!layout) return;

	for (const auto& range : GetRanges())
	{
		if (range.length <= 0) continue;

		layout->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, range);
	}
}

bool TextInlineFormat_Italic::CompareAndUpdateProperties(const std::wstring& pattern)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0)
	{
		SetPattern(pattern);
		return true;
	}

	return false;
}

}  // namespace Gfx
