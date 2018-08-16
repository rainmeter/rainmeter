/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "FontCollectionD2D.h"
#include "Canvas.h"
#include "Util/DWriteFontCollectionLoader.h"

namespace Gfx {

Microsoft::WRL::ComPtr<IDWriteFontCollection> FontCollectionD2D::c_SystemCollection;

FontCollectionD2D::FontCollectionD2D() : FontCollection(),
	m_Collection()
{
}

FontCollectionD2D::~FontCollectionD2D()
{
	Dispose();
}

void FontCollectionD2D::Dispose()
{
	for (IDWriteFontFile* fileReference : m_FileReferences)
	{
		fileReference->Release();
	}
	m_FileReferences.clear();
}

bool FontCollectionD2D::InitializeCollection()
{
	if (!c_SystemCollection)
	{
		Canvas::c_DWFactory->GetSystemFontCollection(c_SystemCollection.GetAddressOf(), TRUE);
	}

	if (!m_Collection)
	{
		auto loader = Util::DWriteFontCollectionLoader::GetInstance();
		Canvas::c_DWFactory->CreateCustomFontCollection(
			loader, &m_FileReferences, sizeof(m_FileReferences), &m_Collection);
	}

	return m_Collection != nullptr;
}

bool FontCollectionD2D::AddFile(const WCHAR* file)
{
	// If DirectWrite font collection already exists, we need to destroy it as fonts cannot be added to
	// an existing collection. The collection will be recreated on the next call to
	// InitializeCollection().
	if (m_Collection)
	{
		m_Collection->Release();
		m_Collection = nullptr;
	}

	IDWriteFontFile* fileReference;
	HRESULT hr = Canvas::c_DWFactory->CreateFontFileReference(file, nullptr, &fileReference);
	if (SUCCEEDED(hr))
	{
		m_FileReferences.push_back(fileReference);
		return true;
	}

	return false;
}

}  // namespace Gfx
