/*
  Copyright (C) 2013 Birunthan Mohanathas

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

#ifndef RM_GFX_TEXTFORMAT_H_
#define RM_GFX_TEXTFORMAT_H_

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

	// Returns true if this TextFormat object is valid for use in draw operations.
	virtual bool IsInitialized() const = 0;

	// Sets the logical properties of the font to use. If the font is not found in the system font
	// collection, the given |fontCollection| is also searched. |fontCollection| may be nullptr.
	virtual void SetProperties(
		const WCHAR* fontFamily, int size, bool bold, bool italic,
		const FontCollection* fontCollection) = 0;

	// Sets the trimming and wrapping of the text. If |trim| is true, subsequent draws using this
	// TextFormat object will produce clipped text with an ellipsis if the text overflows the
	// bounding rectangle. 
	virtual void SetTrimming(bool trim) = 0;

	virtual void SetHorizontalAlignment(HorizontalAlignment alignment);
	HorizontalAlignment GetHorizontalAlignment() const { return m_HorizontalAlignment; }

	virtual void SetVerticalAlignment(VerticalAlignment alignment);
	VerticalAlignment GetVerticalAlignment() const { return m_VerticalAlignment; }

protected:
	TextFormat();

private:
	TextFormat(const TextFormat& other) {}

	HorizontalAlignment m_HorizontalAlignment;
	VerticalAlignment m_VerticalAlignment;
};

}  // namespace Gfx

#endif