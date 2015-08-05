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

#ifndef RM_GFX_TEXTINLINEFORMAT_COLOR_H_
#define RM_GFX_TEXTINLINEFORMAT_COLOR_H_

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets a color for a select range.
class TextInlineFormat_Color final : public TextInlineFormat
{
public:
	TextInlineFormat_Color(const std::wstring& pattern, const Gdiplus::Color& color);
	virtual ~TextInlineFormat_Color();
	virtual InlineType GetType() override { return InlineType::Color; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override { }
	void ApplyInlineFormat(ID2D1RenderTarget* target, IDWriteTextLayout* layout);

	bool CompareAndUpdateProperties(const std::wstring& pattern, const Gdiplus::Color& color);

private:
	TextInlineFormat_Color();
	TextInlineFormat_Color(const TextInlineFormat_Color& other) = delete;

	Gdiplus::Color m_Color;
};

}  // namespace Gfx

#endif