#include "stdafx.h"
#include <shlwapi.h>
#include "BitmapD2D.h"
#include "Bitmap/BitmapD2DLoader.h"

namespace Gfx
{

BitmapBase::BitmapBase() : m_Width(0), m_Height(0)
{
}

BitmapBase::~BitmapBase()
{
}

BitmapD2D::BitmapD2D() : m_CropMode(), m_GreyScale(false), m_Flip(), m_Rotate(0), m_UseExifOrientation(false)
{
}


BitmapD2D::~BitmapD2D()
{
	if (m_FinalImage)
	{
		m_FinalImage->RemoveReference();
		m_FinalImage = nullptr;
	}

	if (m_BaseImage)
	{
		m_BaseImage->RemoveReference();
		m_BaseImage = nullptr;
	}
}

void BitmapD2D::Draw(Gfx::Canvas& canvas, const Gdiplus::Rect& dstRect, const Gdiplus::Rect& srcRect)
{
	if (m_BaseImage)
	{
		if (m_FinalDirty)
		{
			ApplyDynamicOptions(canvas);
		}

		if (m_FinalImage)
		{
			m_FinalImage->Get()->Draw(canvas, dstRect, srcRect);
		}
	}

}

void BitmapD2D::Load(Gfx::Canvas& canvas)
{
	if (m_BaseDirty)
	{
		auto key = GetBaseCacheKey();

		if (m_BaseImage)
		{
			if (key.compare(m_BaseImage->GetKey()) == 0)
			{
				return;
			}
		}

		HRESULT hr = BitmapD2DLoader::LoadBitmapFromFile(canvas, this);

		if (hr == S_OK)
		{
			m_BaseDirty = false;
			m_Loaded = true;
			m_FinalDirty = true;
			if (m_FinalImage)
			{
				m_FinalImage->RemoveReference();
				m_FinalImage = nullptr;
			}
		}
		else
		{
			m_Loaded = false;
		}
	}
}

void BitmapD2D::ApplyDynamicOptions(Gfx::Canvas& canvas)
{
	if (m_FinalDirty)
	{
		auto key = GetDynamicCacheKey();

		if (canvas.m_BitmapCache.Contains(key))
		{
			auto res = canvas.m_BitmapCache.Get(key);
			res->AddReference();
			if (m_FinalImage)
			{
				m_FinalImage->RemoveReference();
				m_FinalImage = nullptr;
			}
			m_FinalImage = res;
			m_FinalDirty = false;
			return;
		}

		if (m_BaseImage)
		{
			auto res = m_BaseImage->Get()->ApplyDynamicOptions(canvas, this);
			if (res == nullptr)
			{
				// TODO: Fail
				m_Loaded = false;
				if (m_FinalImage)
				{
					m_FinalImage->RemoveReference();
					m_FinalImage = nullptr;
				}
				return;
			}
			canvas.m_BitmapCache.Put(key, res);
			auto cacheItem = canvas.m_BitmapCache.Get(key);
			cacheItem->AddReference();

			if (m_FinalImage)
			{
				m_FinalImage->RemoveReference();
				m_FinalImage = nullptr;
			}
			m_FinalImage = cacheItem;
			m_FinalDirty = false;
		}
	}
}
	
std::wstring BitmapD2D::GetBaseCacheKey()
{
	std::wstring key;
	WCHAR buffer[MAX_PATH + 50];
	size_t len = _snwprintf_s(buffer, _TRUNCATE, L"%s:%llx:%x:%x:%s", m_Path.c_str(), m_FileTime, m_FileSize, static_cast<byte>(m_Flip), m_UseExifOrientation ? L"E" : L"N");
	key.append(buffer, len);

	return key;
}

std::wstring BitmapD2D::GetDynamicCacheKey()
{
	std::wstring key;
	WCHAR buffer[255];
	size_t len = _snwprintf_s(buffer, _TRUNCATE, L"%s:%.6f:%.6f:%.6f:%.6f:%f:%s", GetBaseCacheKey().c_str(), m_Crop.left, m_Crop.top, m_Crop.right, m_Crop.bottom, m_Rotate, m_GreyScale ? L"G" : L"N");
	key.append(buffer, len);

	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			WCHAR buffer2[10];
			size_t len2 = _snwprintf_s(buffer2, _TRUNCATE, L":%.6f", m_ColorMatrix.m[i][j]);
			key.append(buffer2, len2);
		}
	}

	return key;
}

UINT BitmapD2D::GetWidth()
{
	if (m_FinalImage)
	{
		return m_FinalImage->Get()->m_Width;
	}
	if (m_BaseImage)
	{
		return m_BaseImage->Get()->m_Width;
	}
	return 0;
}

UINT BitmapD2D::GetHeight()
{
	if (m_BaseImage)
	{
		return m_BaseImage->Get()->m_Height;
	}
	return 0;
}

UINT BitmapD2D::GetFinalWidth()
{
	if (m_FinalImage)
	{
		return m_FinalImage->Get()->m_Width;
	}
	return 0;
}

UINT BitmapD2D::GetFinalHeight()
{
	if (m_FinalImage)
	{
		return m_FinalImage->Get()->m_Height;
	}
	return 0;
}

void BitmapD2D::SetPath(const std::wstring& path)
{
	if (path.empty()) return;
	WCHAR buffer[MAX_PATH];
	if (PathCanonicalize(buffer, path.c_str()))
	{
		m_Path = buffer;
		m_BaseDirty = true;
	}
}

void BitmapD2D::SetCrop(const D2D1_RECT_F& crop, CROPMODE cropmode)
{
	m_Crop = crop;
	m_CropMode = cropmode;
	m_FinalDirty = true;
}

void BitmapD2D::SetGreyscale(bool greyscale)
{
	m_GreyScale = greyscale;
	m_BaseDirty = true;
}

void BitmapD2D::SetColorMatrix(Gdiplus::ColorMatrix* matrix)
{
	for (int i = 0; i < 5; ++i)
		for (int j = 0; j < 4; ++j)
		{
			m_ColorMatrix.m[i][j] = matrix->m[i][j];
		}
	m_FinalDirty = true;
}

void BitmapD2D::SetFlip(FlipDirection flip)
{
	m_Flip = flip;
	m_BaseDirty = true;
}

void BitmapD2D::SetRotation(Gdiplus::REAL rotate)
{
	m_Rotate = rotate;
	m_FinalDirty = true;
}

void BitmapD2D::UseExifOrientation(bool useExif)
{
	m_UseExifOrientation = useExif;
	m_BaseDirty = true;
}

} // namespace Gfx