// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the font oblique property for a select range.
class TextInlineFormat_Oblique final : public TextInlineFormat
{
public:
	TextInlineFormat_Oblique(const std::wstring& pattern);
	virtual ~TextInlineFormat_Oblique();
	virtual InlineType GetType() override { return InlineType::Oblique; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring& pattern);

private:
	TextInlineFormat_Oblique();
	TextInlineFormat_Oblique(const TextInlineFormat_Oblique& other) = delete;
};

}  // namespace Gfx
