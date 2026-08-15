// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <Windows.h>
#include <vector>
#include <dwrite_1.h>
#include <wrl/client.h>

namespace Gfx {

// Wraps the DirectWrite IDWriteFontCollection for use with Canvas.
class FontCollection final
{
public:
	~FontCollection();

	FontCollection(const FontCollection& other) = delete;
	FontCollection& operator=(FontCollection other) = delete;

	bool AddFile(const WCHAR* file);

	bool InitializeCollection();

	bool GetSystemFontFamilies(UINT32& familyCount, std::wstring& families);
	bool GetFontFamilies(UINT32& familyCount, std::wstring& families);

protected:
	FontCollection();

private:
	friend class Canvas;
	friend class TextFormat;

	void Dispose();

	bool GetFontFamiliesFromCollection(IDWriteFontCollection* collection, UINT32& familyCount,
		std::wstring & families, bool isSystemCollection);

	std::vector<IDWriteFontFile*> m_FileReferences;
	IDWriteFontCollection* m_Collection;

	static Microsoft::WRL::ComPtr<IDWriteFontCollection> c_SystemCollection;
};

}  // namespace Gfx
