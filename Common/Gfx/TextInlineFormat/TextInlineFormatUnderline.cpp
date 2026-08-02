// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "TextInlineFormatUnderline.h"

namespace Gfx {

TextInlineFormat_Underline::TextInlineFormat_Underline(const std::wstring& pattern) :
	TextInlineFormat(pattern)
{
}

TextInlineFormat_Underline::~TextInlineFormat_Underline()
{
}

void TextInlineFormat_Underline::ApplyInlineFormat(IDWriteTextLayout* layout)
{
	if (!layout) return;

	for (const auto& range : GetRanges())
	{
		if (range.length <= 0) continue;

		layout->SetUnderline(TRUE, range);
	}
}

bool TextInlineFormat_Underline::CompareAndUpdateProperties(const std::wstring& pattern)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0)
	{
		SetPattern(pattern);
		return true;
	}

	return false;
}

}  // namespace Gfx
