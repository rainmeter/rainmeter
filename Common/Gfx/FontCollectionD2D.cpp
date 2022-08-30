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

#include <algorithm>

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

bool FontCollectionD2D::GetSystemFontFamilies(UINT32& familyCount, std::wstring& families)
{
	return GetFontFamiliesFromCollection(c_SystemCollection.Get(), familyCount, families, true);
}

bool FontCollectionD2D::GetFontFamilies(UINT32& familyCount, std::wstring& families)
{
	return GetFontFamiliesFromCollection(m_Collection, familyCount, families, false);
}

bool FontCollectionD2D::GetFontFamiliesFromCollection(IDWriteFontCollection* collection,
	UINT32& familyCount, std::wstring& families, bool isSystemCollection)
{
	// Reset values
	families.clear();
	familyCount = 0U;

	if (collection)
	{
		const UINT32 nameSize = 256U;
		std::vector<std::wstring> familyList;

		UINT32 lockedEntries = 0U;

		UINT32 count = collection->GetFontFamilyCount();
		if (count == 0U)
		{
			families = isSystemCollection ?
				L"Font Enumeration: No installed fonts found" :
				L"Font Enumeration: No fonts found";
			return false;
		}

		for (UINT32 i = 0U; i < count; ++i)
		{
			Microsoft::WRL::ComPtr<IDWriteFontFamily> family;
			HRESULT hr = collection->GetFontFamily(i, &family);
			if (FAILED(hr))
			{
				families = L"Font Enumeration: GetFontFamily failed";
				return false;
			}

			Microsoft::WRL::ComPtr<IDWriteLocalizedStrings> familyNames;
			hr = family->GetFamilyNames(&familyNames);
			if (FAILED(hr))
			{
				families = L"Font Enumeration: GetFamilyNames failed";
				return false;
			}

			UINT32 index = 0U;
			BOOL exists = FALSE;
			std::wstring familyName;

			// Find the English entry first
			hr = familyNames->FindLocaleName(L"en-us", &index, &exists);
			if (FAILED(hr))
			{
				families = L"Font Enumeration: FindLocaleName failed (\"en-us\" locale)";
				return false;
			}

			if (!exists)
			{
				// Find the name using the current user's locale
				WCHAR localeName[LOCALE_NAME_MAX_LENGTH] = { 0 };
				if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH))
				{
					hr = familyNames->FindLocaleName(localeName, &index, &exists);
					if (FAILED(hr))
					{
						families = L"Font Enumeration: FindLocaleName failed (user default locale)";
						return false;
					}

					familyName = L'*';
				}
			}

			if (!exists)
			{
				// Since there is neither an English name entry, nor an entry
				// in the users locale, just return the first entry's name
				index = 0U;
				familyName = L"**";
			}

			WCHAR name[nameSize + 3] = { 0 };
			hr = familyNames->GetString(index, name, nameSize);
			if (FAILED(hr))
			{
				families = L"Font Enumeration: GetString failed";
				return false;
			}

			familyName.insert(0, name);
			familyList.emplace_back(familyName);

			// Find each font in the family
			UINT32 fontCount = family->GetFontCount();
			for (UINT32 j = 0U; j < fontCount; ++j)
			{
				Microsoft::WRL::ComPtr<IDWriteFont> font;
				hr = family->GetFont(j, &font);
				if (FAILED(hr))
				{
					// Sometimes a font is locked by another application resulting in error: DWRITE_E_FILEACCESS
					// In this case, log the family name and index and move to the next font
					WCHAR tmp[nameSize + 80] = { 0 };
					_snwprintf_s(tmp, _TRUNCATE, L"%s (error=0x%.8X, family=%d, index=%d)***", name, hr, index, j);
					familyList.emplace_back(tmp);
					++lockedEntries;
					continue;
				}

				// Skip any simulated fonts (Bold and Oblique)
				DWRITE_FONT_SIMULATIONS simulations = font->GetSimulations();
				if (simulations != DWRITE_FONT_SIMULATIONS_NONE) continue;

				Microsoft::WRL::ComPtr<IDWriteLocalizedStrings> faceNames;
				hr = font->GetFaceNames(&faceNames);
				if (FAILED(hr)) continue;

				UINT32 faceIndex = 0U;
				BOOL faceExists = FALSE;
				std::wstring faceName;

				// Find the English entry first
				hr = faceNames->FindLocaleName(L"en-us", &faceIndex, &faceExists);
				if (FAILED(hr)) continue;

				if (!faceExists)
				{
					// Find the name using the current user's locale
					WCHAR localeName[LOCALE_NAME_MAX_LENGTH] = { 0 };
					if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH))
					{
						hr = faceNames->FindLocaleName(localeName, &faceIndex, &faceExists);
						if (FAILED(hr)) continue;

						faceName = L'*';
					}
				}

				if (!faceExists)
				{
					// Since there is neither an English name entry, nor an entry
					// in the users locale, just return the first entry's name
					faceIndex = 0U;
					faceName = L"**";
				}

				WCHAR fName[nameSize + 3] = { 0 };
				hr = faceNames->GetString(faceIndex, fName, nameSize);
				if (FAILED(hr))
				{
					families = L"Face Enumeration: GetString failed";
					return false;
				}

				// Insert family name first, then font face (before locale flags)
				faceName.insert(0, fName);
				faceName.insert(0, L" ");
				faceName.insert(0, name);
				familyList.push_back(faceName);
			}
		}

		std::sort(familyList.begin(), familyList.end());  // Sort alphabetically
		auto lastItem = std::unique(familyList.begin(), familyList.end());  // Remove duplicates
		familyList.erase(lastItem, familyList.end());  // Remove remaining commas

		familyCount = (UINT32)familyList.size() - lockedEntries;;

		// Convert list to a string to be logged
		for (size_t i = 0ULL, size = familyList.size(); i < size; ++i)
		{
			families += familyList[i];
			if (i < (size - 1ULL)) families += L", ";
		}

		return true;
	}

	families = L"Font Enumeration: Font collection is not initialized";
	return false;
}

}  // namespace Gfx
