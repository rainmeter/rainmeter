/* Copyright (C) 2018 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __IMAGECACHE_H__
#define __IMAGECACHE_H__

#include <bit>
#include <string>
#include <../Common/Gfx/D2DBitmap.h>
#include "ImageOptions.h"

class ImageCachePool;

template <>
struct ankerl::unordered_dense::hash<ImageOptions>
{
	using is_avalanching = void;

	[[nodiscard]] auto operator()(const ImageOptions& opt) const noexcept -> uint64_t
	{
		const auto normalizeFloatBits = [](FLOAT value) -> uint32_t
		{
			// Handle positive and negative floating point zeros.
			return value == 0.0f ? 0U : std::bit_cast<uint32_t>(value);
		};

		const auto mix64 = [](uint64_t state, uint64_t value) -> uint64_t
		{
			return ankerl::unordered_dense::detail::wyhash::mix(state, value);
		};

		const auto mixFloatPair = [&](uint64_t state, FLOAT first, FLOAT second) -> uint64_t
		{
			return mix64(state, static_cast<uint64_t>(normalizeFloatBits(first)) | (static_cast<uint64_t>(normalizeFloatBits(second)) << 32U));
		};

		const auto packSmallFields = [](uint32_t rotate, uint8_t cropMode, uint8_t flip, bool greyScale, bool useExifOrientation, bool createAlphaMask) -> uint64_t
		{
			return (static_cast<uint64_t>(rotate) << 32U) |
				static_cast<uint64_t>(cropMode) |
				(static_cast<uint64_t>(flip) << 8U) |
				(static_cast<uint64_t>(greyScale) << 16U) |
				(static_cast<uint64_t>(useExifOrientation) << 17U) |
				(static_cast<uint64_t>(createAlphaMask) << 18U);
		};

		uint64_t hash = ankerl::unordered_dense::detail::wyhash::hash(opt.m_Path.data(), opt.m_Path.size() * sizeof(WCHAR));
		hash = mix64(hash, opt.m_FileSize);
		hash = mix64(hash, opt.m_FileTime);
		hash = mixFloatPair(hash, opt.m_Crop.left, opt.m_Crop.top);
		hash = mixFloatPair(hash, opt.m_Crop.right, opt.m_Crop.bottom);
		hash = mix64(hash, packSmallFields(normalizeFloatBits(opt.m_Rotate), opt.m_CropMode, (uint8_t)opt.m_Flip, opt.m_GreyScale, opt.m_UseExifOrientation, opt.m_CreateAlphaMask));

		for (int i = 0; i < 5; ++i)
		{
			for (int j = 0; j < 4; j += 2)
			{
				hash = mixFloatPair(hash, opt.m_ColorMatrix.m[i][j], opt.m_ColorMatrix.m[i][j + 1]);
			}
		}

		return hash;
	}
};

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
private:
	struct ConstructorToken {};

public:
	Gfx::D2DBitmap* GetBitmap() const { return m_Cache->m_Bitmap; }
	ImageOptions& GetKey() const { return m_Cache->m_Key; }

	ImageCacheHandle(const ImageCacheHandle&) = delete;
	ImageCacheHandle& operator=(const ImageCacheHandle&) = delete;
	ImageCacheHandle(ConstructorToken, ImageCache* cache) : m_Cache(cache)
	{
		++cache->m_Instances;
	}
	~ImageCacheHandle();

private:
	friend class ImageCachePool;

	ImageCache* m_Cache;
};

class ImageCachePool
{
public:
	static ImageCachePool& GetInstance();

	std::unique_ptr<ImageCacheHandle> Get(const ImageOptions& key);
	void Put(const ImageOptions& key, Gfx::D2DBitmap* item);

	void InvalidateDeviceResources();

private:
	friend struct ImageCacheHandle;

	ImageCachePool();
	~ImageCachePool();
	ImageCachePool(const ImageCachePool& other) = delete;
	ImageCachePool& operator=(ImageCachePool other) = delete;

	void Remove(const ImageOptions& item);

	ankerl::unordered_dense::map<ImageOptions, ImageCache*> m_CachePool;
};

// Convenience function.
inline ImageCachePool& GetImageCache() { return ImageCachePool::GetInstance(); }

#endif
