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
		m_ColorMatrix(),
		m_Crop(-1, -1, -1, -1),
		m_CropMode(CROPMODE_TL),
		m_GreyScale(false),
		m_Rotate(),
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

		return
			m_Path == other.m_Path &&
			m_FileSize == other.m_FileSize &&
			m_FileTime == other.m_FileTime &&
			m_Rotate == other.m_Rotate &&
			m_GreyScale == other.m_GreyScale &&
			m_UseExifOrientation == other.m_UseExifOrientation &&
			m_Flip == other.m_Flip &&
			m_CropMode == other.m_CropMode &&
			m_Crop.X == other.m_Crop.X &&
			m_Crop.Y == other.m_Crop.Y &&
			m_Crop.Width == other.m_Crop.Width &&
			m_Crop.Height == other.m_Crop.Height;
	}

	D2D1_MATRIX_5X4_F m_ColorMatrix;
	Gdiplus::Rect m_Crop;
	CROPMODE m_CropMode;
	bool m_GreyScale;
	FLOAT m_Rotate;
	Gfx::Util::FlipType m_Flip;
	bool m_UseExifOrientation;
};

#endif