// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

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
