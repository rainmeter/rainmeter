/* Copyright (C) 2010 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __TINTEDIMAGE_H__
#define __TINTEDIMAGE_H__

#include <windows.h>
#include <ole2.h>  // For Gdiplus.h.
#include <gdiplus.h>
#include <string>
#include "Skin.h"

/*
** Helper macro to define an array of option names. A prefix must be given.
**
*/
#define TintedImageHelper_DefineOptionArray(name, prefix) \
	const WCHAR* (name)[TintedImage::OptionCount] = { \
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

class ConfigParser;

class TintedImage
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

	TintedImage(const WCHAR* name = L"ImageName", const WCHAR** optionArray = c_DefaultOptionArray, bool disableTransform = false, Skin* skin = nullptr);
	~TintedImage();

	TintedImage(const TintedImage& other) = delete;
	TintedImage& operator=(TintedImage other) = delete;

	void ReadOptions(ConfigParser& parser, const WCHAR* section, const WCHAR* imagePath = L"");

	bool IsLoaded() { return (m_Bitmap != nullptr); }
	bool IsTinted() { return (m_BitmapTint != nullptr); }
	bool IsOptionsChanged() { return m_NeedsCrop || m_NeedsTinting || m_NeedsTransform || m_HasPathChanged; }
	void ClearOptionFlags() { m_NeedsCrop = m_NeedsTinting = m_NeedsTransform = m_HasPathChanged = false; }

	Gdiplus::Bitmap* GetOriginalImage() { return m_Bitmap; }
	Gdiplus::Bitmap* GetTintedImage() { return m_BitmapTint; }
	Gdiplus::Bitmap* GetImage() { return (m_BitmapTint) ? m_BitmapTint : m_Bitmap; }

	void DisposeImage();
	void LoadImage(const std::wstring& imageName, bool bLoadAlways);

protected:
	enum CROPMODE
	{
		CROPMODE_TL = 1,
		CROPMODE_TR,
		CROPMODE_BR,
		CROPMODE_BL,
		CROPMODE_C
	};

	void ApplyCrop();
	void ApplyTint();
	void ApplyTransform();

	Gdiplus::Bitmap* LoadImageFromFileHandle(HANDLE fileHandle, DWORD fileSize, HGLOBAL* phBuffer);

	static Gdiplus::Bitmap* TurnGreyscale(Gdiplus::Bitmap* source);
	static bool CompareColorMatrix(const Gdiplus::ColorMatrix* a, const Gdiplus::ColorMatrix* b);

	Gdiplus::Bitmap* m_Bitmap;
	Gdiplus::Bitmap* m_BitmapTint;		// Tinted bitmap

	const WCHAR* m_Name;
	const WCHAR** m_OptionArray;
	const bool m_DisableTransform;

	bool m_NeedsCrop;
	bool m_NeedsTinting;
	bool m_NeedsTransform;

	Gdiplus::Rect m_Crop;
	CROPMODE m_CropMode;
	bool m_GreyScale;
	Gdiplus::ColorMatrix* m_ColorMatrix;
	Gdiplus::RotateFlipType m_Flip;
	Gdiplus::REAL m_Rotate;
	bool m_UseExifOrientation;

	std::wstring m_Path;
	bool m_HasPathChanged;

	std::wstring m_CacheKey;

	Skin* m_Skin;

	static const Gdiplus::ColorMatrix c_GreyScaleMatrix;
	static const Gdiplus::ColorMatrix c_IdentityMatrix;

	static const WCHAR* c_DefaultOptionArray[OptionCount];
};

#endif
