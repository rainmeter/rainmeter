/* Copyright (C) 2018 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Gfx/Canvas.h"
#include "Gfx/Util/D2DBitmapLoader.h"
#include "Gfx/D2DBitmap.h"

namespace Gfx {
namespace Util {

HRESULT D2DBitmapLoader::LoadBitmapFromFile(const Canvas& canvas, D2DBitmap* bitmap)
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

	ULONGLONG fileTime = 0ULL;
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
		hr = decoder->GetFrame(0U, decoderFrame.GetAddressOf());
		if (SUCCEEDED(hr))
		{
			hr = ConvertToD2DFormat(decoderFrame.Get(), source);
		}
	}
	if (FAILED(hr)) return cleanup(hr);

	const int orientation = GetExifOrientation(decoderFrame.Get());
	bitmap->SetOrientation(orientation);

	UINT width = 0U;
	UINT height = 0U;
	hr = source->GetSize(&width, &height);
	if (FAILED(hr)) return cleanup(hr);

	const auto maxBitmapSize = canvas.m_MaxBitmapSize;
	if (width <= maxBitmapSize && height <= maxBitmapSize)
	{
		Microsoft::WRL::ComPtr<ID2D1Bitmap1> d2dbitmap;
		hr = canvas.m_Target->CreateBitmapFromWicBitmap(
			source.Get(),
			nullptr,
			d2dbitmap.GetAddressOf());
		if (FAILED(hr)) return cleanup(hr);

		const BitmapSegment bmp(d2dbitmap, 0U, 0U, width, height);
		bitmap->AddSegment(bmp);

		bitmap->SetSize(width, height);
		return cleanup(S_OK);
	}

	for (UINT y = 0U, H = (UINT)floor(height / maxBitmapSize); y <= H; ++y)
	{
		for (UINT x = 0U, W = (UINT)floor(width / maxBitmapSize); x <= W; ++x)
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

			const BitmapSegment segment(d2dbitmap, rcClip);
			bitmap->AddSegment(segment);
		}
	}

	bitmap->SetSize(width, height);
	return cleanup(S_OK);
}

bool D2DBitmapLoader::HasFileChanged(D2DBitmap* bitmap, const std::wstring& file)
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

	ULONGLONG fileTime = 0ULL;
	BOOL lastWrite = GetFileTime(fileHandle, nullptr, nullptr, (LPFILETIME)&fileTime);
	CloseHandle(fileHandle);

	return lastWrite ? (fileTime != bitmap->GetFileTime()) : true;
}

HRESULT D2DBitmapLoader::GetFileInfo(const std::wstring& path, FileInfo* fileInfo)
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

	ULONGLONG fileTime = 0ULL;
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

HRESULT D2DBitmapLoader::CropWICBitmapSource(WICRect& clipRect,
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

HRESULT D2DBitmapLoader::ConvertToD2DFormat(
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

int D2DBitmapLoader::GetExifOrientation(IWICBitmapFrameDecode* source)
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
