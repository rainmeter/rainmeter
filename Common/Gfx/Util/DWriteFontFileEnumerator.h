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