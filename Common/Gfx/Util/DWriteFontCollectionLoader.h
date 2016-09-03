/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_UTIL_DWRITEFONTCOLLECTIONLOADER_H_
#define RM_GFX_UTIL_DWRITEFONTCOLLECTIONLOADER_H_

#include <dwrite_1.h>

namespace Gfx {
namespace Util {

// Implements the IDWriteFontCollectionLoader interface as a singleton object. When
// CreateEnumeratorFromKey is called, a new DWriteFontFileEnumerator object is created using
// |fontCollectionKey| (which is assumed to be a pointer to std::vector<IDWriteFontFile*>).
class DWriteFontCollectionLoader : public IDWriteFontCollectionLoader
{
public:
	static DWriteFontCollectionLoader* GetInstance();

	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID uuid, void** object) override;
	virtual ULONG STDMETHODCALLTYPE AddRef() override;
	virtual ULONG STDMETHODCALLTYPE Release() override;

	// IFontCollectionLoader
	virtual HRESULT STDMETHODCALLTYPE CreateEnumeratorFromKey(
		IDWriteFactory* factory, void const* fontCollectionKey, UINT32 fontCollectionKeySize,
		IDWriteFontFileEnumerator** fontFileEnumerator) override;

private:
	DWriteFontCollectionLoader() {}
	DWriteFontCollectionLoader(const DWriteFontCollectionLoader& other) {}
};

}  // namespace Util
}  // namespace Gfx

#endif
