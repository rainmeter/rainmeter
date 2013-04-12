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

#include "WICBitmapLockDIB.h"
#include <cassert>

namespace Gfx {
namespace Util {

WICBitmapLockDIB::WICBitmapLockDIB(WICBitmapDIB* bitmap, const WICRect* lockRect) :
	m_Bitmap(bitmap),
	m_Rect(lockRect),
	m_RefCount(1)
{
}

WICBitmapLockDIB::~WICBitmapLockDIB()
{
}

IFACEMETHODIMP WICBitmapLockDIB::QueryInterface(REFIID riid, void** ppvObject)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP_(ULONG) WICBitmapLockDIB::AddRef()
{
	++m_RefCount;
	return m_RefCount;
}

IFACEMETHODIMP_(ULONG) WICBitmapLockDIB::Release()
{
	--m_RefCount;
	if (m_RefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_RefCount;
}

IFACEMETHODIMP WICBitmapLockDIB::GetSize(UINT* puiWidth, UINT* puiHeight)
{
	return m_Bitmap->GetSize(puiWidth, puiHeight);
}

IFACEMETHODIMP WICBitmapLockDIB::GetStride(UINT* pcbStride)
{
	UINT width = 0;
	m_Bitmap->GetSize(&width, nullptr);

	if (pcbStride) *pcbStride = (width * 32 + 31) / 32 * 4;

	return S_OK;
}

IFACEMETHODIMP WICBitmapLockDIB::GetDataPointer(UINT* pcbBufferSize, BYTE** ppbData)
{
	UINT stride = 0;
	GetStride(&stride);

	if (pcbBufferSize) *pcbBufferSize = stride * m_Rect->Height;
	if (ppbData) *ppbData = (BYTE*)&m_Bitmap->m_DIBSectionBufferPixels[m_Rect->Y * m_Rect->Width + m_Rect->X];

	return S_OK;
}

IFACEMETHODIMP WICBitmapLockDIB::GetPixelFormat(WICPixelFormatGUID* pPixelFormat)
{
	return m_Bitmap->GetPixelFormat(pPixelFormat);
}

}  // namespace Util
}  // namespace Gfx
