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

#ifndef RM_GFX_TEXTFORMATGDIP_H_
#define RM_GFX_TEXTFORMATGDIP_H_

#include "TextFormat.h"
#include <memory>
#include <GdiPlus.h>

namespace Gfx {

// Provides a GDI+ implementation of TextFormat for use with CanvasGDIP.
class TextFormatGDIP : public TextFormat
{
public:
	TextFormatGDIP();
	virtual ~TextFormatGDIP();

	virtual bool IsInitialized() const override { return m_Font != nullptr; }

	virtual void SetProperties(
		const WCHAR* fontFamily, int size, bool bold, bool italic,
		const FontCollection* fontCollection) override;

	virtual void SetTrimming(bool trim) override;

	virtual void SetHorizontalAlignment(HorizontalAlignment alignment) override;
	virtual void SetVerticalAlignment(VerticalAlignment alignment) override;

private:
	friend class CanvasGDIP;

	TextFormatGDIP(const TextFormatGDIP& other) {}

	std::unique_ptr<Gdiplus::Font> m_Font;
	std::unique_ptr<Gdiplus::FontFamily> m_FontFamily;
	Gdiplus::StringFormat m_StringFormat;
};

}  // namespace Gfx

#endif