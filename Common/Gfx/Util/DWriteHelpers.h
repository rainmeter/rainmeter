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

#ifndef RM_GFX_UTIL_DWRITEHELPERS_H_
#define RM_GFX_UTIL_DWRITEHELPERS_H_

#include <dwrite_1.h>

namespace Gfx {
namespace Util {

// Maps the GDI family name and italic/bold flags to the DirectWrite family name, weight, style,
// and stretch.
HRESULT GetDWritePropertiesFromGDIProperties(
	IDWriteFactory* factory, const WCHAR* gdiFamilyName, const bool gdiBold, const bool gdiItalic,
	DWRITE_FONT_WEIGHT& dwriteFontWeight, DWRITE_FONT_STYLE& dwriteFontStyle,
	DWRITE_FONT_STRETCH& dwriteFontStretch, WCHAR* dwriteFamilyName, UINT dwriteFamilyNameSize);

void GetPropertiesFromDWriteFont(
	IDWriteFont* dwriteFont, const bool bold, const bool italic,
	DWRITE_FONT_WEIGHT* dwriteFontWeight, DWRITE_FONT_STYLE* dwriteFontStyle,
	DWRITE_FONT_STRETCH* dwriteFontStretch);

// Creates a IDWriteFont using the GDI family name instead of the DirectWrite family name. For
// example, 'Segoe UI' and 'Segoe UI Semibold' are separate family names with GDI whereas
// DirectWrite uses the family name 'Segoe UI' for both and differentiates them by the font
// style.
IDWriteFont* CreateDWriteFontFromGDIFamilyName(IDWriteFactory* factory, const WCHAR* fontFamily);

HRESULT GetFamilyNameFromDWriteFont(IDWriteFont* font, WCHAR* buffer, UINT bufferSize);

HRESULT GetFamilyNameFromDWriteFontFamily(
	IDWriteFontFamily* fontFamily, WCHAR* buffer, UINT bufferSize);

bool IsFamilyInSystemFontCollection(IDWriteFactory* factory, const WCHAR* familyName);

IDWriteFont* FindDWriteFontInFontCollectionByGDIFamilyName(
	IDWriteFontCollection* fontCollection, const WCHAR* gdiFamilyName);

}  // namespace Util
}  // namespace Gfx

#endif