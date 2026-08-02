// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <vector>
#include <dwrite_1.h>

namespace Gfx {
namespace Util {

// Implements IDWriteFontFileEnumerator by enumerating over std::vector<IDWriteFontFile*>.
class DWriteFontFileEnumerator : public IDWriteFontFileEnumerator
{
public:
	DWriteFontFileEnumerator(const std::vector<IDWriteFontFile*>& fontFiles);

	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID uuid, void** object) override;
	virtual ULONG STDMETHODCALLTYPE AddRef() override;
	virtual ULONG STDMETHODCALLTYPE Release() override;

	// IDWriteFontFileEnumerator
	virtual HRESULT STDMETHODCALLTYPE MoveNext(BOOL* hasCurrentFile) override;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentFontFile(IDWriteFontFile** currentFontFile) override;

private:
	ULONG m_RefCount;

	const std::vector<IDWriteFontFile*>& m_FontFiles;

	// Current index of |m_FontFiles|. The type is int instead of size_t as it starts from -1 as
	// required by IDWriteFontFileEnumerator.
	int m_CurrentFontFileIndex;
};

}  // namespace Util
}  // namespace Gfx
