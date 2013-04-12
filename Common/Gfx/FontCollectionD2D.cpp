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

#include "FontCollectionD2D.h"
#include "CanvasD2D.h"
#include "Util/DWriteFontCollectionLoader.h"
#include <GdiPlus.h>

namespace Gfx {

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
	if (!m_Collection)
	{
		auto loader = Util::DWriteFontCollectionLoader::GetInstance();
		CanvasD2D::c_DWFactory->CreateCustomFontCollection(
			loader, &m_FileReferences, sizeof(m_FileReferences), &m_Collection);
	}

	return m_Collection;
}

bool FontCollectionD2D::AddFile(const WCHAR* file)
{
	// If DirecWrite font collection already exists, we need to destroy it as fonts cannot be added to
	// an existing collection. The collection will be recreated on the next call to
	// InitializeCollection().
	if (m_Collection)
	{
		m_Collection->Release();
		m_Collection = nullptr;
	}

	IDWriteFontFile* fileReference;
	HRESULT hr = CanvasD2D::c_DWFactory->CreateFontFileReference(file, nullptr, &fileReference);
	if (SUCCEEDED(hr))
	{
		m_FileReferences.push_back(fileReference);
		return true;
	}

	return false;
}

}  // namespace Gfx
