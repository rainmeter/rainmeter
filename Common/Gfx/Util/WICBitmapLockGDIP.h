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

#ifndef RM_GFX_UTIL_WICBITMAPLOCKGDIP_H_
#define RM_GFX_UTIL_WICBITMAPLOCKGDIP_H_

#include <Windows.h>
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
	Gdiplus::BitmapData m_BitmapData;
	UINT m_RefCount;
};

}  // namespace Util
}  // namespace Gfx

#endif