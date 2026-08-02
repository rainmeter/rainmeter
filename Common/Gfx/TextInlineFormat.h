// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <string>
#include <vector>
#include <dwrite_1.h>
#include <d2d1_1.h>
#include <Windows.h>

namespace Gfx {

enum class InlineType : BYTE
{
	Case,
	CharacterSpacing,
	Color,
	Face,
	GradientColor,
	Italic,
	None,
	Oblique,
	Shadow,
	Size,
	Stretch,
	Strikethrough,
	Typography,
	Underline,
	Weight
};

class __declspec(novtable) TextInlineFormat
{
public:
	virtual ~TextInlineFormat();
	virtual InlineType GetType() = 0;
	virtual void InvalidateDeviceResources() {}

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) = 0;

	const std::wstring& GetPattern() { return m_Pattern; }
	void SetRanges(std::vector<DWRITE_TEXT_RANGE> ranges) { m_TextRange = ranges; }

protected:
	TextInlineFormat(std::wstring pattern);
	TextInlineFormat(const TextInlineFormat& other) = delete;

	void SetPattern(const std::wstring pattern) { m_Pattern = pattern; }
	const std::vector<DWRITE_TEXT_RANGE>& GetRanges() { return m_TextRange; }

private:
	std::wstring m_Pattern;
	std::vector<DWRITE_TEXT_RANGE> m_TextRange;
};

}  // namespace Gfx
