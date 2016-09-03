/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_TEXTINLINEFORMAT_WEIGHT_H_
#define RM_GFX_TEXTINLINEFORMAT_WEIGHT_H_

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// Sets the weight property for a select range.
class TextInlineFormat_Weight final : public TextInlineFormat
{
public:
	TextInlineFormat_Weight(const std::wstring& pattern, const DWRITE_FONT_WEIGHT& weight);
	virtual ~TextInlineFormat_Weight();
	virtual InlineType GetType() override { return InlineType::Weight; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring& pattern, const DWRITE_FONT_WEIGHT& weight);

private:
	TextInlineFormat_Weight();
	TextInlineFormat_Weight(const TextInlineFormat_Weight& other) = delete;

	DWRITE_FONT_WEIGHT m_Weight;
};

}  // namespace Gfx

#endif
