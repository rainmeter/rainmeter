/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_UTIL_WICBITMAPDIB_H_
#define RM_GFX_UTIL_WICBITMAPDIB_H_

#include <Windows.h>
#include <wincodec.h>

namespace Gfx {
namespace Util {

// Allows the use of a DIB section (HBITMAP) in Direct2D as a WIC bitmap. It is assumed that this
// class is used only with 32bpp PARGB bitmaps and using a sigle thread.
//
// This class does not follow the COM reference count model. RTTI is used instead. This class
// implements only the bare essentials in order to use a DIB section as a Direct2D render target.
class WICBitmapDIB : public IWICBitmap
{
public:
	WICBitmapDIB();
	~WICBitmapDIB();

	WICBitmapDIB(const WICBitmapDIB& other) = delete;
	WICBitmapDIB& operator=(WICBitmapDIB other) = delete;

	void Resize(UINT w, UINT h);

	HBITMAP GetHandle() const { return m_DIBSectionBuffer; }
	BYTE* GetData() const { return (BYTE*)m_DIBSectionBufferPixels; }

	// IUnknown
	IFACEMETHOD(QueryInterface)(REFIID riid, void** ppvObject);
	IFACEMETHOD_(ULONG, AddRef)();
	IFACEMETHOD_(ULONG, Release)();

	// IWICBitmapSource
	IFACEMETHOD(GetSize)(UINT* puiWidth, UINT* puiHeight);
	IFACEMETHOD(GetPixelFormat)(WICPixelFormatGUID* pPixelFormat);
	IFACEMETHOD(GetResolution)(double* pDpiX, double* pDpiY);
	IFACEMETHOD(CopyPalette)(IWICPalette* pIPalette);
	IFACEMETHOD(CopyPixels)(const WICRect* prc, UINT cbStride, UINT cbBufferSize, BYTE* pbBuffer);

	// IWICBitmap
	IFACEMETHOD(Lock)(const WICRect* prcLock, DWORD flags, IWICBitmapLock** ppILock);
	IFACEMETHOD(SetPalette)(IWICPalette* pIPalette);
	IFACEMETHOD(SetResolution)(double dpiX, double dpiY);

private:
	friend class WICBitmapLockDIB;

	HBITMAP m_DIBSectionBuffer;
	LPDWORD m_DIBSectionBufferPixels;
	UINT m_W;
	UINT m_H;
};

}  // namespace Util
}  // namespace Gfx

#endif
