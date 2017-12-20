#pragma once
#include "Gfx/Canvas.h"
#include "Gfx/BitmapD2D.h"

namespace Gfx
{

class BitmapLoader
{
public:
	static HRESULT LoadBitmapFromFile(Canvas& canvas, BitmapD2D* bitmap);

private:
	BitmapLoader() = delete;
	~BitmapLoader() = delete;

	static HRESULT CropWICBitmapSource(WICRect& clipRect, IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest);
	static HRESULT ConvertToD2DFormat(IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest);
};

} // namespace Gfx
