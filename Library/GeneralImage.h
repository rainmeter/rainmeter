// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "../Common/Gfx/Bitmap.h"
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

	void ReadOptions(ConfigParser& parser, const WCHAR* section, const WCHAR* imagePath = L"");
	bool LoadImage(const std::wstring& imageName, bool createAlphaMask = false);

private:

	D2D1_SIZE_F ApplyCrop(Gfx::Util::EffectStream* stream, Gfx::Bitmap* bitmap) const;
	void ApplyTransforms();
	bool HasActiveTransforms(Gfx::Bitmap* bitmap) const;

	std::unique_ptr<ImageCacheHandle> m_Bitmap;
	std::unique_ptr<ImageCacheHandle> m_BitmapProcessed;
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
