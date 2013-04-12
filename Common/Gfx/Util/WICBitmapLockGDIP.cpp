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

#include "WICBitmapLockGDIP.h"
#include <cassert>

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
