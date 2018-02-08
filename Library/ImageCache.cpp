/* Copyright (C) 2018 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "ImageCache.h"

ImageCacheHandle::~ImageCacheHandle()
{
	--m_Cache->m_Instances;
	if (m_Cache->m_Instances == 0)
	{
		m_Cache->m_Pool->Remove(m_Cache->m_Key);
	}
}

ImageCachePool::ImageCachePool()
{
}

ImageCachePool::~ImageCachePool()
{
}

ImageCachePool& ImageCachePool::GetInstance()
{
	static ImageCachePool s_CachePool;
	return s_CachePool;
}

ImageCacheHandle* ImageCachePool::Get(const ImageOptions& key)
{
	const auto find = m_CachePool.find(key);
	if (find == m_CachePool.end()) return nullptr;
	const auto cleanup = m_CleanupPool.find(key);
	if (cleanup != m_CleanupPool.end())
		m_CleanupPool.erase(cleanup); // reset cleanup counter
	Cleanup();
	return new ImageCacheHandle(find->second);
}

void ImageCachePool::Put(const ImageOptions& key, Gfx::D2DBitmap* item)
{
	if (m_CachePool.find(key) == m_CachePool.end())
	{
		m_CachePool[key] = new ImageCache(key, item, this);
		m_CachePoolWeight[key] += 100;
		return;
	}

	// Already exists in the cache... replace?
}

void ImageCachePool::Cleanup()
{
	for (auto it = m_CleanupPool.begin(); it != m_CleanupPool.end(); ++it)
	{
		--it->second;
		if (it->second <= 0)
		{
			const auto key = it->first;
			auto imageCache = m_CachePool[key];
			m_CachePool.erase(m_CachePool.find(key));
			it = m_CleanupPool.erase(it);
			if (m_CachePoolWeight[key] < 0) m_CachePoolWeight[key] = 0;
			delete imageCache;
			imageCache = nullptr;
		}
	}
}

void ImageCachePool::Remove(const ImageOptions& item)
{
	m_CleanupPool[item] = m_CachePoolWeight[item];
}
