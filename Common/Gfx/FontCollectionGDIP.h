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

#ifndef RM_GFX_FONTCOLLECTIONGDIP_H_
#define RM_GFX_FONTCOLLECTIONGDIP_H_

#include "FontCollection.h"

namespace Gdiplus {
class PrivateFontCollection;
}

namespace Gfx {

// Wraps the GDI+ PrivateFontCollection for use with CanvasGDIP.
class FontCollectionGDIP final : public FontCollection
{
public:
	virtual ~FontCollectionGDIP();

	virtual bool AddFile(const WCHAR* file) override;

protected:
	FontCollectionGDIP();

private:
	friend class CanvasGDIP;
	friend class TextFormatGDIP;

	FontCollectionGDIP(const FontCollectionGDIP& other) {}

	void Dispose();

	Gdiplus::PrivateFontCollection* m_PrivateCollection;
};

}  // namespace Gfx

#endif