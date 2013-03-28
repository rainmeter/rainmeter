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
#include <GdiPlus.h>

namespace Gfx {

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

class __declspec(novtable) TextFormat
{
public:
	virtual ~TextFormat();

	virtual bool IsInitialized() const = 0;
	virtual void SetProperties(const WCHAR* fontFamily, int size, bool bold, bool italic, Gdiplus::PrivateFontCollection* fontCollection) = 0;

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