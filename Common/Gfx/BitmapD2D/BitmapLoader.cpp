#include "StdAfx.h"
#include "BitmapLoader.h"

namespace Gfx
{

HRESULT BitmapLoader::LoadBitmapFromFile(Canvas& canvas, BitmapD2D* bitmap)
{
	if (!bitmap || bitmap->m_Path.empty()) return E_FAIL;
	HRESULT hr = S_OK;

	Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder = NULL;
	Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> decoderFrame = NULL;

	HANDLE fileHandle = CreateFile(bitmap->m_Path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	if (fileHandle == INVALID_HANDLE_VALUE) return S_FALSE;

	hr = Canvas::c_WICFactory->CreateDecoderFromFileHandle((ULONG_PTR)fileHandle, NULL, WICDecodeMetadataCacheOnDemand, decoder.ReleaseAndGetAddressOf());
	if (FAILED(hr)) return hr;

	hr = decoder->GetFrame(0, decoderFrame.ReleaseAndGetAddressOf());
	if (FAILED(hr)) return hr;

	Microsoft::WRL::ComPtr<IWICBitmapSource> source = decoderFrame;
	hr = ConvertToD2DFormat(decoderFrame.Get(), source);
	if (FAILED(hr)) return hr;

	UINT width = 0, height = 0;
	hr = source->GetSize(&width, &height);
	if (FAILED(hr)) return hr;
	FLOAT maxBitmapSize = (FLOAT)canvas.m_MaxBitmapSize;

	if (width <= maxBitmapSize && height <= maxBitmapSize)
	{
		Microsoft::WRL::ComPtr<ID2D1Bitmap1> d2dbitmap;
		hr = canvas.m_Target->CreateBitmapFromWicBitmap(source.Get(), NULL, d2dbitmap.GetAddressOf());
		if (FAILED(hr)) return hr;

		BitmapSegment bmp({ d2dbitmap, 0, 0, width, height });
		bmp.m_Width = width;
		bmp.m_Height = height;

		const BitmapHandle handle({bmp}, width, height);
		
		bitmap->m_Bitmap = handle;

		CloseHandle(fileHandle);
		return S_OK;
	}

	std::vector<BitmapSegment> segments;
	for (int y = 0; y < ceil(height / maxBitmapSize); ++y)
		for (int x = 0; x < ceil(width / maxBitmapSize); ++x)
		{
			WICRect rcClip = { x * maxBitmapSize, y * maxBitmapSize, maxBitmapSize, maxBitmapSize };

			// if last x, find cutoff point
			if (x == ceil(width / maxBitmapSize) - 1)
			{
				rcClip.Width = round(width - maxBitmapSize * x);
			}

			// if last y, find cutoff point
			if (y == ceil(height / maxBitmapSize) - 1)
			{
				rcClip.Height = round(height - maxBitmapSize * y);
			}

			Microsoft::WRL::ComPtr<IWICBitmapSource> bitmapSegment = NULL;

			hr = CropWICBitmapSource(rcClip, source.Get(), bitmapSegment);
			if (FAILED(hr)) return hr;

			Microsoft::WRL::ComPtr<ID2D1Bitmap1> d2dbitmap;
			hr = canvas.m_Target->CreateBitmapFromWicBitmap(bitmapSegment.Get(), NULL, d2dbitmap.GetAddressOf());
			if (FAILED(hr)) return hr;

			segments.emplace_back(d2dbitmap, rcClip.X, rcClip.Y, rcClip.Width, rcClip.Height);
		}

	const BitmapHandle bitmapHandle(segments, width, height);
	bitmap->m_Bitmap = bitmapHandle;

	CloseHandle(fileHandle);
	return hr;
}

HRESULT BitmapLoader::CropWICBitmapSource(WICRect& clipRect, IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest)
{
	if (clipRect.Width > 0 && clipRect.Height > 0)
	{
		Microsoft::WRL::ComPtr<IWICBitmapClipper> clipper = NULL;
		HRESULT hr = S_OK;
		hr = Canvas::c_WICFactory->CreateBitmapClipper(clipper.GetAddressOf());
		if (FAILED(hr))
		{
			return hr;
		}

		hr = clipper->Initialize(source, &clipRect);
		if (FAILED(hr))
		{
			return hr;
		}

		dest = clipper;
	}
	else
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT BitmapLoader::ConvertToD2DFormat(IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest)
{
	Microsoft::WRL::ComPtr<IWICFormatConverter> converter = NULL;
	HRESULT hr = S_OK;

	// Convert the image format to 32bppPBGRA
	// (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
	hr = Canvas::c_WICFactory->CreateFormatConverter(converter.GetAddressOf());
	if (FAILED(hr)) return hr;

	hr = converter->Initialize(source, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
	if (FAILED(hr)) return hr;

	dest = converter;
	return hr;
}

} // namespace Gfx
