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

#ifndef RM_GFX_TEXTINLINEFORMAT_TYPOGRAPHY_H_
#define RM_GFX_TEXTINLINEFORMAT_TYPOGRAPHY_H_

#include "../TextInlineFormat.h"
#include <Windows.h>

// DirectWrite supports at least 80 different typographic settings for open type fonts,
// using a 4-byte identifier for the registered name of the feature, which we store in
// |m_Tag|. Some tags use an additional parameter which we store in |m_Parameter|. In most
// cases, |m_Parameter| should be '1' to indicate 'on'. '0' would represent 'off'.
// Example: |m_Tag| = 'salt' or Stylistic Alternatives, |m_Parameter| would represent the
//          index to the list of alternate substituting glyphs.
// Note: |m_Tag| is not validated, but fails silently if not supported or incorrect.

namespace Gfx {

// Sets the font typography property for a select range.
class TextInlineFormat_Typography final : public TextInlineFormat
{
public:
	TextInlineFormat_Typography(const std::wstring pattern, const DWRITE_FONT_FEATURE_TAG tag, const UINT32 parameter);
	virtual ~TextInlineFormat_Typography();
	virtual InlineType GetType() override { return InlineType::Typography; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring pattern, const DWRITE_FONT_FEATURE_TAG tag, const UINT32 parameter);

private:
	TextInlineFormat_Typography();
	TextInlineFormat_Typography(const TextInlineFormat_Typography& other) = delete;

	DWRITE_FONT_FEATURE_TAG m_Tag;
	UINT32 m_Parameter;
};

}  // namespace Gfx

#endif