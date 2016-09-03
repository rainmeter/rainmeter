/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_TEXTINLINEFORMAT_CASE_H_
#define RM_GFX_TEXTINLINEFORMAT_CASE_H_

#include "../TextInlineFormat.h"
#include <Windows.h>

namespace Gfx {

enum class CaseType : BYTE
{
	Lower,
	Upper,
	Proper,
	Sentence,
	None
};

// Sets the case for a select range.
class TextInlineFormat_Case final : public TextInlineFormat
{
public:
	TextInlineFormat_Case(const std::wstring& pattern, const CaseType type);
	virtual ~TextInlineFormat_Case();
	virtual InlineType GetType() override { return InlineType::Case; }

	virtual void ApplyInlineFormat(IDWriteTextLayout* layout) override { }
	void ApplyInlineFormat(std::wstring& str);

	bool CompareAndUpdateProperties(const std::wstring& pattern, const CaseType type);

private:
	TextInlineFormat_Case();
	TextInlineFormat_Case(const TextInlineFormat_Case& other) = delete;

	CaseType m_Type;
};

}  // namespace Gfx

#endif
