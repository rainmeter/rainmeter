// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

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
