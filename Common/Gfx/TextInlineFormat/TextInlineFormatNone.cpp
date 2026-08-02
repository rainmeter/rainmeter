// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "TextInlineFormatNone.h"

namespace Gfx {

TextInlineFormat_None::TextInlineFormat_None(const std::wstring& pattern) :
	TextInlineFormat(pattern)
{
}

TextInlineFormat_None::~TextInlineFormat_None()
{
}

void TextInlineFormat_None::ApplyInlineFormat(IDWriteTextLayout* layout)
{
}

bool TextInlineFormat_None::CompareAndUpdateProperties(const std::wstring& pattern)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0)
	{
		SetPattern(pattern);
		return true;
	}

	return false;
}

}  // namespace Gfx
