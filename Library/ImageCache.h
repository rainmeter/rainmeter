/* Copyright (C) 2018 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __IMAGECACHE_H__
#define __IMAGECACHE_H__

#include <unordered_map>
#include <map>
#include <string>
#include <../Common/Gfx/D2DBitmap.h>
#include "ImageOptions.h"

class ImageCachePool;

namespace std {

template <> struct hash<ImageOptions>
{
	std::size_t operator()(const ImageOptions& opt) const noexcept
	{
		size_t res = 17;

		res = res * 31 + std::hash<std::wstring>()(opt.m_Path);
		res = res * 31 + std::hash<DWORD>()(opt.m_FileSize);
		res = res * 31 + std::hash<ULONGLONG>()(opt.m_FileTime);
		res = res * 31 + std::hash<FLOAT>()(opt.m_Rotate);
		res = res * 31 + std::hash<FLOAT>()(opt.m_Crop.left);
		res = res * 31 + std::hash<FLOAT>()(opt.m_Crop.top);
		res = res * 31 + std::hash<FLOAT>()(opt.m_Crop.right);
		res = res * 31 + std::hash<FLOAT>()(opt.m_Crop.bottom);
		res = res * 31 + std::hash<INT>()((INT)opt.m_CropMode);
		res = res * 31 + std::hash<INT>()((INT)opt.m_Flip);
		res = res * 31 + std::hash<bool>()(opt.m_GreyScale);
		res = res * 31 + std::hash<bool>()(opt.m_UseExifOrientation);

		for (int i = 0; i < 5; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				res = res * 31 + std::hash<FLOAT>()(opt.m_ColorMatrix.m[i][j]);
			}
		}

		return res;
	}
};

}  // namespace std

struct ImageCache
{
	ImageCache(const ImageOptions& key, Gfx::D2DBitmap* bitmap, ImageCachePool* pool) :
		m_Key(key),
		m_Bitmap(bitmap),
		m_Pool(pool),
		m_Instances(0U)
	{ }

	~ImageCache()
	{
		delete m_Bitmap;
		m_Bitmap = nullptr;
	}

	void Update(const ImageOptions& key, Gfx::D2DBitmap* item);

	ImageOptions m_Key;
	Gfx::D2DBitmap* m_Bitmap;
	ImageCachePool* m_Pool;
	UINT m_Instances;
};

struct ImageCacheHandle
{
	Gfx::D2DBitmap* GetBitmap() const { return m_Cache->m_Bitmap; }
	ImageOptions& GetKey() const { return m_Cache->m_Key; }

	ImageCacheHandle(ImageCacheHandle&) = delete;
	ImageCacheHandle& operator=(ImageCacheHandle other) = delete;
	~ImageCacheHandle();

private:
	friend class ImageCachePool;

	ImageCacheHandle(ImageCache* cache) : m_Cache(cache)
	{
		++cache->m_Instances;
	}

	ImageCache* m_Cache;
};

class ImageCachePool
{
public:
	static ImageCachePool& GetInstance();

	ImageCacheHandle* Get(const ImageOptions& key);
	void Put(const ImageOptions& key, Gfx::D2DBitmap* item);

private:
	friend struct ImageCacheHandle;

	ImageCachePool();
	~ImageCachePool();
	ImageCachePool(const ImageCachePool& other) = delete;
	ImageCachePool& operator=(ImageCachePool other) = delete;

	void Remove(const ImageOptions& item);

	std::unordered_map<ImageOptions, ImageCache*> m_CachePool;
};

// Convenience function.
inline ImageCachePool& GetImageCache() { return ImageCachePool::GetInstance(); }

#endif
