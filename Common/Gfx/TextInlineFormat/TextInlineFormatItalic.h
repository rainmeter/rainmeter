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

#ifndef RM_GFX_TEXTINLINEFORMAT_ITALIC_H_
#define RM_GFX_TEXTINLINEFORMAT_ITALIC_H_

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the italic property for a select range.
class TextInlineFormat_Italic final : public TextInlineFormat
{
public:
	TextInlineFormat_Italic(const std::wstring pattern);
	virtual ~TextInlineFormat_Italic();
	virtual InlineType GetType() override { return InlineType::Italic; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring pattern);

private:
	TextInlineFormat_Italic();
	TextInlineFormat_Italic(const TextInlineFormat_Italic& other) = delete;
};

}  // namespace Gfx

#endif