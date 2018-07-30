/* Copyright (C) 2018 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_UTIL_D2DBITMAPLOADER_H_
#define RM_GFX_UTIL_D2DBITMAPLOADER_H_

#include "../D2DBitmap.h"

namespace Gfx {
namespace Util {

class D2DBitmapLoader
{
public:
	static HRESULT LoadBitmapFromFile(const Canvas& canvas, D2DBitmap* bitmap);
	static bool HasFileChanged(D2DBitmap* bitmap, const std::wstring& file);
	static HRESULT GetFileInfo(const std::wstring& path, FileInfo* fileInfo);

private:
	friend class Gfx::Canvas;

	D2DBitmapLoader() = delete;
	~D2DBitmapLoader() = delete;
	D2DBitmapLoader(const D2DBitmapLoader& other) = delete;
	D2DBitmapLoader& operator=(D2DBitmapLoader other) = delete;

	static HRESULT CropWICBitmapSource(WICRect& clipRect,
		IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest);
	static HRESULT ConvertToD2DFormat(IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest);
	static int GetExifOrientation(IWICBitmapFrameDecode* source);
};

}  // namespace Util
}  // namespace Gfx

#endif
