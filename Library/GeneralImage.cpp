/* Copyright (C) 2017 Rainmeter Project Developers
*
* This Source Code Form is subject to the terms of the GNU General Public
* License; either version 2 of the License, or (at your option) any later
* version. If a copy of the GPL was not distributed with this file, You can
* obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "GeneralImage.h"
#include "../Common/Gfx/Util/D2DEffectStream.h"
#include "Logger.h"

using namespace Gdiplus;

// GrayScale Matrix
const D2D1_MATRIX_5X4_F GeneralImage::c_GreyScaleMatrix = {
	0.299f, 0.299f, 0.299f, 0.0f,
	0.587f, 0.587f, 0.587f, 0.0f,
	0.114f, 0.114f, 0.114f, 0.0f,
	0.0f,   0.0f,   0.0f, 1.0f,
	0.0f,   0.0f,   0.0f, 0.0f
};


const D2D1_MATRIX_5X4_F GeneralImage::c_IdentityMatrix = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 0.0f, 0.0f
};

GeneralImageHelper_DefineOptionArray(GeneralImage::c_DefaultOptionArray, L"");

GeneralImage::GeneralImage(const WCHAR* name, const WCHAR** optionArray, bool disableTransform, Skin* skin) :
	m_Bitmap(nullptr),
	m_BitmapTinted(nullptr),
	m_Skin(skin),
	m_Name(name ? name : L"ImageName"), 
	m_OptionArray(optionArray ? optionArray : c_DefaultOptionArray),
	m_DisableTransform(disableTransform),
	m_Crop(-1, -1, -1, -1),
	m_CropMode(CROPMODE_TL),
	m_Rotate()
{
}

GeneralImage::~GeneralImage()
{
	if (m_Bitmap) delete m_Bitmap;
}

void GeneralImage::ReadOptions(ConfigParser& parser, const WCHAR* section, const WCHAR* imagePath)
{
	Rect oldCrop = m_Crop;
	CROPMODE oldCropMode = m_CropMode;
	REAL oldRotate = m_Rotate;

	if (!m_DisableTransform)
	{
		m_Crop.X = m_Crop.Y = m_Crop.Width = m_Crop.Height = -1;
		m_CropMode = CROPMODE_TL;

		const std::wstring& crop = parser.ReadString(section, m_OptionArray[OptionIndexImageCrop], L"");
		if (!crop.empty())
		{
			if (wcschr(crop.c_str(), L','))
			{
				WCHAR* context = nullptr;
				WCHAR* parseSz = _wcsdup(crop.c_str());
				WCHAR* token;

				token = wcstok(parseSz, L",", &context);
				if (token)
				{
					m_Crop.X = parser.ParseInt(token, 0);

					token = wcstok(nullptr, L",", &context);
					if (token)
					{
						m_Crop.Y = parser.ParseInt(token, 0);

						token = wcstok(nullptr, L",", &context);
						if (token)
						{
							m_Crop.Width = parser.ParseInt(token, 0);

							token = wcstok(nullptr, L",", &context);
							if (token)
							{
								m_Crop.Height = parser.ParseInt(token, 0);

								token = wcstok(nullptr, L",", &context);
								if (token)
								{
									m_CropMode = (CROPMODE)parser.ParseInt(token, 0);
								}
							}
						}
					}
				}
				free(parseSz);
			}

			if (m_CropMode < CROPMODE_TL || m_CropMode > CROPMODE_C)
			{
				m_CropMode = CROPMODE_TL;
				LogErrorF(m_Skin, L"%s=%s (origin) is not valid in [%s]", m_OptionArray[OptionIndexImageCrop], crop, section);
			}
		}
	}

	Color tint = parser.ReadColor(section, m_OptionArray[OptionIndexImageTint], Color::White);
	int alpha = parser.ReadInt(section, m_OptionArray[OptionIndexImageAlpha], tint.GetAlpha());  // for backwards compatibility
	alpha = min(255, alpha);
	alpha = max(0, alpha);

	m_ColorMatrix = c_IdentityMatrix;

	// Read in the Color Matrix
	// It has to be read in like this because it crashes when reading over 17 floats
	// at one time. The parser does it fine, but after putting the returned values
	// into the Color Matrix the next time the parser is used it crashes.
	// Note: is this still relevant? Kept for BWC
	std::vector<Gdiplus::REAL> matrix1 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix1]);
	if (matrix1.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 0.
		{
			m_ColorMatrix.m[0][i] = matrix1[i];
		}
	}
	else
	{
		m_ColorMatrix.m[0][0] = (Gdiplus::REAL)tint.GetRed() / 255.0f;
	}

	std::vector<Gdiplus::REAL> matrix2 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix2]);
	if (matrix2.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 0.
		{
			m_ColorMatrix.m[1][i] = matrix2[i];
		}
	}
	else
	{
		m_ColorMatrix.m[1][1] = (Gdiplus::REAL)tint.GetGreen() / 255.0f;
	}

	std::vector<Gdiplus::REAL> matrix3 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix3]);
	if (matrix3.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 0.
		{
			m_ColorMatrix.m[2][i] = matrix3[i];
		}
	}
	else
	{
		m_ColorMatrix.m[2][2] = (Gdiplus::REAL)tint.GetBlue() / 255.0f;
	}

	std::vector<Gdiplus::REAL> matrix4 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix4]);
	if (matrix4.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 0.
		{
			m_ColorMatrix.m[3][i] = matrix4[i];
		}
	}
	else
	{
		m_ColorMatrix.m[3][3] = (Gdiplus::REAL)alpha / 255.0f;
	}

	std::vector<Gdiplus::REAL> matrix5 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix5]);
	if (matrix5.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 1.
		{
			m_ColorMatrix.m[4][i] = matrix5[i];
		}
	}

	if (!m_DisableTransform)
	{
		m_Rotate = (REAL)parser.ReadFloat(section, m_OptionArray[OptionIndexImageRotate], 0.0);
	}
}

bool GeneralImage::LoadImage(const std::wstring& imageName)
{
	if (m_Skin == nullptr) return false;
	delete m_Bitmap;
	m_Bitmap = new Gfx::D2DBitmap(imageName);
	if (m_Bitmap->Load(m_Skin->GetCanvas()))
	{
		ApplyTransforms();
		return true;
	}
	delete m_Bitmap;
	m_Bitmap = nullptr;
	return false;
}

void GeneralImage::ApplyCrop(Gfx::Util::D2DEffectStream* stream) const
{
	if (m_Crop.Width >= 0 && m_Crop.Height >= 0)
	{
		const int imageW = m_Bitmap->GetWidth();
		const int imageH = m_Bitmap->GetHeight();

		int x, y;

		switch (m_CropMode)
		{
		case CROPMODE_TL:
		default:
			x = m_Crop.X;
			y = m_Crop.Y;
			break;

		case CROPMODE_TR:
			x = m_Crop.X + imageW;
			y = m_Crop.Y;
			break;

		case CROPMODE_BR:
			x = m_Crop.X + imageW;
			y = m_Crop.Y + imageH;
			break;

		case CROPMODE_BL:
			x = m_Crop.X;
			y = m_Crop.Y + imageH;
			break;

		case CROPMODE_C:
			x = m_Crop.X + (imageW / 2);
			y = m_Crop.Y + (imageH / 2);
			break;
		}

		stream->Crop(m_Skin->GetCanvas(), {(FLOAT)x, (FLOAT)y, (FLOAT)(m_Crop.Width + x), (FLOAT)(m_Crop.Height + y)});
	}
}

void GeneralImage::ApplyTransforms()
{
	if (m_BitmapTinted)
	{
		delete m_BitmapTinted;
		m_BitmapTinted = nullptr;
	}

	auto stream = m_Bitmap->CreateEffectStream();

	ApplyCrop(stream);

	if(m_Rotate != 0)
		stream->Rotate(m_Skin->GetCanvas(), m_Rotate);

	if(!CompareColorMatrix(m_ColorMatrix, c_IdentityMatrix))
		stream->Tint(m_Skin->GetCanvas(), m_ColorMatrix);
	

	m_BitmapTinted = stream->ToBitmap(m_Skin->GetCanvas());
	delete stream;
}

/*
** Compares the two given color matrices.
**
*/
bool GeneralImage::CompareColorMatrix(const D2D1_MATRIX_5X4_F& a, const D2D1_MATRIX_5X4_F& b)
{
	for (int i = 0; i < 5; ++i)
	{
		for (int j = 0; j < 4; ++j)  // The fifth column is reserved.
		{
			if (a.m[i][j] != b.m[i][j])
			{
				return false;
			}
		}
	}
	return true;
}

