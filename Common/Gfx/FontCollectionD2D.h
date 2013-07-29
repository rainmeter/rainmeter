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

#ifndef RM_GFX_FONTCOLLECTIOND2D_H_
#define RM_GFX_FONTCOLLECTIOND2D_H_

#include "FontCollection.h"
#include <vector>
#include <dwrite_1.h>

namespace Gfx {

// Wraps the DirectWrite IDWriteFontCollection for use with CanvasD2D.
class FontCollectionD2D final : public FontCollection
{
public:
	virtual ~FontCollectionD2D();

	virtual bool AddFile(const WCHAR* file) override;

protected:
	FontCollectionD2D();

private:
	friend class CanvasD2D;
	friend class TextFormatD2D;

	FontCollectionD2D(const FontCollectionD2D& other) {}

	void Dispose();

	bool InitializeCollection();

	std::vector<IDWriteFontFile*> m_FileReferences;
	IDWriteFontCollection* m_Collection;
};

}  // namespace Gfx

#endif