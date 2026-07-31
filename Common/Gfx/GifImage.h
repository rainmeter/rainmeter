/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_GFX_GIFIMAGE_H_
#define RM_GFX_GIFIMAGE_H_

#include "Bitmap.h"
#include <memory>
#include <vector>

namespace Gfx {

class GifImage
{
public:
	GifImage();
	~GifImage();

	// Returns S_FALSE when |path| is not a GIF image.
	HRESULT Load(const Canvas& canvas, const std::wstring& path, bool createAlphaMask);
	bool Advance(const Canvas& canvas, ULONGLONG currentTime);
	HRESULT EnsureDeviceResources(const Canvas& canvas);

	Bitmap* GetBitmap() const { return m_Bitmap.get(); }
	UINT GetFrameCount() const { return m_FrameCount; }
	bool IsFinished() const { return m_Finished; }

	void InvalidateDeviceResources();
	bool HasFileChanged(const std::wstring& path) const;

private:
	struct FrameInfo
	{
		UINT left = 0;
		UINT top = 0;
		UINT width = 0;
		UINT height = 0;
		UINT delay = 0;
		UINT disposal = 0;
	};

	GifImage(const GifImage&) = delete;
	GifImage& operator=(const GifImage&) = delete;

	HRESULT ReadGlobalMetadata();
	HRESULT ComposeFrame(const Canvas& canvas, UINT frameIndex, bool startOfLoop);
	HRESULT UpdateBitmap(const Canvas& canvas);
	void DisposeCurrentFrame();
	UINT GetCurrentFrameDelay() const;

	Microsoft::WRL::ComPtr<IWICBitmapDecoder> m_Decoder;
	Microsoft::WRL::ComPtr<IWICStream> m_Stream;
	std::unique_ptr<Bitmap> m_Bitmap;
	std::vector<BYTE> m_FileData;
	std::vector<BYTE> m_Canvas;
	std::vector<BYTE> m_SavedCanvas;
	BYTE m_Background[4] = {};
	FrameInfo m_CurrentFrame;
	UINT m_Width;
	UINT m_Height;
	UINT m_Stride;
	UINT m_FrameCount;
	UINT m_CurrentFrameIndex;
	UINT m_TotalLoopCount;
	UINT m_CompletedLoops;
	ULONGLONG m_NextFrameTime;
	bool m_Finished;
};

}  // namespace Gfx

#endif
