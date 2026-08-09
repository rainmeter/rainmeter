// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "TextInlineFormatCase.h"
#include "StringUtil.h"

namespace Gfx {

TextInlineFormat_Case::TextInlineFormat_Case(const std::wstring& pattern, const CaseType type) :
	TextInlineFormat(pattern),
	m_Type(type)
{
}

TextInlineFormat_Case::~TextInlineFormat_Case()
{
}

void TextInlineFormat_Case::ApplyInlineFormat(std::wstring& str)
{
	for (const auto& range : GetRanges())
	{
		// The ranges were found in the string as it was when the inline options were last updated,
		// which is not necessarily the string being formatted here.
		if (range.startPosition >= str.length()) continue;

		const size_t count = min((size_t)range.length, str.length() - range.startPosition);
		if (count == 0) continue;

		WCHAR* text = &str[range.startPosition];
		switch (m_Type)
		{
		case CaseType::Lower: StringUtil::ToLowerCase(text, count); break;
		case CaseType::Upper: StringUtil::ToUpperCase(text, count); break;
		case CaseType::Proper: StringUtil::ToProperCase(text, count); break;
		case CaseType::Sentence: StringUtil::ToSentenceCase(text, count); break;
		}
	}
}

bool TextInlineFormat_Case::CompareAndUpdateProperties(const std::wstring& pattern, const CaseType type)
{
	if (_wcsicmp(GetPattern().c_str(), pattern.c_str()) != 0 || m_Type != type)
	{
		SetPattern(pattern);
		m_Type = type;
		return true;
	}

	return false;
}

}  // namespace Gfx
