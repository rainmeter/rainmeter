// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "TextInlineFormatStrikethrough.h"

namespace Gfx {

TextInlineFormat_Strikethrough::TextInlineFormat_Strikethrough(const std::wstring& pattern) :
	TextInlineFormat(pattern)
{
}

TextInlineFormat_Strikethrough::~TextInlineFormat_Strikethrough()
{
}

void TextInlineFormat_Strikethrough::ApplyInlineFormat(IDWriteTextLayout* layout)
{
	if (!layout) return;

	for (const auto& range : GetRanges())
	{
		if (range.length <= 0) continue;

		layout->SetStrikethrough(TRUE, range);
	}
}

bool TextInlineFormat_Strikethrough::CompareAndUpdateProperties(const std::wstring& pattern)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0)
	{
		SetPattern(pattern);
		return true;
	}

	return false;
}

}  // namespace Gfx
