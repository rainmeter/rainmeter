#pragma once
#include "Canvas.h"
#include "Cache.h"

namespace Gfx
{
class BitmapD2D;

enum class FlipDirection
{
	None = 0,
	Horizontal = 1,
	Vertical = 2,
	Both = 3
};

enum CROPMODE
{
	CROPMODE_TL = 1,
	CROPMODE_TR,
	CROPMODE_BR,
	CROPMODE_BL,
	CROPMODE_C
};

class BitmapBase
{
public:
	virtual void Draw(const Gfx::Canvas& canvas, const Gdiplus::Rect& dstRect, const Gdiplus::Rect& srcRect) = 0;

protected:
	BitmapBase();
	virtual ~BitmapBase();

	UINT m_Width;
	UINT m_Height;

private:
	friend class BitmapD2DLoader;
	friend class Deleter<BitmapBase*>;
	friend class BitmapD2D;

	virtual BitmapBase* ApplyDynamicOptions(const Gfx::Canvas& canvas, BitmapD2D* bitmap) = 0;
	virtual HRESULT Tint(const Gfx::Canvas& canvas, const D2D1_MATRIX_5X4_F& matrix, Microsoft::WRL::ComPtr<ID2D1Effect>& m_Image) = 0;
	virtual HRESULT Scale(const Gfx::Canvas& canvas, const D2D1_VECTOR_2F& scale, Microsoft::WRL::ComPtr<ID2D1Effect>& m_Image) = 0;
	virtual HRESULT Crop(const Gfx::Canvas& canvas, const D2D1_RECT_F& crop, CROPMODE cropmode, Microsoft::WRL::ComPtr<ID2D1Effect>& m_Image) = 0;
	virtual HRESULT Transform(const Gfx::Canvas& canvas, const D2D1_MATRIX_3X2_F& transform, Microsoft::WRL::ComPtr<ID2D1Effect>& m_Image) = 0;
	virtual HRESULT Greyscale(const Gfx::Canvas& canvas, Microsoft::WRL::ComPtr<ID2D1Effect>& m_Image) = 0;

};

class BitmapD2D
{
public:
	BitmapD2D();
	virtual ~BitmapD2D();

	void Draw(Gfx::Canvas& canvas, const Gdiplus::Rect& dstRect, const Gdiplus::Rect& srcRect);

	void Load(Gfx::Canvas& canvas);

	std::wstring GetBaseCacheKey();
	std::wstring GetDynamicCacheKey();

	UINT GetWidth();
	UINT GetHeight();

	UINT GetFinalWidth();
	UINT GetFinalHeight();

	bool IsLoaded() const { return m_Loaded; }

	void SetPath(const std::wstring& path);
	void SetCrop(const D2D1_RECT_F& crop, CROPMODE cropmode);
	void SetGreyscale(bool greyscale);
	void SetColorMatrix(Gdiplus::ColorMatrix* matrix);
	void SetFlip(FlipDirection flip);
	void SetRotation(Gdiplus::REAL rotate);
	void UseExifOrientation(bool useExif);

	void ApplyDynamicOptions(Gfx::Canvas& canvas);

private:
	friend class BitmapD2DLoader;
	friend class SingleBitmapD2D;
	friend class MultiBitmapD2D;

	bool m_Loaded = false;
	bool m_BaseDirty = false;
	bool m_FinalDirty = false;

	std::wstring m_Path;
	DWORD m_FileSize;
	ULONGLONG m_FileTime;
	D2D1_RECT_F m_Crop = { -1,-1,-1,-1 };
	CROPMODE m_CropMode;
	bool m_GreyScale;
	D2D1_MATRIX_5X4_F m_ColorMatrix;
	FlipDirection m_Flip;
	Gdiplus::REAL m_Rotate;
	bool m_UseExifOrientation;

	CacheItem<BitmapBase*>* m_BaseImage = nullptr;
	CacheItem<BitmapBase*>* m_FinalImage = nullptr;
};

} // namespace Gfx