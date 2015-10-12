/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "WICBitmapLockGDIP.h"

namespace Gfx {
namespace Util {

WICBitmapLockGDIP::WICBitmapLockGDIP() :
	m_RefCount(1)
{
}

IFACEMETHODIMP WICBitmapLockGDIP::QueryInterface(REFIID riid, void** ppvObject)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP_(ULONG) WICBitmapLockGDIP::AddRef()
{
	++m_RefCount;
	return m_RefCount;
}

IFACEMETHODIMP_(ULONG) WICBitmapLockGDIP::Release()
{
	--m_RefCount;
	if (m_RefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_RefCount;
}

IFACEMETHODIMP WICBitmapLockGDIP::GetSize(UINT* puiWidth, UINT* puiHeight)
{
	*puiWidth = m_BitmapData.Width;
	*puiHeight = m_BitmapData.Height;
	return S_OK;
}

IFACEMETHODIMP WICBitmapLockGDIP::GetStride(UINT* pcbStride)
{
	*pcbStride = m_BitmapData.Stride;
	return S_OK;
}

IFACEMETHODIMP WICBitmapLockGDIP::GetDataPointer(UINT* pcbBufferSize, BYTE** ppbData)
{
	assert(m_BitmapData.PixelFormat == PixelFormat32bppPARGB);
	*pcbBufferSize = m_BitmapData.Stride * m_BitmapData.Height;
	*ppbData = (BYTE*)m_BitmapData.Scan0;
	return S_OK;
}

IFACEMETHODIMP WICBitmapLockGDIP::GetPixelFormat(WICPixelFormatGUID* pPixelFormat)
{
	assert(m_BitmapData.PixelFormat == PixelFormat32bppPARGB);
	*pPixelFormat = GUID_WICPixelFormat32bppPBGRA;
	return S_OK;
}

}  // namespace Util
}  // namespace Gfx
