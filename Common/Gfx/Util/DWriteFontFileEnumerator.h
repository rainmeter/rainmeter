/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_UTIL_DWRITEFONTFILEENUMERATOR_H_
#define RM_GFX_UTIL_DWRITEFONTFILEENUMERATOR_H_

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

#endif
