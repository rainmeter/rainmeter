/* Copyright (C) 2018 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __GENERALIMAGE_H__
#define __GENERALIMAGE_H__

#include "../Common/Gfx/Bitmap.h"
#include "../Common/Gfx/GifImage.h"
#include "../Common/Gfx/Util/EffectStream.h"
#include <string>
#include "Skin.h"
#include "ImageCache.h"
#include "ImageOptions.h"

// Helper macro to define an array of option names. A prefix must be given.
#define GeneralImageHelper_DefineOptionArray(name, prefix) \
	const WCHAR* (name)[GeneralImage::OptionCount] = { \
		prefix  L"ImageCrop", \
		prefix  L"Greyscale", \
		prefix  L"ImageTint", \
		prefix  L"ImageAlpha", \
		prefix  L"ColorMatrix1", \
		prefix  L"ColorMatrix2", \
		prefix  L"ColorMatrix3", \
		prefix  L"ColorMatrix4", \
		prefix  L"ColorMatrix5", \
		prefix  L"ImageFlip", \
		prefix  L"ImageRotate", \
		prefix  L"UseExifOrientation", \
		prefix  L"ImagePath" \
	};

class GeneralImage
{
public:
	enum OptionIndex
	{
		OptionIndexImageCrop = 0,
		OptionIndexGreyscale,
		OptionIndexImageTint,
		OptionIndexImageAlpha,
		OptionIndexColorMatrix1,
		OptionIndexColorMatrix2,
		OptionIndexColorMatrix3,
		OptionIndexColorMatrix4,
		OptionIndexColorMatrix5,
		OptionIndexImageFlip,
		OptionIndexImageRotate,
		OptionIndexUseExifOrientation,
		OptionIndexImagePath,

		OptionCount
	};

	GeneralImage(const WCHAR* name = L"ImageName", const WCHAR** optionArray = c_DefaultOptionArray,
		bool disableTransform = false, Skin* skin = nullptr);
	~GeneralImage();

	void DisposeImage();
	void InvalidateDeviceResources();

	bool IsLoaded();
	Gfx::Bitmap* GetImage();
	bool AdvanceAnimation(ULONGLONG currentTime);

	void ReadOptions(ConfigParser& parser, const WCHAR* section, const WCHAR* imagePath = L"");
	bool LoadImage(const std::wstring& imageName, bool createAlphaMask = false);

private:

	D2D1_SIZE_F ApplyCrop(Gfx::Util::EffectStream* stream, Gfx::Bitmap* bitmap) const;
	void ApplyTransforms();
	bool HasActiveTransforms(Gfx::Bitmap* bitmap) const;
	Gfx::Bitmap* GetSourceBitmap() const;
	Gfx::Bitmap* CreateTransformedBitmap(Gfx::Bitmap* bitmap);

	std::unique_ptr<ImageCacheHandle> m_Bitmap;
	std::unique_ptr<ImageCacheHandle> m_BitmapProcessed;
	std::unique_ptr<Gfx::GifImage> m_GifImage;
	std::unique_ptr<Gfx::Bitmap> m_GifBitmapProcessed;
	ImageOptions m_GifProcessedOptions;
	Skin* m_Skin;

	const WCHAR* m_Name;
	const WCHAR** m_OptionArray;
	const bool m_DisableTransform;

	ImageOptions m_Options;

	std::wstring m_Path;
	std::wstring m_ImageName;

	static bool CompareColorMatrix(const D2D1_MATRIX_5X4_F& a, const D2D1_MATRIX_5X4_F& b);

	static const D2D1_MATRIX_5X4_F c_GreyScaleMatrix;
	static const D2D1_MATRIX_5X4_F c_IdentityMatrix;
	static const WCHAR* c_DefaultOptionArray[OptionCount];
};

#endif
