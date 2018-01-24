/* Copyright (C) 2017 Rainmeter Project Developers
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
	if (fileHandle == INVALID_HANDLE_VALUE) return S_FALSE;

	auto cleanup = [&](HRESULT hr)
	{
		CloseHandle(fileHandle);
		return hr;
	};

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

	int orientation = 0;
	hr = GetExifOrientation(decoderFrame.Get(), &orientation);
	if (FAILED(hr)) return cleanup(hr);

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
		
		BitmapSegment bmp(d2dbitmap, 0, 0, width, height);
		hr = bitmap->AddSegment(bmp);
		if (FAILED(hr)) return cleanup(hr);

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

			BitmapSegment segment(d2dbitmap, rcClip);
			hr = bitmap->AddSegment(segment);
			if (FAILED(hr)) return cleanup(hr);
		}
	}

	bitmap->SetSize(width, height);
	return cleanup(S_OK);
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
	if (FAILED(hr)) return hr;

	dest.Swap(converter);
	return S_OK;
}

HRESULT D2DBitmapLoader::GetExifOrientation(IWICBitmapFrameDecode* source, int* orientation)
{
	Microsoft::WRL::ComPtr<IWICMetadataQueryReader> reader = NULL;
	HRESULT hr = source->GetMetadataQueryReader(reader.GetAddressOf());
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

}  // namespace Util
}  // namespace Gfx
