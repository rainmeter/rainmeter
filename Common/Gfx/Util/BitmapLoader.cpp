// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "Gfx/Canvas.h"
#include "Gfx/Util/BitmapLoader.h"
#include "Gfx/Bitmap.h"

namespace Gfx {
namespace Util {

HRESULT BitmapLoader::LoadBitmapFromFile(const Canvas& canvas, Bitmap* bitmap)
{
	if (!bitmap) return E_FAIL;

	std::wstring& path = bitmap->GetPath();
	if (path.empty()) return E_FAIL;

	HANDLE fileHandle = CreateFile(
		path.c_str(),
		GENERIC_READ, FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
		nullptr);
	if (fileHandle == INVALID_HANDLE_VALUE) return E_FAIL;

	auto cleanup = [&](HRESULT hr)
	{
		CloseHandle(fileHandle);
		return hr;
	};

	const DWORD fileSize = GetFileSize(fileHandle, nullptr);
	if (fileSize == INVALID_FILE_SIZE)
	{
		return cleanup(E_FAIL);
	}
	bitmap->SetFileSize(fileSize);

	ULONGLONG fileTime = 0;
	if (GetFileTime(fileHandle, nullptr, nullptr, (LPFILETIME)&fileTime) == FALSE)
	{
		return cleanup(E_FAIL);
	}
	bitmap->SetFileTime(fileTime);

	Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
	Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> decoderFrame;
	Microsoft::WRL::ComPtr<IWICBitmapSource> source;

	HRESULT hr = Canvas::c_WICFactory->CreateDecoderFromFileHandle(
		(ULONG_PTR)fileHandle,
		nullptr,
		WICDecodeMetadataCacheOnDemand,
		decoder.GetAddressOf());
	if (SUCCEEDED(hr))
	{
		hr = decoder->GetFrame(0, decoderFrame.GetAddressOf());
		if (SUCCEEDED(hr))
		{
			hr = ConvertToD2DFormat(decoderFrame.Get(), source);
		}
	}
	if (FAILED(hr)) return cleanup(hr);

	const int orientation = GetExifOrientation(decoderFrame.Get());
	bitmap->SetOrientation(orientation);

	UINT width = 0;
	UINT height = 0;
	hr = source->GetSize(&width, &height);
	if (FAILED(hr)) return cleanup(hr);

	if (bitmap->GetCreateAlphaMask())
	{
		std::vector<BYTE> alphaMask;
		CreateAlphaMask(source.Get(), width, height, alphaMask);
		bitmap->SetAlphaMask(alphaMask);
	}

	const auto maxBitmapSize = canvas.m_MaxBitmapSize;
	if (width <= maxBitmapSize && height <= maxBitmapSize)
	{
		Microsoft::WRL::ComPtr<ID2D1Bitmap1> d2dbitmap;
		hr = canvas.m_Target->CreateBitmapFromWicBitmap(
			source.Get(),
			nullptr,
			d2dbitmap.GetAddressOf());
		if (FAILED(hr)) return cleanup(hr);

		bitmap->AddSegment(d2dbitmap, 0, 0, width, height);

		bitmap->SetSize(width, height);
		return cleanup(S_OK);
	}

	for (UINT y = 0, H = (UINT)floor(height / maxBitmapSize); y <= H; ++y)
	{
		for (UINT x = 0, W = (UINT)floor(width / maxBitmapSize); x <= W; ++x)
		{
			WICRect rcClip = {
				(INT)(x * maxBitmapSize),
				(INT)(y * maxBitmapSize),
				(INT)(x == W ? (width - maxBitmapSize * x) : maxBitmapSize),		// If last x coordinate, find cutoff
				(INT)(y == H ? (height - maxBitmapSize * y) : maxBitmapSize) };		// If last y coordinate, find cutoff

			Microsoft::WRL::ComPtr<IWICBitmapSource> bitmapSegment;
			hr = CropWICBitmapSource(rcClip, source.Get(), bitmapSegment);
			if (FAILED(hr)) return cleanup(hr);

			Microsoft::WRL::ComPtr<ID2D1Bitmap1> d2dbitmap;
			hr = canvas.m_Target->CreateBitmapFromWicBitmap(
				bitmapSegment.Get(),
				nullptr,
				d2dbitmap.GetAddressOf());
			if (FAILED(hr)) return cleanup(hr);

			bitmap->AddSegment(d2dbitmap, rcClip);
		}
	}

	bitmap->SetSize(width, height);
	return cleanup(S_OK);
}

HRESULT BitmapLoader::LoadBitmapFromIcon(const Canvas& canvas, Bitmap* bitmap, HICON icon, float scale)
{
	if (!bitmap || !icon || scale <= 0.0f) return E_INVALIDARG;

	Microsoft::WRL::ComPtr<IWICBitmap> source;
	HRESULT hr = Canvas::c_WICFactory->CreateBitmapFromHICON(icon, source.GetAddressOf());
	if (FAILED(hr)) return hr;

	Microsoft::WRL::ComPtr<IWICBitmapSource> convertedSource;
	hr = ConvertToD2DFormat(source.Get(), convertedSource);
	if (FAILED(hr)) return hr;

	UINT width = 0;
	UINT height = 0;
	hr = convertedSource->GetSize(&width, &height);
	if (FAILED(hr)) return hr;

	if (scale != 1.0f)
	{
		UINT scaledWidth = (UINT)roundf(width / scale);
		UINT scaledHeight = (UINT)roundf(height / scale);
		if (scaledWidth == 0) scaledWidth = 1;
		if (scaledHeight == 0) scaledHeight = 1;

		Microsoft::WRL::ComPtr<IWICBitmapScaler> scaler;
		hr = Canvas::c_WICFactory->CreateBitmapScaler(scaler.GetAddressOf());
		if (SUCCEEDED(hr))
		{
			const auto interpolationMode = WICBitmapInterpolationModeHighQualityCubic;
			hr = scaler->Initialize(convertedSource.Get(), scaledWidth, scaledHeight, interpolationMode);
		}
		if (FAILED(hr)) return hr;

		convertedSource = scaler;
		width = scaledWidth;
		height = scaledHeight;
	}

	if (bitmap->GetCreateAlphaMask())
	{
		std::vector<BYTE> alphaMask;
		CreateAlphaMask(convertedSource.Get(), width, height, alphaMask);
		bitmap->SetAlphaMask(alphaMask);
	}

	Microsoft::WRL::ComPtr<ID2D1Bitmap1> d2dbitmap;
	hr = canvas.m_Target->CreateBitmapFromWicBitmap(convertedSource.Get(), nullptr, d2dbitmap.GetAddressOf());
	if (FAILED(hr)) return hr;

	bitmap->AddSegment(d2dbitmap, 0, 0, width, height);
	bitmap->SetSize(width, height);
	return S_OK;
}

bool BitmapLoader::HasFileChanged(Bitmap* bitmap, const std::wstring& file)
{
	if (file.empty() || file != bitmap->GetPath()) return true;

	HANDLE fileHandle = CreateFile(
		file.c_str(),
		GENERIC_READ, FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
		nullptr);
	if (fileHandle == INVALID_HANDLE_VALUE) return true;

	const DWORD fileSize = GetFileSize(fileHandle, nullptr);
	if (fileSize == INVALID_FILE_SIZE || fileSize != bitmap->GetFileSize())
	{
		CloseHandle(fileHandle);
		return true;
	}

	ULONGLONG fileTime = 0;
	BOOL lastWrite = GetFileTime(fileHandle, nullptr, nullptr, (LPFILETIME)&fileTime);
	CloseHandle(fileHandle);

	return lastWrite ? (fileTime != bitmap->GetFileTime()) : true;
}

HRESULT BitmapLoader::GetFileInfo(const std::wstring& path, FileInfo* fileInfo)
{
	if (path.empty()) return E_FAIL;

	HANDLE fileHandle = CreateFile(
		path.c_str(),
		GENERIC_READ, FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
		nullptr);
	if (fileHandle == INVALID_HANDLE_VALUE) return E_FAIL;

	const DWORD fileSize = GetFileSize(fileHandle, nullptr);
	if (fileSize == INVALID_FILE_SIZE)
	{
		CloseHandle(fileHandle);
		return E_FAIL;
	}

	ULONGLONG fileTime = 0;
	BOOL lastWrite = GetFileTime(fileHandle, nullptr, nullptr, (LPFILETIME)&fileTime);
	CloseHandle(fileHandle);

	if (lastWrite)
	{
		fileInfo->m_Path = path;
		fileInfo->m_FileSize = fileSize;
		fileInfo->m_FileTime = fileTime;
		return S_OK;
	}

	return E_FAIL;
}

HRESULT BitmapLoader::CropWICBitmapSource(WICRect& clipRect,
	IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest)
{
	if (clipRect.Width > 0 && clipRect.Height > 0)
	{
		Microsoft::WRL::ComPtr<IWICBitmapClipper> clipper;
		HRESULT hr = Canvas::c_WICFactory->CreateBitmapClipper(clipper.GetAddressOf());
		if (FAILED(hr)) return hr;

		hr = clipper->Initialize(source, &clipRect);
		if (FAILED(hr)) return hr;

		dest.Swap(clipper);
		return S_OK;
	}

	return E_FAIL;
}

HRESULT BitmapLoader::CreateAlphaMask(IWICBitmapSource* source, UINT width, UINT height, std::vector<BYTE>& alphaMask)
{
	alphaMask.clear();
	if (!source || width == 0 || height == 0 || width > (UINT)INT_MAX || height > (UINT)INT_MAX || width > UINT_MAX / 4)
	{
		return E_FAIL;
	}

	const UINT stride = width * 4;
	const UINT64 pixelCount64 = (UINT64)width * height;
	const size_t pixelCount = (size_t)pixelCount64;
	if ((UINT64)pixelCount != pixelCount64)
	{
		return E_FAIL;
	}

	alphaMask.resize(pixelCount);
	std::vector<BYTE> row(stride);
	for (UINT y = 0; y < height; ++y)
	{
		WICRect rect = { 0, (INT)y, (INT)width, 1 };
		HRESULT hr = source->CopyPixels(&rect, stride, stride, row.data());
		if (FAILED(hr))
		{
			alphaMask.clear();
			return hr;
		}

		const size_t dstRow = (size_t)y * width;
		for (UINT x = 0; x < width; ++x)
		{
			alphaMask[dstRow + x] = row[(size_t)x * 4 + 3];
		}
	}

	return S_OK;
}

HRESULT BitmapLoader::ConvertToD2DFormat(
	IWICBitmapSource* source, Microsoft::WRL::ComPtr<IWICBitmapSource>& dest)
{
	// Convert the image format to 32bppPBGRA
	// (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).

	Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
	HRESULT hr = Canvas::c_WICFactory->CreateFormatConverter(converter.GetAddressOf());
	if (FAILED(hr)) return hr;

	hr = converter->Initialize(
		source,
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.0f,
		WICBitmapPaletteTypeMedianCut);
	if (SUCCEEDED(hr))
	{
		dest.Swap(converter);
	}

	return hr;
}

int BitmapLoader::GetExifOrientation(IWICBitmapFrameDecode* source)
{
	Microsoft::WRL::ComPtr<IWICMetadataQueryReader> reader;
	HRESULT hr = source->GetMetadataQueryReader(reader.GetAddressOf());
	if (FAILED(hr)) return 0;

	PROPVARIANT propValue;
	PropVariantInit(&propValue);
	hr = reader->GetMetadataByName(L"/app1/ifd/{ushort=274}", &propValue);
	if (FAILED(hr))
	{
		hr = reader->GetMetadataByName(L"/ifd/{ushort=274}", &propValue);
		if (FAILED(hr)) return 0;
	}

	return propValue.intVal;
}

}  // namespace Util
}  // namespace Gfx
