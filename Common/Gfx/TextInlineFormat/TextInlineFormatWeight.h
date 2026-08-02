// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the weight property for a select range.
class TextInlineFormat_Weight final : public TextInlineFormat
{
public:
	TextInlineFormat_Weight(const std::wstring& pattern, const DWRITE_FONT_WEIGHT& weight);
	virtual ~TextInlineFormat_Weight();
	virtual InlineType GetType() override { return InlineType::Weight; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring& pattern, const DWRITE_FONT_WEIGHT& weight);

private:
	TextInlineFormat_Weight();
	TextInlineFormat_Weight(const TextInlineFormat_Weight& other) = delete;

	DWRITE_FONT_WEIGHT m_Weight;
};

}  // namespace Gfx
