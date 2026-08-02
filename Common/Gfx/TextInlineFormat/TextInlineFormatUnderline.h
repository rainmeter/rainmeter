// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the underline property for a select range.
class TextInlineFormat_Underline final : public TextInlineFormat
{
public:
	TextInlineFormat_Underline(const std::wstring& pattern);
	virtual ~TextInlineFormat_Underline();
	virtual InlineType GetType() override { return InlineType::Underline; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring& pattern);

private:
	TextInlineFormat_Underline();
	TextInlineFormat_Underline(const TextInlineFormat_Underline& other) = delete;
};

}  // namespace Gfx
