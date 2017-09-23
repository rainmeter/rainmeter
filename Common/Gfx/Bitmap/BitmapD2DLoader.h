#pragma once
#include "Gfx/BitmapD2D.h"
#include "Gfx/Canvas.h"

namespace Gfx
{

	class BitmapD2DLoader
	{
	public:
		static HRESULT LoadBitmapFromFile(Gfx::Canvas& canvas, BitmapD2D* bitmap);

	private:
		friend class Gfx::Canvas;
		BitmapD2DLoader() = delete;
		~BitmapD2DLoader() = delete;

		static HRESULT CropBitmap(WICRect& clipRect, const Gfx::CROPMODE& cropmode, IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest);
		static HRESULT FlipBitmap(const WICBitmapTransformOptions& flip, IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest);
		static HRESULT ApplyExifOrientation(int orientation, IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest);
		static HRESULT GetExifOrientation(Microsoft::WRL::ComPtr<IWICBitmapFrameDecode>& source, int* orientation);
		static HRESULT ConvertToD2DFormat(IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest);
	};

} // namespace Gfx
