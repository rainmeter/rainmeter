/*
  Copyright (C) 2010 Kimmo Pekkola, MattKing, spx

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef __TINTEDIMAGE_H__
#define __TINTEDIMAGE_H__

#include "Meter.h"
#include "MeterWindow.h"

class CTintedImage
{
public:
	CTintedImage(bool disableTransform = false);
	virtual ~CTintedImage();

	void SetConfigAttributes(const WCHAR* name, const WCHAR* prefix);
	void ReadConfig(CConfigParser& parser, const WCHAR* section);

	bool IsLoaded() { return (m_Bitmap != NULL); }
	bool IsTinted() { return (m_BitmapTint != NULL); }
	bool IsConfigsChanged() { return m_NeedsCrop || m_NeedsTinting || m_NeedsTransform; }
	void ClearConfigFlags() { m_NeedsCrop = m_NeedsTinting = m_NeedsTransform = false; }

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

	static Gdiplus::Bitmap* TurnGreyscale(Gdiplus::Bitmap* source);
	static bool CompareColorMatrix(const Gdiplus::ColorMatrix& a, const Gdiplus::ColorMatrix& b);

	Gdiplus::Bitmap* m_Bitmap;			// The bitmap
	Gdiplus::Bitmap* m_BitmapTint;		// The tinted bitmap

	HGLOBAL m_hBuffer;
	FILETIME m_Modified;

	std::wstring m_ConfigName;
	std::wstring m_ConfigImageCrop;
	std::wstring m_ConfigGreyscale;
	std::wstring m_ConfigImageTint;
	std::wstring m_ConfigImageAlpha;
	std::wstring m_ConfigColorMatrix1;
	std::wstring m_ConfigColorMatrix2;
	std::wstring m_ConfigColorMatrix3;
	std::wstring m_ConfigColorMatrix4;
	std::wstring m_ConfigColorMatrix5;
	std::wstring m_ConfigImageFlip;
	std::wstring m_ConfigImageRotate;

	const bool m_DisableTransform;

	bool m_NeedsCrop;
	bool m_NeedsTinting;
	bool m_NeedsTransform;

	Gdiplus::Rect m_Crop;
	CROPMODE m_CropMode;
	bool m_GreyScale;
	Gdiplus::ColorMatrix m_ColorMatrix;
	Gdiplus::RotateFlipType m_Flip;
	Gdiplus::REAL m_Rotate;

	static const Gdiplus::ColorMatrix c_GreyScaleMatrix;
	static const Gdiplus::ColorMatrix c_IdentifyMatrix;
};

#endif
