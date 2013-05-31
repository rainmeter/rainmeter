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

#include "DWriteFontCollectionLoader.h"
#include "DWriteFontFileEnumerator.h"
#include <vector>

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
