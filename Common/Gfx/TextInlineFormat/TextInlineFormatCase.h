// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

enum class CaseType : BYTE
{
	Lower,
	Upper,
	Proper,
	Sentence,
	None
};

// Sets the case for a select range.
class TextInlineFormat_Case final : public TextInlineFormat
{
public:
	TextInlineFormat_Case(const std::wstring& pattern, const CaseType type);
	virtual ~TextInlineFormat_Case();
	virtual InlineType GetType() override { return InlineType::Case; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override { }
	void ApplyInlineFormat(std::wstring& str);

	bool CompareAndUpdateProperties(const std::wstring& pattern, const CaseType type);

private:
	TextInlineFormat_Case();
	TextInlineFormat_Case(const TextInlineFormat_Case& other) = delete;

	CaseType m_Type;
};

}  // namespace Gfx
