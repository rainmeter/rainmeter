/* Copyright (C) 2018 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "ImageCache.h"

void ImageCache::Update(const ImageOptions& key, Gfx::D2DBitmap* item)
{
	if (m_Bitmap)
	{
		delete m_Bitmap;
		m_Bitmap = nullptr;
	}

	m_Key = key;
	m_Bitmap = item;
}

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
	return new ImageCacheHandle(find->second);
}

void ImageCachePool::Put(const ImageOptions& key, Gfx::D2DBitmap* item)
{
	if (m_CachePool.find(key) == m_CachePool.end())
	{
		m_CachePool[key] = new ImageCache(key, item, this);
		return;
	}

	// sanity check
	if (item != nullptr)
	{
		m_CachePool[key]->Update(key, item);
	}
}

void ImageCachePool::Remove(const ImageOptions& item)
{
	auto it = m_CachePool.find(item);
	if (it == m_CachePool.end()) return;

	delete it->second;
	m_CachePool.erase(it);
}
