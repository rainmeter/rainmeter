/* Copyright (C) 2018 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __IMAGEOPTIONS_H__
#define __IMAGEOPTIONS_H__

#include "../Common/Gfx/D2DBitmap.h"
#include "../Common/Gfx/Util/D2DEffectStream.h"

struct ImageOptions : Gfx::FileInfo
{
	ImageOptions() :
		m_ColorMatrix(D2D1::Matrix5x4F(
			-1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f, -1.0f, -1.0f)),
		m_Crop(D2D1::RectF(-1.0f, -1.0f, -1.0f, -1.0f)),
		m_CropMode(CROPMODE_TL),
		m_GreyScale(false),
		m_Rotate(0.0f),
		m_Flip(Gfx::Util::FlipType::None),
		m_UseExifOrientation(false)
	{}

	enum CROPMODE
	{
		CROPMODE_TL = 1,
		CROPMODE_TR,
		CROPMODE_BR,
		CROPMODE_BL,
		CROPMODE_C
	};

	bool operator==(const ImageOptions& other) const
	{
		for (int i = 0; i < 5; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				if (m_ColorMatrix.m[i][j] != other.m_ColorMatrix.m[i][j]) return false;
			}
		}

		return wcscmp(m_Path.c_str(), other.m_Path.c_str()) == 0 &&
			m_FileSize == other.m_FileSize &&
			m_FileTime == other.m_FileTime &&
			m_Rotate == other.m_Rotate &&
			m_GreyScale == other.m_GreyScale &&
			m_UseExifOrientation == other.m_UseExifOrientation &&
			m_Flip == other.m_Flip &&
			m_CropMode == other.m_CropMode &&
			m_Crop.left == other.m_Crop.left &&
			m_Crop.top == other.m_Crop.top &&
			m_Crop.right == other.m_Crop.right &&
			m_Crop.bottom == other.m_Crop.bottom;
	}

	D2D1_MATRIX_5X4_F m_ColorMatrix;
	D2D1_RECT_F m_Crop;
	CROPMODE m_CropMode;
	bool m_GreyScale;
	FLOAT m_Rotate;
	Gfx::Util::FlipType m_Flip;
	bool m_UseExifOrientation;
};

#endif
