#pragma once
#include "Gfx/BitmapD2D.h"

namespace Gfx
{

class SingleBitmapD2D : public BitmapBase
{
public:
	SingleBitmapD2D(Microsoft::WRL::ComPtr<ID2D1Image>& bitmap);
	virtual ~SingleBitmapD2D();

	void Draw(const Gfx::Canvas& canvas, const Gdiplus::Rect& dstRect, const Gdiplus::Rect& srcRect) override;
private:
	HRESULT Tint(const Gfx::Canvas& canvas, const D2D1_MATRIX_5X4_F& matrix, Microsoft::WRL::ComPtr<ID2D1Effect>& m_Image) override;
	HRESULT Scale(const Gfx::Canvas& canvas, const D2D1_VECTOR_2F& scale, Microsoft::WRL::ComPtr<ID2D1Effect>& m_Image) override;
	HRESULT Crop(const Gfx::Canvas& canvas, const D2D1_RECT_F& crop, CROPMODE cropmode, Microsoft::WRL::ComPtr<ID2D1Effect>& m_Image) override;
	HRESULT Transform(const Gfx::Canvas& canvas, const D2D1_MATRIX_3X2_F& transform, Microsoft::WRL::ComPtr<ID2D1Effect>& m_Image) override;
	HRESULT Greyscale(const Gfx::Canvas& canvas, Microsoft::WRL::ComPtr<ID2D1Effect>& m_Image) override;

	BitmapBase* ApplyDynamicOptions(const Gfx::Canvas& canvas, BitmapD2D* bitmap) override;
	Microsoft::WRL::ComPtr<ID2D1Image> m_Bitmap;

};

} // namespace Gfx