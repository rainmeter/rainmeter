// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the strikethrough property for a select range.
class TextInlineFormat_Strikethrough final : public TextInlineFormat
{
public:
	TextInlineFormat_Strikethrough(const std::wstring& pattern);
	virtual ~TextInlineFormat_Strikethrough();
	virtual InlineType GetType() override { return InlineType::Strikethrough; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring& pattern);

private:
	TextInlineFormat_Strikethrough();
	TextInlineFormat_Strikethrough(const TextInlineFormat_Strikethrough& other) = delete;
};

}  // namespace Gfx
