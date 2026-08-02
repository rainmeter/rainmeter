// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "TextInlineFormat.h"

namespace Gfx {

TextInlineFormat::TextInlineFormat(std::wstring pattern) :
	m_Pattern(std::move(pattern))
{
}

TextInlineFormat::~TextInlineFormat()
{
}

}  // namespace Gfx
