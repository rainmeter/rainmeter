/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "WICBitmapDIB.h"
#include "WICBitmapLockDIB.h"

namespace Gfx {
namespace Util {

WICBitmapDIB::WICBitmapDIB() :
	m_DIBSectionBuffer(),
	m_DIBSectionBufferPixels(),
	m_W(0),
	m_H(0)
{
}

WICBitmapDIB::~WICBitmapDIB()
{
	if (m_DIBSectionBuffer)
	{
		DeleteObject(m_DIBSectionBuffer);
		m_DIBSectionBufferPixels = nullptr;
	}
}

void WICBitmapDIB::Resize(UINT w, UINT h)
{
	if (m_DIBSectionBuffer)
	{
		DeleteObject(m_DIBSectionBuffer);
		m_DIBSectionBufferPixels = nullptr;
	}

	m_W = w;
	m_H = h;

	BITMAPV4HEADER bh = {sizeof(BITMAPV4HEADER)};
	bh.bV4Width = (LONG)m_W;
	bh.bV4Height = -(LONG)m_H;	// Top-down DIB
	bh.bV4Planes = 1;
	bh.bV4BitCount = 32;
	bh.bV4V4Compression = BI_BITFIELDS;
	bh.bV4RedMask = 0x00FF0000;
	bh.bV4GreenMask = 0x0000FF00;
	bh.bV4BlueMask = 0x000000FF;
	bh.bV4AlphaMask = 0xFF000000;

	m_DIBSectionBuffer = CreateDIBSection(
		nullptr,
		(BITMAPINFO*)&bh,
		DIB_RGB_COLORS,
		(void**)&m_DIBSectionBufferPixels,
		nullptr,
		0);

	assert(m_DIBSectionBufferPixels);
}

IFACEMETHODIMP WICBitmapDIB::QueryInterface(REFIID riid, void** ppvObject)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP_(ULONG) WICBitmapDIB::AddRef()
{
	return 0;
}

IFACEMETHODIMP_(ULONG) WICBitmapDIB::Release()
{
	return 0;
}

IFACEMETHODIMP WICBitmapDIB::GetSize(UINT* puiWidth, UINT* puiHeight)
{
	if (puiWidth) *puiWidth = m_W;
	if (puiHeight) *puiHeight = m_H;
	return S_OK;
}

IFACEMETHODIMP WICBitmapDIB::GetPixelFormat(WICPixelFormatGUID* pPixelFormat)
{
	*pPixelFormat = GUID_WICPixelFormat32bppPBGRA;
	return S_OK;
}

IFACEMETHODIMP WICBitmapDIB::GetResolution(double* pDpiX, double* pDpiY)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP WICBitmapDIB::CopyPalette(IWICPalette* pIPalette)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP WICBitmapDIB::CopyPixels(const WICRect* prc, UINT cbStride, UINT cbBufferSize, BYTE* pbBuffer)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP WICBitmapDIB::Lock(const WICRect* prcLock, DWORD flags, IWICBitmapLock** ppILock)
{
	if (ppILock) *ppILock = (IWICBitmapLock*)new WICBitmapLockDIB(this, prcLock);
	return S_OK;
}

IFACEMETHODIMP WICBitmapDIB::SetPalette(IWICPalette* pIPalette)
{
	return E_NOTIMPL;
}

IFACEMETHODIMP WICBitmapDIB::SetResolution(double dpiX, double dpiY)
{
	return E_NOTIMPL;
}

}  // namespace Util
}  // namespace Gfx
