/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_UTIL_WICBITMAPLOCKDIB_H_
#define RM_GFX_UTIL_WICBITMAPLOCKDIB_H_

#include <Windows.h>
#include <wincodec.h>
#include "WICBitmapDIB.h"

namespace Gfx {
namespace Util {

// Implements the IWICBitmapLock interface for use with WICBitmapDIB. It is assumed that this
// class is used only with 32bpp PARGB bitmaps and using a sigle thread.
class WICBitmapLockDIB : public IWICBitmapLock
{
public:
	WICBitmapLockDIB(WICBitmapDIB* bitmap, const WICRect* lockRect);
	virtual ~WICBitmapLockDIB();

	void Resize(UINT w, UINT h);

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
	WICBitmapLockDIB(const WICBitmapLockDIB& other) = delete;
	WICBitmapLockDIB& operator=(WICBitmapLockDIB other) = delete;

	WICBitmapDIB* m_Bitmap;
	const WICRect* m_Rect;
	UINT m_RefCount;
};

}  // namespace Util
}  // namespace Gfx

#endif
