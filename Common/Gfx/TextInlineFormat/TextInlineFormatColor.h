// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets a color for a select range.
class TextInlineFormat_Color final : public TextInlineFormat
{
public:
	TextInlineFormat_Color(const std::wstring& pattern, const D2D1_COLOR_F& color);
	virtual ~TextInlineFormat_Color();
	virtual InlineType GetType() override { return InlineType::Color; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override { }
	void ApplyInlineFormat(ID2D1DeviceContext* target, IDWriteTextLayout* layout);

	bool CompareAndUpdateProperties(const std::wstring& pattern, const D2D1_COLOR_F& color);

private:
	TextInlineFormat_Color();
	TextInlineFormat_Color(const TextInlineFormat_Color& other) = delete;

	D2D1_COLOR_F m_Color;
};

}  // namespace Gfx
