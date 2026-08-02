// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the font stretch property for a select range.
class TextInlineFormat_Stretch final : public TextInlineFormat
{
public:
	TextInlineFormat_Stretch(const std::wstring& pattern, const DWRITE_FONT_STRETCH& stretch);
	virtual ~TextInlineFormat_Stretch();
	virtual InlineType GetType() override { return InlineType::Stretch; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring& pattern, const DWRITE_FONT_STRETCH& stretch);

private:
	TextInlineFormat_Stretch();
	TextInlineFormat_Stretch(const TextInlineFormat_Stretch& other) = delete;

	DWRITE_FONT_STRETCH m_Stretch;
};

}  // namespace Gfx
