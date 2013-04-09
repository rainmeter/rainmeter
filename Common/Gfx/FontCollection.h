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

#ifndef RM_GFX_FONTCOLLECTION_H_
#define RM_GFX_FONTCOLLECTION_H_

#include <Windows.h>

namespace Gfx {

// Interface for a collection of fonts that may or may not be installed on the system.
class __declspec(novtable) FontCollection
{
public:
	virtual ~FontCollection();

	// Adds a file to the collection. Returns true if the file was successfully added.
	virtual bool AddFile(const WCHAR* file) = 0;

protected:
	FontCollection();

private:
	FontCollection(const FontCollection& other) {}
};

}  // namespace Gfx

#endif