/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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
