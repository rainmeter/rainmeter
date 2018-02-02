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

template <>
struct hash<ImageOptions>
{
	std::size_t operator()(const ImageOptions& k) const
	{
		using std::size_t;
		using std::hash;
		using std::string;

		size_t res = 17;

		res = res * 31 + hash<std::wstring>()(k.m_Path);
		res = res * 31 + hash<DWORD>()(k.m_FileSize);
		res = res * 31 + hash<ULONGLONG>()(k.m_FileTime);
		res = res * 31 + hash<FLOAT>()(k.m_Rotate);
		res = res * 31 + hash<INT>()(k.m_Crop.X);
		res = res * 31 + hash<INT>()(k.m_Crop.Y);
		res = res * 31 + hash<INT>()(k.m_Crop.Width);
		res = res * 31 + hash<INT>()(k.m_Crop.Height);
		res = res * 31 + hash<INT>()((INT)k.m_CropMode);
		res = res * 31 + hash<INT>()((INT)k.m_Flip);
		res = res * 31 + hash<bool>()(k.m_GreyScale);
		res = res * 31 + hash<bool>()(k.m_UseExifOrientation);

		for (int i = 0; i < 5; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				res = res * 31 + hash<FLOAT>()(k.m_ColorMatrix.m[i][j]);
			}
		}

		return res;
	}
};
}

struct ImageCache
{
	ImageCache(const ImageOptions& key, Gfx::D2DBitmap* bitmap, ImageCachePool* pool) : m_Key(key), m_Bitmap(bitmap), m_Pool(pool), m_Instances(0)
	{}

	~ImageCache()
	{
		delete m_Bitmap;
	}

	ImageOptions m_Key;
	Gfx::D2DBitmap* m_Bitmap;
	ImageCachePool* m_Pool;
	int m_Instances;
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
	ImageCacheHandle* Get(const ImageOptions& key);
	void Put(const ImageOptions& key, Gfx::D2DBitmap* item);
	void Cleanup();
	
	static ImageCachePool& GetInstance();

private:
	friend struct ImageCacheHandle;

	void Remove(const ImageOptions& item);

	std::unordered_map<ImageOptions, ImageCache*> m_CachePool;
	std::unordered_map<ImageOptions, int> m_CachePoolWeight;
	std::unordered_map<ImageOptions, int> m_CleanupPool;
};

#endif
