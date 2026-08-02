// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../FontCollection.h"
#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the font face for a select range.
class TextInlineFormat_Face final : public TextInlineFormat
{
public:
	TextInlineFormat_Face(const std::wstring& pattern, const std::wstring& face);
	virtual ~TextInlineFormat_Face();
	virtual InlineType GetType() override { return InlineType::Face; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	void SetFontCollection(FontCollection* fontCollection) { m_FontCollection = fontCollection; }

	bool CompareAndUpdateProperties(const std::wstring& pattern, const std::wstring& face);

private:
	TextInlineFormat_Face();
	TextInlineFormat_Face(const TextInlineFormat_Face& other) = delete;

	std::wstring m_Face;

	FontCollection* m_FontCollection;
};

}  // namespace Gfx
