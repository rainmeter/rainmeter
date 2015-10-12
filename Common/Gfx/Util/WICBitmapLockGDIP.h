/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_UTIL_WICBITMAPLOCKGDIP_H_
#define RM_GFX_UTIL_WICBITMAPLOCKGDIP_H_

#include <Windows.h>
#include <ole2.h>  // For Gdiplus.h.
#include <GdiPlus.h>
#include <wincodec.h>

namespace Gfx {
namespace Util {

// Allows the creation of a shared ID2D1Bitmap using pixel data in a Gdiplus::Bitmap. It is
// assumed that this class is used only with 32bpp PARGB bitmaps and using a sigle thread.
class WICBitmapLockGDIP : public IWICBitmapLock
{
public:
	WICBitmapLockGDIP();

	Gdiplus::BitmapData* GetBitmapData() { return &m_BitmapData; }

	// IUnknown
	IFACEMETHOD(QueryInterface)(REFIID riid, void** ppvObject);
	IFACEMETHOD_(ULONG, AddRef)();
	IFACEMETHOD_(ULONG, Release)();

	// IWICBitmapLock
	IFACEMETHOD(GetSize)(UINT* puiWidth, UINT* puiHeight);
	IFACEMETHOD(GetStride)(UINT* pcbStride);
	IFACEMETHOD(GetDataPointer)(UINT* pcbBufferSize, BYTE** ppbData);
	IFACEMETHOD(GetPixelFormat)(WICPixelFormatGUID* pPixelFormat);

private:
	WICBitmapLockGDIP(const WICBitmapLockGDIP& other) = delete;
	WICBitmapLockGDIP& operator=(WICBitmapLockGDIP other) = delete;

	Gdiplus::BitmapData m_BitmapData;
	UINT m_RefCount;
};

}  // namespace Util
}  // namespace Gfx

#endif
