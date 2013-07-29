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