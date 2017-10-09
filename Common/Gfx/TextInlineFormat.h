/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_TEXTINLINEFORMAT_H_
#define RM_GFX_TEXTINLINEFORMAT_H_

#include <string>
#include <vector>
#include <dwrite_1.h>
#include <d2d1_1.h>
#include <ole2.h>  // For Gdiplus.h.
#include <GdiPlus.h>
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

#endif
