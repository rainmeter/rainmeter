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

#include "FontCollectionD2D.h"
#include <GdiPlus.h>

namespace Gfx {

FontCollectionD2D::FontCollectionD2D() : FontCollection()
{
}

FontCollectionD2D::~FontCollectionD2D()
{
}

bool FontCollectionD2D::AddFile(const WCHAR* file)
{
	// FIXME.
	return true;
}

}  // namespace Gfx
