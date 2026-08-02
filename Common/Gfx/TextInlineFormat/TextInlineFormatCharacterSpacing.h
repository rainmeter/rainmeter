// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the spacing values for each character for a select range.
class TextInlineFormat_CharacterSpacing final : public TextInlineFormat
{
public:
	TextInlineFormat_CharacterSpacing(const std::wstring& pattern, FLOAT leading, FLOAT trailing, FLOAT advanceWidth);
	virtual ~TextInlineFormat_CharacterSpacing();
	virtual InlineType GetType() override { return InlineType::CharacterSpacing; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring& pattern, FLOAT leading,
		FLOAT trailing, FLOAT advanceWidth);

private:
	TextInlineFormat_CharacterSpacing();
	TextInlineFormat_CharacterSpacing(const TextInlineFormat_CharacterSpacing& other) = delete;

	FLOAT m_Leading;
	FLOAT m_Trailing;
	FLOAT m_AdvanceWidth;
};

}  // namespace Gfx
