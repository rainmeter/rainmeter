/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_FONTCOLLECTION_H_
#define RM_GFX_FONTCOLLECTION_H_

#include <Windows.h>

namespace Gfx {

// Interface for a collection of fonts that may or may not be installed on the system.
class __declspec(novtable) FontCollection
{
public:
	virtual ~FontCollection();

	FontCollection(const FontCollection& other) = delete;

	// Adds a file to the collection. Returns true if the file was successfully added.
	virtual bool AddFile(const WCHAR* file) = 0;

protected:
	FontCollection();
};

}  // namespace Gfx

#endif
