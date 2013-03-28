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

#ifndef RM_GFX_TEXTFORMATD2D_H_
#define RM_GFX_TEXTFORMATD2D_H_

#include "TextFormat.h"
#include <dwrite.h>

namespace Gfx {

class TextFormatD2D : public TextFormat
{
public:
	TextFormatD2D();
	virtual ~TextFormatD2D();

	virtual bool IsInitialized() const override { return m_TextFormat != nullptr; }
	virtual void SetProperties(const WCHAR* fontFamily, int size, bool bold, bool italic, Gdiplus::PrivateFontCollection* fontCollection) override;

	virtual void SetTrimming(bool trim) override;
	virtual void SetHorizontalAlignment(HorizontalAlignment alignment) override;
	virtual void SetVerticalAlignment(VerticalAlignment alignment) override;

private:
	friend class CanvasD2D;

	TextFormatD2D(const TextFormatD2D& other) {}

	void Dispose();

	IDWriteTextFormat* m_TextFormat;
	IDWriteInlineObject* m_InlineEllipsis;
};

}  // namespace Gfx

#endif