// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
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
