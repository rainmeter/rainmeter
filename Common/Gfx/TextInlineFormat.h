/*
  Copyright (C) 2015 Brian Ferguson

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

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
	CharacterSpacing,
	Color,
	Face,
	GradientColor,
	Italic,
	Kerning,
	Oblique,
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