/* Copyright (C) 2017 Rainmeter Project Developers
*
* This Source Code Form is subject to the terms of the GNU General Public
* License; either version 2 of the License, or (at your option) any later
* version. If a copy of the GPL was not distributed with this file, You can
* obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __GENERALIMAGE_H__
#define __GENERALIMAGE_H__

#include "../Common/Gfx/D2DBitmap.h"
#include "../Common/Gfx/Util/D2DEffectStream.h"
#include <string>
#include "Skin.h"

/*
** Helper macro to define an array of option names. A prefix must be given.
**
*/
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

	GeneralImage(const WCHAR* name = L"ImageName", const WCHAR** optionArray = c_DefaultOptionArray, bool disableTransform = false, Skin* skin = nullptr);
	~GeneralImage();

	bool IsLoaded() const { return m_Bitmap != nullptr; }
	Gfx::D2DBitmap* GetImage() { return m_BitmapTinted ? m_BitmapTinted : m_Bitmap; }

	void ReadOptions(ConfigParser& parser, const WCHAR* section, const WCHAR* imagePath = L"");
	bool LoadImage(const std::wstring& imageName);

private:
	enum CROPMODE
	{
		CROPMODE_TL = 1,
		CROPMODE_TR,
		CROPMODE_BR,
		CROPMODE_BL,
		CROPMODE_C
	};

	Gfx::D2DBitmap* m_Bitmap;
	Gfx::D2DBitmap* m_BitmapTinted;
	Skin* m_Skin;

	const WCHAR* m_Name;
	const WCHAR** m_OptionArray;
	const bool m_DisableTransform;

	D2D1_MATRIX_5X4_F m_ColorMatrix;
	Gdiplus::Rect m_Crop;
	CROPMODE m_CropMode;
	bool m_GreyScale;
	FLOAT m_Rotate;
	Gfx::Util::FlipType m_Flip;

	void ApplyCrop(Gfx::Util::D2DEffectStream* stream) const;
	void ApplyTransforms();

	static bool CompareColorMatrix(const D2D1_MATRIX_5X4_F& a, const D2D1_MATRIX_5X4_F& b);

	static const D2D1_MATRIX_5X4_F c_GreyScaleMatrix;
	static const D2D1_MATRIX_5X4_F c_IdentityMatrix;

	static const WCHAR* c_DefaultOptionArray[OptionCount];
};

#endif