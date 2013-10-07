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

#include "DWriteFontFileEnumerator.h"

namespace Gfx {
namespace Util {

DWriteFontFileEnumerator::DWriteFontFileEnumerator(const std::vector<IDWriteFontFile*>& fontFiles) :
	m_RefCount(1),
	m_FontFiles(fontFiles),
	m_CurrentFontFileIndex(-1)
{
}

ULONG STDMETHODCALLTYPE DWriteFontFileEnumerator::AddRef()
{
	++m_RefCount;
	return m_RefCount;
}

ULONG STDMETHODCALLTYPE DWriteFontFileEnumerator::Release()
{
	--m_RefCount;
	if (m_RefCount == 0)
	{
		delete this;
		return 0;
	}
	return m_RefCount;
}

HRESULT STDMETHODCALLTYPE DWriteFontFileEnumerator::QueryInterface(IID const& riid, void** ppvObject)
{
	if (riid == IID_IUnknown ||
		riid == __uuidof(IDWriteFontFileEnumerator))
	{
		*ppvObject = this;
		AddRef();
		return S_OK;
	}

	*ppvObject = nullptr;
	return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE DWriteFontFileEnumerator::MoveNext(BOOL* hasCurrentFile)
{
	*hasCurrentFile = (++m_CurrentFontFileIndex < (int)m_FontFiles.size());
	return S_OK;
}

HRESULT STDMETHODCALLTYPE DWriteFontFileEnumerator::GetCurrentFontFile(IDWriteFontFile** fontFile)
{
	IDWriteFontFile* currentFontFile = m_FontFiles[m_CurrentFontFileIndex];
	currentFontFile->AddRef();
	*fontFile = currentFontFile;
	return S_OK;
}

}  // namespace Util
}  // namespace Gfx
