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
	std::wstring formatStr;

	for (const auto& range : GetRanges())
	{
		if (range.length <= 0) continue;

		formatStr = str.substr(range.startPosition, range.length);
		switch (m_Type)
		{
		case CaseType::Lower: StringUtil::ToLowerCase(formatStr); break;
		case CaseType::Upper: StringUtil::ToUpperCase(formatStr); break;
		case CaseType::Proper: StringUtil::ToProperCase(formatStr); break;
		case CaseType::Sentence: StringUtil::ToSentenceCase(formatStr); break;
		}

		str.replace((size_t)range.startPosition, (size_t)range.length, formatStr);
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
