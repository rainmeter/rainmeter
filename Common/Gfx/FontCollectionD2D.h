/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_FONTCOLLECTIOND2D_H_
#define RM_GFX_FONTCOLLECTIOND2D_H_

#include "FontCollection.h"
#include <vector>
#include <dwrite_1.h>
#include <wrl/client.h>

namespace Gfx {

// Wraps the DirectWrite IDWriteFontCollection for use with CanvasD2D.
class FontCollectionD2D final : public FontCollection
{
public:
	virtual ~FontCollectionD2D();

	FontCollectionD2D(const FontCollectionD2D& other) = delete;
	FontCollectionD2D& operator=(FontCollectionD2D other) = delete;

	virtual bool AddFile(const WCHAR* file) override;

protected:
	FontCollectionD2D();

private:
	friend class Canvas;
	friend class TextFormatD2D;
	friend class TextInlineFormat_Face;

	void Dispose();

	bool InitializeCollection();

	std::vector<IDWriteFontFile*> m_FileReferences;
	IDWriteFontCollection* m_Collection;

	static Microsoft::WRL::ComPtr<IDWriteFontCollection> c_SystemCollection;
};

}  // namespace Gfx

#endif
