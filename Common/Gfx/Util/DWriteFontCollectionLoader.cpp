// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "DWriteFontCollectionLoader.h"
#include "DWriteFontFileEnumerator.h"

namespace Gfx {
namespace Util {

DWriteFontCollectionLoader* DWriteFontCollectionLoader::GetInstance()
{
	static DWriteFontCollectionLoader s_Instance;
	return &s_Instance;
}

ULONG STDMETHODCALLTYPE DWriteFontCollectionLoader::AddRef()
{
	// This is a singleton class so return a dummy value.
	return 1;
}

ULONG STDMETHODCALLTYPE DWriteFontCollectionLoader::Release()
{
	// This is a singleton class so return a dummy value.
	return 1;
}

HRESULT STDMETHODCALLTYPE DWriteFontCollectionLoader::QueryInterface(IID const& riid, void** ppvObject)
{
	if (riid == IID_IUnknown ||
		riid == __uuidof(IDWriteFontCollectionLoader))
	{
		*ppvObject = this;
		return S_OK;
	}

	*ppvObject = nullptr;
	return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE DWriteFontCollectionLoader::CreateEnumeratorFromKey(
	IDWriteFactory* factory, void const* collectionKey, UINT32 collectionKeySize,
	IDWriteFontFileEnumerator** fontFileEnumerator)
{
	*fontFileEnumerator = new DWriteFontFileEnumerator(
		*(const std::vector<IDWriteFontFile*>*)collectionKey);
	return S_OK;
}

}  // namespace Util
}  // namespace Gfx
