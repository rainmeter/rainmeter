// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// This serves as a placeholder to allow for dynamically turning off an InlineSetting
class TextInlineFormat_None final : public TextInlineFormat
{
public:
	TextInlineFormat_None(const std::wstring& pattern);
	virtual ~TextInlineFormat_None();
	virtual InlineType GetType() override { return InlineType::None; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring& pattern);

private:
	TextInlineFormat_None();
	TextInlineFormat_None(const TextInlineFormat_None& other) = delete;
};

}  // namespace Gfx
