/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "GifImage.h"
#include "Util/BitmapLoader.h"

namespace Gfx {

namespace {

constexpr UINT c_MinimumFrameDelay = 20;

UINT ReadUnsignedMetadata(IWICMetadataQueryReader* reader, const WCHAR* name, UINT defaultValue)
{
	PROPVARIANT value;
	PropVariantInit(&value);
	if (FAILED(reader->GetMetadataByName(name, &value)))
	{
		PropVariantClear(&value);
		return defaultValue;
	}

	UINT result = defaultValue;
	switch (value.vt)
	{
	case VT_UI1: result = value.bVal; break;
	case VT_UI2: result = value.uiVal; break;
	case VT_UI4: result = value.ulVal; break;
	}
	PropVariantClear(&value);
	return result;
}

}  // namespace

GifImage::GifImage() :
	m_CurrentFrame(),
	m_Width(0),
	m_Height(0),
	m_Stride(0),
	m_FrameCount(0),
	m_CurrentFrameIndex(0),
	m_TotalLoopCount(0),
	m_CompletedLoops(0),
	m_NextFrameTime(0),
	m_Finished(false)
{
}

GifImage::~GifImage() = default;

HRESULT GifImage::Load(const Canvas& canvas, const std::wstring& path, bool createAlphaMask)
{
	FileInfo fileInfo;
	HRESULT hr = Bitmap::GetFileInfo(path, &fileInfo);
	if (FAILED(hr) || !fileInfo.isValid()) return FAILED(hr) ? hr : E_FAIL;

	HANDLE file = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	if (file == INVALID_HANDLE_VALUE) return HRESULT_FROM_WIN32(GetLastError());

	m_FileData.resize(fileInfo.m_FileSize);
	DWORD bytesRead = 0;
	const BOOL readResult = ReadFile(file, m_FileData.data(), fileInfo.m_FileSize, &bytesRead, nullptr);
	CloseHandle(file);
	if (!readResult || bytesRead != fileInfo.m_FileSize) return E_FAIL;

	Microsoft::WRL::ComPtr<IWICStream> stream;
	hr = Canvas::c_WICFactory->CreateStream(stream.GetAddressOf());
	if (FAILED(hr)) return hr;
	hr = stream->InitializeFromMemory(m_FileData.data(), (DWORD)m_FileData.size());
	if (FAILED(hr)) return hr;

	Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
	hr = Canvas::c_WICFactory->CreateDecoderFromStream(
		stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf());
	if (FAILED(hr)) return hr;

	GUID containerFormat = {};
	hr = decoder->GetContainerFormat(&containerFormat);
	if (FAILED(hr)) return hr;
	if (containerFormat != GUID_ContainerFormatGif)
	{
		m_FileData.clear();
		return S_FALSE;
	}

	m_Decoder = std::move(decoder);
	m_Stream = std::move(stream);
	m_Bitmap = std::make_unique<Bitmap>(path, 0, createAlphaMask);
	m_Bitmap->SetFileSize(fileInfo.m_FileSize);
	m_Bitmap->SetFileTime(fileInfo.m_FileTime);

	hr = ReadGlobalMetadata();
	if (FAILED(hr)) return hr;

	m_Canvas.resize((size_t)m_Stride * m_Height);
	m_SavedCanvas.clear();
	m_CurrentFrame = {};
	m_CurrentFrameIndex = 0;
	m_CompletedLoops = 0;
	m_Finished = false;

	hr = ComposeFrame(canvas, 0, true);
	if (FAILED(hr)) return hr;

	while (m_CurrentFrame.delay == 0 && m_CurrentFrameIndex + 1 < m_FrameCount)
	{
		hr = ComposeFrame(canvas, m_CurrentFrameIndex + 1, false);
		if (FAILED(hr)) return hr;
	}

	m_NextFrameTime = GetTickCount64() + GetCurrentFrameDelay();
	return S_OK;
}

HRESULT GifImage::ReadGlobalMetadata()
{
	HRESULT hr = m_Decoder->GetFrameCount(&m_FrameCount);
	if (FAILED(hr) || m_FrameCount == 0) return FAILED(hr) ? hr : E_FAIL;

	Microsoft::WRL::ComPtr<IWICMetadataQueryReader> reader;
	hr = m_Decoder->GetMetadataQueryReader(reader.GetAddressOf());
	if (FAILED(hr)) return hr;

	m_Width = ReadUnsignedMetadata(reader.Get(), L"/logscrdesc/Width", 0);
	m_Height = ReadUnsignedMetadata(reader.Get(), L"/logscrdesc/Height", 0);
	if (m_Width == 0 || m_Height == 0 || m_Width > UINT_MAX / 4) return E_FAIL;
	m_Stride = m_Width * 4;
	if ((UINT64)m_Stride * m_Height > UINT_MAX) return E_FAIL;

	memset(m_Background, 0, sizeof(m_Background));
	PROPVARIANT globalColorTable;
	PropVariantInit(&globalColorTable);
	const bool hasGlobalColorTable = SUCCEEDED(reader->GetMetadataByName(
		L"/logscrdesc/GlobalColorTableFlag", &globalColorTable)) &&
		globalColorTable.vt == VT_BOOL && globalColorTable.boolVal;
	PropVariantClear(&globalColorTable);

	if (hasGlobalColorTable)
	{
		const UINT backgroundIndex = ReadUnsignedMetadata(
			reader.Get(), L"/logscrdesc/BackgroundColorIndex", UINT_MAX);
		Microsoft::WRL::ComPtr<IWICPalette> palette;
		if (backgroundIndex != UINT_MAX && SUCCEEDED(Canvas::c_WICFactory->CreatePalette(palette.GetAddressOf())) &&
			SUCCEEDED(m_Decoder->CopyPalette(palette.Get())))
		{
			WICColor colors[256] = {};
			UINT colorCount = 0;
			if (SUCCEEDED(palette->GetColors(ARRAYSIZE(colors), colors, &colorCount)) && backgroundIndex < colorCount)
			{
				const WICColor color = colors[backgroundIndex];
				const UINT alpha = color >> 24;
				m_Background[0] = (BYTE)((((color >> 0) & 0xFF) * alpha + 127) / 255);
				m_Background[1] = (BYTE)((((color >> 8) & 0xFF) * alpha + 127) / 255);
				m_Background[2] = (BYTE)((((color >> 16) & 0xFF) * alpha + 127) / 255);
				m_Background[3] = (BYTE)alpha;
			}
		}
	}

	m_TotalLoopCount = 0;
	PROPVARIANT application;
	PropVariantInit(&application);
	if (SUCCEEDED(reader->GetMetadataByName(L"/appext/application", &application)) &&
		application.vt == (VT_UI1 | VT_VECTOR) && application.caub.cElems == 11 &&
		(!memcmp(application.caub.pElems, "NETSCAPE2.0", 11) ||
			!memcmp(application.caub.pElems, "ANIMEXTS1.0", 11)))
	{
		PropVariantClear(&application);
		PROPVARIANT data;
		PropVariantInit(&data);
		if (SUCCEEDED(reader->GetMetadataByName(L"/appext/data", &data)) &&
			data.vt == (VT_UI1 | VT_VECTOR) && data.caub.cElems >= 4 &&
			data.caub.pElems[0] > 0 && data.caub.pElems[1] == 1)
		{
			m_TotalLoopCount = MAKEWORD(data.caub.pElems[2], data.caub.pElems[3]);
		}
		PropVariantClear(&data);
	}
	else
	{
		PropVariantClear(&application);
	}

	return S_OK;
}

void GifImage::DisposeCurrentFrame()
{
	if (m_CurrentFrame.disposal == 2)
	{
		for (UINT y = m_CurrentFrame.top; y < m_CurrentFrame.top + m_CurrentFrame.height; ++y)
		{
			BYTE* pixel = m_Canvas.data() + (size_t)y * m_Stride + (size_t)m_CurrentFrame.left * 4;
			for (UINT x = 0; x < m_CurrentFrame.width; ++x, pixel += 4)
			{
				memcpy(pixel, m_Background, sizeof(m_Background));
			}
		}
	}
	else if (m_CurrentFrame.disposal == 3 && m_SavedCanvas.size() == m_Canvas.size())
	{
		m_Canvas = m_SavedCanvas;
	}
}

HRESULT GifImage::ComposeFrame(const Canvas& canvas, UINT frameIndex, bool startOfLoop)
{
	if (startOfLoop)
	{
		for (size_t i = 0; i < m_Canvas.size(); i += 4)
		{
			memcpy(m_Canvas.data() + i, m_Background, sizeof(m_Background));
		}
	}
	else
	{
		DisposeCurrentFrame();
	}

	Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
	HRESULT hr = m_Decoder->GetFrame(frameIndex, frame.GetAddressOf());
	if (FAILED(hr)) return hr;

	FrameInfo info;
	hr = frame->GetSize(&info.width, &info.height);
	if (FAILED(hr)) return hr;

	Microsoft::WRL::ComPtr<IWICMetadataQueryReader> reader;
	hr = frame->GetMetadataQueryReader(reader.GetAddressOf());
	if (FAILED(hr)) return hr;
	info.left = ReadUnsignedMetadata(reader.Get(), L"/imgdesc/Left", 0);
	info.top = ReadUnsignedMetadata(reader.Get(), L"/imgdesc/Top", 0);
	info.width = ReadUnsignedMetadata(reader.Get(), L"/imgdesc/Width", info.width);
	info.height = ReadUnsignedMetadata(reader.Get(), L"/imgdesc/Height", info.height);
	const UINT delay = ReadUnsignedMetadata(reader.Get(), L"/grctlext/Delay", 0);
	info.delay = delay > UINT_MAX / 10 ? UINT_MAX : delay * 10;
	info.disposal = ReadUnsignedMetadata(reader.Get(), L"/grctlext/Disposal", 0);

	if (info.width == 0 || info.height == 0 || info.left > m_Width || info.top > m_Height ||
		info.width > m_Width - info.left || info.height > m_Height - info.top || info.width > UINT_MAX / 4)
	{
		return E_FAIL;
	}

	if (info.disposal == 3)
	{
		m_SavedCanvas = m_Canvas;
	}
	else
	{
		m_SavedCanvas.clear();
	}

	Microsoft::WRL::ComPtr<IWICBitmapSource> source;
	hr = Util::BitmapLoader::ConvertToD2DFormat(frame.Get(), source);
	if (FAILED(hr)) return hr;

	const UINT frameStride = info.width * 4;
	const UINT64 frameSize = (UINT64)frameStride * info.height;
	if (frameSize > UINT_MAX) return E_FAIL;
	std::vector<BYTE> pixels((size_t)frameSize);
	hr = source->CopyPixels(nullptr, frameStride, (UINT)frameSize, pixels.data());
	if (FAILED(hr)) return hr;

	for (UINT y = 0; y < info.height; ++y)
	{
		BYTE* destination = m_Canvas.data() + (size_t)(info.top + y) * m_Stride + (size_t)info.left * 4;
		const BYTE* sourcePixel = pixels.data() + (size_t)y * frameStride;
		for (UINT x = 0; x < info.width; ++x, destination += 4, sourcePixel += 4)
		{
			const UINT inverseAlpha = 255 - sourcePixel[3];
			destination[0] = (BYTE)(sourcePixel[0] + (destination[0] * inverseAlpha + 127) / 255);
			destination[1] = (BYTE)(sourcePixel[1] + (destination[1] * inverseAlpha + 127) / 255);
			destination[2] = (BYTE)(sourcePixel[2] + (destination[2] * inverseAlpha + 127) / 255);
			destination[3] = (BYTE)(sourcePixel[3] + (destination[3] * inverseAlpha + 127) / 255);
		}
	}

	m_CurrentFrame = info;
	m_CurrentFrameIndex = frameIndex;
	return UpdateBitmap(canvas);
}

HRESULT GifImage::UpdateBitmap(const Canvas& canvas)
{
	Microsoft::WRL::ComPtr<IWICBitmap> source;
	HRESULT hr = Canvas::c_WICFactory->CreateBitmapFromMemory(
		m_Width, m_Height, GUID_WICPixelFormat32bppPBGRA, m_Stride,
		(UINT)m_Canvas.size(), m_Canvas.data(), source.GetAddressOf());
	if (FAILED(hr)) return hr;
	return Util::BitmapLoader::LoadBitmapFromWicSource(canvas, m_Bitmap.get(), source.Get());
}

UINT GifImage::GetCurrentFrameDelay() const
{
	if (m_CurrentFrame.delay == 0 && m_CurrentFrameIndex + 1 < m_FrameCount) return 0;
	return max(m_CurrentFrame.delay, c_MinimumFrameDelay);
}

bool GifImage::Advance(const Canvas& canvas, ULONGLONG currentTime)
{
	if (m_FrameCount < 2 || m_Finished || currentTime < m_NextFrameTime) return false;

	bool changed = false;
	UINT framesComposed = 0;
	while (!m_Finished && currentTime >= m_NextFrameTime && framesComposed < m_FrameCount)
	{
		UINT nextFrame = m_CurrentFrameIndex + 1;
		bool startOfLoop = false;
		if (nextFrame >= m_FrameCount)
		{
			++m_CompletedLoops;
			if (m_TotalLoopCount != 0 && m_CompletedLoops >= m_TotalLoopCount)
			{
				m_Finished = true;
				break;
			}
			nextFrame = 0;
			startOfLoop = true;
		}

		if (FAILED(ComposeFrame(canvas, nextFrame, startOfLoop)))
		{
			m_Finished = true;
			break;
		}
		changed = true;
		++framesComposed;

		const UINT delay = GetCurrentFrameDelay();
		m_NextFrameTime += delay;
	}

	// Do not replay an unbounded number of missed loops after a long pause.
	if (!m_Finished && currentTime >= m_NextFrameTime)
	{
		m_NextFrameTime = currentTime + GetCurrentFrameDelay();
	}

	return changed;
}

HRESULT GifImage::EnsureDeviceResources(const Canvas& canvas)
{
	return m_Bitmap && !m_Bitmap->HasDeviceResources() ? UpdateBitmap(canvas) : S_OK;
}

void GifImage::InvalidateDeviceResources()
{
	if (m_Bitmap) m_Bitmap->InvalidateDeviceResources();
}

bool GifImage::HasFileChanged(const std::wstring& path) const
{
	return !m_Bitmap || m_Bitmap->HasFileChanged(path);
}

}  // namespace Gfx
