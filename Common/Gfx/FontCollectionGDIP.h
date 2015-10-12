/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

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

	FontCollectionGDIP(const FontCollectionGDIP& other) = delete;
	FontCollectionGDIP& operator=(FontCollectionGDIP other) = delete;

	virtual bool AddFile(const WCHAR* file) override;

protected:
	FontCollectionGDIP();

private:
	friend class CanvasGDIP;
	friend class TextFormatGDIP;

	void Dispose();

	Gdiplus::PrivateFontCollection* m_PrivateCollection;
};

}  // namespace Gfx

#endif
