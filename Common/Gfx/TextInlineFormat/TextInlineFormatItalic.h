// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the italic property for a select range.
class TextInlineFormat_Italic final : public TextInlineFormat
{
public:
	TextInlineFormat_Italic(const std::wstring& pattern);
	virtual ~TextInlineFormat_Italic();
	virtual InlineType GetType() override { return InlineType::Italic; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring& pattern);

private:
	TextInlineFormat_Italic();
	TextInlineFormat_Italic(const TextInlineFormat_Italic& other) = delete;
};

}  // namespace Gfx
