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

#include "DWriteHelpers.h"

namespace Gfx {
namespace Util {

HRESULT GetDWritePropertiesFromGDIProperties(
	IDWriteFactory* factory, const WCHAR* gdiFamilyName, const bool gdiBold, const bool gdiItalic,
	DWRITE_FONT_WEIGHT& dwriteFontWeight, DWRITE_FONT_STYLE& dwriteFontStyle,
	DWRITE_FONT_STRETCH& dwriteFontStretch, WCHAR* dwriteFamilyName, UINT dwriteFamilyNameSize)
{
	HRESULT hr = E_FAIL;
	IDWriteFont* dwriteFont = CreateDWriteFontFromGDIFamilyName(factory, gdiFamilyName);
	if (dwriteFont)
	{
		if (GetFamilyNameFromDWriteFont(dwriteFont, dwriteFamilyName, dwriteFamilyNameSize))
		{
			dwriteFontWeight = dwriteFont->GetWeight();
			if (gdiBold)
			{
				if (dwriteFontWeight == DWRITE_FONT_WEIGHT_NORMAL)
				{
					dwriteFontWeight = DWRITE_FONT_WEIGHT_BOLD;
				}
				else if (dwriteFontWeight < DWRITE_FONT_WEIGHT_ULTRA_BOLD)
				{
					// If 'gdiFamilyName' was e.g. 'Segoe UI Light', |dwFontWeight| wil be equal to
					// DWRITE_FONT_WEIGHT_LIGHT. If |gdiBold| is true in that case, we need to
					// increase the weight a little more for similar results with GDI+.
					// TODO: Is +100 enough?
					dwriteFontWeight = (DWRITE_FONT_WEIGHT)(dwriteFontWeight + 100);
				}
			}

			dwriteFontStyle = dwriteFont->GetStyle();
			if (gdiItalic && dwriteFontStyle == DWRITE_FONT_STYLE_NORMAL)
			{
				dwriteFontStyle = DWRITE_FONT_STYLE_ITALIC;
			}

			dwriteFontStretch = dwriteFont->GetStretch();

			hr = S_OK;
		}

		dwriteFont->Release();
	}

	return hr;
}

IDWriteFont* CreateDWriteFontFromGDIFamilyName(IDWriteFactory* factory, const WCHAR* gdiFamilyName)
{
	IDWriteGdiInterop* dwGdiInterop;
	HRESULT hr = factory->GetGdiInterop(&dwGdiInterop);
	if (SUCCEEDED(hr))
	{
		LOGFONT lf = {};
		wcscpy_s(lf.lfFaceName, gdiFamilyName);
		lf.lfHeight = -12;
		lf.lfWeight = FW_DONTCARE;
		lf.lfCharSet = DEFAULT_CHARSET;
		lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
		lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		lf.lfQuality = ANTIALIASED_QUALITY;
		lf.lfPitchAndFamily = VARIABLE_PITCH;

		IDWriteFont* dwFont;
		hr = dwGdiInterop->CreateFontFromLOGFONT(&lf, &dwFont);
		if (SUCCEEDED(hr))
		{
			return dwFont;
		}

		dwGdiInterop->Release();
	}

	return nullptr;
}

HRESULT GetFamilyNameFromDWriteFont(IDWriteFont* font, WCHAR* buffer, const UINT bufferSize)
{
	IDWriteFontFamily* dwriteFontFamily;
	HRESULT hr = font->GetFontFamily(&dwriteFontFamily);
	if (SUCCEEDED(hr))
	{
		GetFamilyNameFromDWriteFontFamily(dwriteFontFamily, buffer, bufferSize);
		dwriteFontFamily->Release();
	}

	return hr;
}

HRESULT GetFamilyNameFromDWriteFontFamily(
	IDWriteFontFamily* fontFamily, WCHAR* buffer, const UINT bufferSize)
{
	IDWriteLocalizedStrings* dwFamilyNames;
	HRESULT hr = fontFamily->GetFamilyNames(&dwFamilyNames);
	if (SUCCEEDED(hr))
	{
		// TODO: Determine the best index?
		hr = dwFamilyNames->GetString(0, buffer, bufferSize);
		dwFamilyNames->Release();
	}

	return hr;
}

bool IsFamilyInSystemFontCollection(IDWriteFactory* factory, const WCHAR* familyName)
{
	bool result = false;
	IDWriteFontCollection* systemFontCollection;
	HRESULT hr = factory->GetSystemFontCollection(&systemFontCollection);
	if (SUCCEEDED(hr))
	{
		UINT32 familyNameIndex;
		BOOL familyNameFound;
		HRESULT hr = systemFontCollection->FindFamilyName(
			familyName, &familyNameIndex, &familyNameFound);
		if (SUCCEEDED(hr) && familyNameFound)
		{
			result = true;
		}

		systemFontCollection->Release();
	}

	return result;
}

}  // namespace Util
}  // namespace Gfx
