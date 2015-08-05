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

#ifndef RM_GFX_TEXTINLINEFORMAT_FACE_H_
#define RM_GFX_TEXTINLINEFORMAT_FACE_H_

#include "../FontCollectionD2D.h"
#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the font face for a select range.
class TextInlineFormat_Face final : public TextInlineFormat
{
public:
	TextInlineFormat_Face(const std::wstring pattern, const std::wstring face);
	virtual ~TextInlineFormat_Face();
	virtual InlineType GetType() override { return InlineType::Face; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	void SetFontCollection(FontCollectionD2D* fontCollection) { m_FontCollection = fontCollection; }

	bool CompareAndUpdateProperties(const std::wstring pattern, const std::wstring face);

private:
	TextInlineFormat_Face();
	TextInlineFormat_Face(const TextInlineFormat_Face& other) = delete;

	std::wstring m_Face;

	FontCollectionD2D* m_FontCollection;
};

}  // namespace Gfx

#endif