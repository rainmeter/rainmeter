// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the font size property for a select range.
class TextInlineFormat_Size final : public TextInlineFormat
{
public:
	TextInlineFormat_Size(const std::wstring& pattern, FLOAT size);
	virtual ~TextInlineFormat_Size();
	virtual InlineType GetType() override { return InlineType::Size; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring& pattern, FLOAT size);

private:
	TextInlineFormat_Size();
	TextInlineFormat_Size(const TextInlineFormat_Size& other) = delete;

	FLOAT m_Size;
};

}  // namespace Gfx
