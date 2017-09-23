#include "stdafx.h"
#include "BitmapD2DLoader.h"
#include "SingleBitmapD2D.h"

#include <Wincodecsdk.h>

namespace Gfx
{

HRESULT BitmapD2DLoader::LoadBitmapFromFile(Gfx::Canvas& canvas, BitmapD2D* bitmap)
{
	HRESULT hr = S_OK;

	if (!bitmap || bitmap->m_Path.empty())
	{
		return S_FALSE;
	}

	auto key = bitmap->GetBaseCacheKey();
	if (canvas.m_BitmapCache.Contains(key))
	{
		auto cacheitem = canvas.m_BitmapCache.Get(key);
		cacheitem->AddReference();

		if (bitmap->m_BaseImage)
		{
			bitmap->m_BaseImage->RemoveReference();
			bitmap->m_BaseImage = nullptr;
		}
		bitmap->m_BaseImage = cacheitem;
		return S_OK;

	}

	Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder = NULL;
	Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> decoderFrame = NULL;
	Microsoft::WRL::ComPtr<IWICBitmapSource> source = NULL;

	HANDLE fileHandle = CreateFile(bitmap->m_Path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	DWORD fileSize;
	if (fileHandle == INVALID_HANDLE_VALUE || (fileSize = GetFileSize(fileHandle, nullptr)) == INVALID_FILE_SIZE)
	{
		return S_FALSE;
	}

	ULONGLONG fileTime;
	GetFileTime(fileHandle, nullptr, nullptr, (LPFILETIME)&fileTime);

	hr = Canvas::c_WICFactory->CreateDecoderFromFileHandle((ULONG_PTR)fileHandle, NULL, WICDecodeMetadataCacheOnDemand, decoder.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		// TODO: Fail
		return hr;
	}

	hr = decoder->GetFrame(0, decoderFrame.ReleaseAndGetAddressOf());

	if (FAILED(hr))
	{
		// TODO: Fail
		return hr;
	}
	source = decoderFrame;

	UINT width = 0, height = 0;
	hr = decoderFrame->GetSize(&width, &height);
	if (FAILED(hr))
	{
		// TODO: Log Fail
		return hr;
	}

	FLOAT maxBitmapSize = (FLOAT)canvas.m_MaxBitmapSize;

	// TODO: Larger bitmaps
	if (width <= maxBitmapSize && height <= maxBitmapSize)
	{

		hr = ConvertToD2DFormat(decoderFrame.Get(), source);
		if (FAILED(hr))
		{
			// TODO: Log Fail
			return hr;
		}

		int orientation = 0;
		hr = GetExifOrientation(decoderFrame, &orientation);
		if (FAILED(hr))
		{
			// TODO: Log Fail
			return hr;
		}
		if (hr == S_OK)
		{
			hr = ApplyExifOrientation(orientation, source.Get(), source);
			if (FAILED(hr))
			{
				// TODO: Log Fail
				return hr;
			}
		}

		switch (bitmap->m_Flip)
		{
		case FlipDirection::None: break;
		case FlipDirection::Horizontal: hr = FlipBitmap(WICBitmapTransformFlipHorizontal, source.Get(), source); break;
		case FlipDirection::Vertical: hr = FlipBitmap(WICBitmapTransformFlipVertical, source.Get(), source); break;
		case FlipDirection::Both: hr = FlipBitmap(WICBitmapTransformFlipHorizontal, source.Get(), source); FlipBitmap(WICBitmapTransformFlipVertical, source.Get(), source); break;
		default:;
		}

		if (FAILED(hr))
		{
			// TODO: Log Fail
			return hr;
		}
		width = 0, height = 0;
		hr = source->GetSize(&width, &height);
		if (FAILED(hr))
		{
			// TODO: Log Fail
			return hr;
		}
		Microsoft::WRL::ComPtr<ID2D1Bitmap1> theBitmap;
		hr = canvas.m_Target->CreateBitmapFromWicBitmap(source.Get(), NULL, theBitmap.GetAddressOf());
		if (FAILED(hr))
		{
			// TODO: Log Fail
			return hr;
		}

		UINT width2 = 0, height2 = 0;
		hr = source->GetSize(&width2, &height2);
		if (FAILED(hr))
		{
			// TODO: Log Fail
			return hr;
		}

		Microsoft::WRL::ComPtr<ID2D1Image> img = theBitmap;
		Gfx::BitmapBase* bmp = new SingleBitmapD2D(img);

		canvas.m_BitmapCache.Put(key, bmp);
		auto cacheitem = canvas.m_BitmapCache.Get(key);
		cacheitem->AddReference();

		if (bitmap->m_BaseImage && bitmap->m_BaseImage->GetKey().compare(key) != 0)
		{
			bitmap->m_BaseImage->RemoveReference();
			bitmap->m_BaseImage = nullptr;
		}

		bitmap->m_BaseImage = cacheitem;
		bitmap->m_FileSize = fileSize;
		bitmap->m_FileTime = fileTime;

		bmp->m_Width = width2;
		bmp->m_Height = height2;

		CloseHandle(fileHandle);
		return S_OK;
	}
}

HRESULT BitmapD2DLoader::CropBitmap(WICRect& clipRect, const Gfx::CROPMODE& cropmode, IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest)
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

		UINT width = 0, height = 0;
		hr = source->GetSize(&width, &height);
		if (FAILED(hr))
		{
			return hr;
		}

		switch (cropmode)
		{

		case CROPMODE_TR:
			clipRect.X += width;
			break;

		case CROPMODE_BR:
			clipRect.X += width;
			clipRect.Y += height;
			break;

		case CROPMODE_BL:
			clipRect.Y += height;
			break;

		case CROPMODE_C:
			clipRect.X += width / 2;
			clipRect.Y += height / 2;
			break;
		}

		INT rW = clipRect.Width;
		INT rH = clipRect.Height;

		if (clipRect.X + clipRect.Width > width)
		{
			clipRect.Width = width - clipRect.X;
		}

		if (clipRect.Y + clipRect.Height > height)
		{
			clipRect.Height = height - clipRect.Y;
		}

		hr = clipper->Initialize(source, &clipRect);

		clipRect.Width = rW;
		clipRect.Height = rH;

		if (FAILED(hr))
		{
			return hr;
		}

		dest = clipper;
	}
	else
	{
		dest = source;
	}
	return S_OK;
}

HRESULT BitmapD2DLoader::FlipBitmap(const WICBitmapTransformOptions& flip, IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest)
{
	Microsoft::WRL::ComPtr<IWICBitmapFlipRotator> flipper = NULL;
	HRESULT hr = S_OK;
	hr = Canvas::c_WICFactory->CreateBitmapFlipRotator(flipper.GetAddressOf());
	if (FAILED(hr))
	{
		return hr;
	}
	hr = flipper->Initialize(source, flip);

	if (FAILED(hr))
	{
		return hr;
	}

	dest = flipper;
	return S_OK;
}

HRESULT BitmapD2DLoader::ApplyExifOrientation(int orientation, IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest)
{
	HRESULT hr;
	switch (orientation)
	{
	case 2: hr = FlipBitmap(WICBitmapTransformFlipHorizontal, source, dest); break;
	case 3: hr = FlipBitmap(WICBitmapTransformRotate180, source, dest); break;
	case 4: hr = FlipBitmap(WICBitmapTransformFlipVertical, source, dest); break;
	case 5: hr = FlipBitmap(WICBitmapTransformFlipHorizontal, source, dest); FlipBitmap(WICBitmapTransformFlipHorizontal, dest.Get(), dest); break;
	case 6: hr = FlipBitmap(WICBitmapTransformRotate90, source, dest); break;
	case 7: hr = FlipBitmap(WICBitmapTransformFlipHorizontal, source, dest); FlipBitmap(WICBitmapTransformRotate90, dest.Get(), dest); break;
	case 8: hr = FlipBitmap(WICBitmapTransformRotate270, source, dest); break;
	default: hr = S_FALSE; break;
	}

	return hr;
}

HRESULT BitmapD2DLoader::GetExifOrientation(Microsoft::WRL::ComPtr<IWICBitmapFrameDecode>& source, int* orientation)
{
	Microsoft::WRL::ComPtr<IWICMetadataQueryReader> reader = NULL;

	HRESULT hr = S_OK;

	hr = source->GetMetadataQueryReader(reader.GetAddressOf());
	if (FAILED(hr))
	{
		return hr;
	}

	PROPVARIANT propValue;
	PropVariantInit(&propValue);
	hr = reader->GetMetadataByName(L"/app1/ifd/{ushort=274}", &propValue);

	if (FAILED(hr))
	{
		hr = reader->GetMetadataByName(L"/ifd/{ushort=274}", &propValue);
		if (FAILED(hr)) return S_FALSE;
	}

	(*orientation) = propValue.intVal;

	return hr;
}

HRESULT BitmapD2DLoader::ConvertToD2DFormat(IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest)
{
	Microsoft::WRL::ComPtr<IWICFormatConverter> converter = NULL;
	HRESULT hr = S_OK;

	// Convert the image format to 32bppPBGRA
	// (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
	hr = Canvas::c_WICFactory->CreateFormatConverter(converter.GetAddressOf());
	if (FAILED(hr))
	{
		return hr;
	}

	hr = converter->Initialize(source, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);

	if (FAILED(hr))
	{
		return hr;
	}

	dest = converter;

	return hr;
}

} // namespace Gfx