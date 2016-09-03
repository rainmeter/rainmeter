/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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
	TextInlineFormat_Face(const std::wstring& pattern, const std::wstring& face);
	virtual ~TextInlineFormat_Face();
	virtual InlineType GetType() override { return InlineType::Face; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	void SetFontCollection(FontCollectionD2D* fontCollection) { m_FontCollection = fontCollection; }

	bool CompareAndUpdateProperties(const std::wstring& pattern, const std::wstring& face);

private:
	TextInlineFormat_Face();
	TextInlineFormat_Face(const TextInlineFormat_Face& other) = delete;

	std::wstring m_Face;

	FontCollectionD2D* m_FontCollection;
};

}  // namespace Gfx

#endif
