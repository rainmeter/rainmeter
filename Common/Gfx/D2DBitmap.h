/* Copyright (C) 2018 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_UTIL_D2DBITMAP_H_
#define RM_GFX_UTIL_D2DBITMAP_H_

#include "Canvas.h"

namespace Gfx {

class Canvas;

class BitmapSegment
{
public:
	BitmapSegment(Microsoft::WRL::ComPtr<ID2D1Bitmap1>& bitmap, UINT x, UINT y, UINT width, UINT height);
	BitmapSegment(Microsoft::WRL::ComPtr<ID2D1Bitmap1>& bitmap, D2D1_RECT_U& rect);
	BitmapSegment(Microsoft::WRL::ComPtr<ID2D1Bitmap1>& bitmap, WICRect& rect);
	~BitmapSegment() { }

	UINT GetX() { return m_X; }
	UINT GetY() { return m_Y; }

	D2D1_RECT_F GetRect() { return D2D1::RectF((FLOAT)m_X, (FLOAT)m_Y, (FLOAT)m_Width, (FLOAT)m_Height); }

	ID2D1Bitmap1* GetBitmap() { return m_Bitmap.Get(); }

private:
	BitmapSegment() = delete;

	UINT m_X;
	UINT m_Y;
	UINT m_Width;
	UINT m_Height;

	Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_Bitmap;
};

struct FileInfo
{
	FileInfo() : m_Path(), m_FileSize(), m_FileTime()
	{}

	std::wstring m_Path;
	DWORD m_FileSize;
	ULONGLONG m_FileTime;

	bool isValid() { return !m_Path.empty() && m_FileSize != 0UL && m_FileTime != 0ULL; }
};

class D2DBitmap
{
public:
	D2DBitmap(const std::wstring& path, int exifOrientation = 0);
	~D2DBitmap();

	UINT GetWidth() const{ return m_Width; }
	UINT GetHeight() const{ return m_Height; }

	void AddSegment(Microsoft::WRL::ComPtr<ID2D1Bitmap1>& bitmap, UINT x, UINT y, UINT width, UINT height);
	void AddSegment(Microsoft::WRL::ComPtr<ID2D1Bitmap1>& bitmap, D2D1_RECT_U& rect);
	void AddSegment(Microsoft::WRL::ComPtr<ID2D1Bitmap1>& bitmap, WICRect& rect);

	void SetSize(UINT width, UINT height) { m_Width = width; m_Height = height; }

	int GetOrientation() { return m_ExifOrientation; }
	void SetOrientation(const int orientation) { m_ExifOrientation = orientation; }

	DWORD GetFileSize() { return m_FileSize; }
	void SetFileSize(const DWORD& fileSize) { m_FileSize = fileSize; }

	ULONGLONG GetFileTime() { return m_FileTime; }
	void SetFileTime(const ULONGLONG& fileTime) { m_FileTime= fileTime; }

	bool HasFileChanged(const std::wstring& file);

	std::wstring& GetPath() { return m_Path; }

	HRESULT Load(const Canvas& canvas);

	Util::D2DEffectStream* CreateEffectStream();
	bool GetPixel(Canvas& canvas, int px, int py, D2D1_COLOR_F& color);

	static HRESULT GetFileInfo(const std::wstring& path, FileInfo* fileInfo);

private:
	friend class Canvas;
	friend class Util::D2DEffectStream;
	friend class Gfx::RenderTexture;

	D2DBitmap();
	D2DBitmap(const D2DBitmap& other) = delete;
	D2DBitmap& operator=(D2DBitmap other) = delete;

	UINT m_Width;
	UINT m_Height;

	int m_ExifOrientation;

	std::wstring m_Path;
	DWORD m_FileSize;
	ULONGLONG m_FileTime;

	std::vector<BitmapSegment> m_Segments;
};

}  // namespace Gfx

#endif
