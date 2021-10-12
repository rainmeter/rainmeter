/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_TEXTFORMAT_H_
#define RM_GFX_TEXTFORMAT_H_

#include "../../Library/ConfigParser.h"
#include <Windows.h>

namespace Gfx {

class FontCollection;

enum class HorizontalAlignment : BYTE
{
	Left,
	Center,
	Right
};

enum class VerticalAlignment : BYTE
{
	Top,
	Center,
	Bottom
};

// Represents the logical font properties used to format text.
class __declspec(novtable) TextFormat
{
public:
	virtual ~TextFormat();

	TextFormat(const TextFormat& other) = delete;

	// Returns true if this TextFormat object is valid for use in draw operations.
	virtual bool IsInitialized() const = 0;

	// Sets the logical properties of the font to use. If the font is not found in the system font
	// collection, the given |fontCollection| is also searched. |fontCollection| may be nullptr.
	virtual void SetProperties(
		const WCHAR* fontFamily, int size, bool bold, bool italic,
		const FontCollection* fontCollection) = 0;

	// Sets the font weight of the font used. |weight| should be between 1-999.
	virtual void SetFontWeight(int weight) = 0;

	// Sets the trimming and wrapping of the text. If |trim| is true, subsequent draws using this
	// TextFormat object will produce clipped text with an ellipsis if the text overflows the
	// bounding rectangle. 
	virtual void SetTrimming(bool trim) = 0;

	virtual void SetHorizontalAlignment(HorizontalAlignment alignment);
	HorizontalAlignment GetHorizontalAlignment() const { return m_HorizontalAlignment; }

	virtual void SetVerticalAlignment(VerticalAlignment alignment);
	VerticalAlignment GetVerticalAlignment() const { return m_VerticalAlignment; }

	// Reads any inline options for the string meter. This is only available with D2D.
	virtual void ReadInlineOptions(ConfigParser& parser, const WCHAR* section) = 0;
	virtual void FindInlineRanges(const std::wstring& str) = 0;

protected:
	TextFormat();

private:
	HorizontalAlignment m_HorizontalAlignment;
	VerticalAlignment m_VerticalAlignment;
};

}  // namespace Gfx

#endif
