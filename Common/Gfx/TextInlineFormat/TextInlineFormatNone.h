/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_TEXTINLINEFORMAT_NONE_H_
#define RM_GFX_TEXTINLINEFORMAT_NONE_H_

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

// This serves as a placeholder to allow for dynamically turning off an InlineSetting
class TextInlineFormat_None final : public TextInlineFormat
{
public:
	TextInlineFormat_None(const std::wstring& pattern);
	virtual ~TextInlineFormat_None();
	virtual InlineType GetType() override { return InlineType::None; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override;

	bool CompareAndUpdateProperties(const std::wstring& pattern);

private:
	TextInlineFormat_None();
	TextInlineFormat_None(const TextInlineFormat_None& other) = delete;
};

}  // namespace Gfx

#endif
