// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../Bitmap.h"

namespace Gfx {
namespace Util {

class BitmapLoader
{
public:
	static HRESULT LoadBitmapFromFile(const Canvas& canvas, Bitmap* bitmap);
	static HRESULT LoadBitmapFromIcon(const Canvas& canvas, Bitmap* bitmap, HICON icon, float scale);
	static bool HasFileChanged(Bitmap* bitmap, const std::wstring& file);
	static HRESULT GetFileInfo(const std::wstring& path, FileInfo* fileInfo);

private:
	friend class Gfx::Canvas;

	BitmapLoader() = delete;
	~BitmapLoader() = delete;
	BitmapLoader(const BitmapLoader& other) = delete;
	BitmapLoader& operator=(BitmapLoader other) = delete;

	static HRESULT CropWICBitmapSource(WICRect& clipRect,
		IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest);
	static HRESULT ConvertToD2DFormat(IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest);
	static HRESULT CreateAlphaMask(IWICBitmapSource* source, UINT width, UINT height, std::vector<BYTE>& alphaMask);
	static int GetExifOrientation(IWICBitmapFrameDecode* source);
};

}  // namespace Util
}  // namespace Gfx
