/* Copyright (C) 2018 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "GeneralImage.h"
#include "Logger.h"
#include "../Common/PathUtil.h"

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
	m_BitmapProcessed(nullptr),
	m_Skin(skin),
	m_Name(name ? name : L"ImageName"), 
	m_OptionArray(optionArray ? optionArray : c_DefaultOptionArray),
	m_DisableTransform(disableTransform),
	m_Options()
{
}

GeneralImage::~GeneralImage()
{
	DisposeImage();
}

void GeneralImage::DisposeImage()
{
	if (m_Bitmap)
	{
		delete m_Bitmap;
		m_Bitmap = nullptr;
	}

	if (m_BitmapProcessed)
	{
		delete m_BitmapProcessed;
		m_BitmapProcessed = nullptr;
	}
}

void GeneralImage::ReadOptions(ConfigParser& parser, const WCHAR* section, const WCHAR* imagePath)
{
	m_Path = parser.ReadString(section, m_OptionArray[OptionIndexImagePath], imagePath);
	PathUtil::AppendBackslashIfMissing(m_Path);

	if (!m_DisableTransform)
	{
		m_Options.m_Crop.left = m_Options.m_Crop.top = m_Options.m_Crop.right = m_Options.m_Crop.bottom = -1;
		m_Options.m_CropMode = ImageOptions::CROPMODE_TL;

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
					m_Options.m_Crop.left = (FLOAT)parser.ParseInt(token, 0);

					token = wcstok(nullptr, L",", &context);
					if (token)
					{
						m_Options.m_Crop.top = (FLOAT)parser.ParseInt(token, 0);

						token = wcstok(nullptr, L",", &context);
						if (token)
						{
							m_Options.m_Crop.right = (FLOAT)parser.ParseInt(token, 0) + m_Options.m_Crop.left;

							token = wcstok(nullptr, L",", &context);
							if (token)
							{
								m_Options.m_Crop.bottom = (FLOAT)parser.ParseInt(token, 0) + m_Options.m_Crop.top;

								token = wcstok(nullptr, L",", &context);
								if (token)
								{
									m_Options.m_CropMode = (ImageOptions::CROPMODE)parser.ParseInt(token, 0);
								}
							}
						}
					}
				}
				free(parseSz);
			}

			if (m_Options.m_CropMode < ImageOptions::CROPMODE_TL || m_Options.m_CropMode > ImageOptions::CROPMODE_C)
			{
				m_Options.m_CropMode = ImageOptions::CROPMODE_TL;
				LogErrorF(m_Skin, L"%s=%s (origin) is not valid in [%s]", m_OptionArray[OptionIndexImageCrop], crop, section);
			}
		}
	}

	m_Options.m_GreyScale = parser.ReadBool(section, m_OptionArray[OptionIndexGreyscale], false);

	D2D1_COLOR_F tint = parser.ReadColor(section, m_OptionArray[OptionIndexImageTint], D2D1::ColorF(D2D1::ColorF::White));
	int alpha = parser.ReadInt(section, m_OptionArray[OptionIndexImageAlpha], (INT)(tint.a * 255));  // for backwards compatibility
	alpha = min(255, alpha);
	alpha = max(0, alpha);

	m_Options.m_ColorMatrix = c_IdentityMatrix;

	// Read in the Color Matrix
	// It has to be read in like this because it crashes when reading over 17 floats
	// at one time. The parser does it fine, but after putting the returned values
	// into the Color Matrix the next time the parser is used it crashes.
	// Note: is this still relevant? Kept for BWC
	std::vector<FLOAT> matrix1 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix1]);
	if (matrix1.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 0.
		{
			m_Options.m_ColorMatrix.m[0][i] = matrix1[i];
		}
	}
	else
	{
		m_Options.m_ColorMatrix.m[0][0] = tint.r;
	}

	std::vector<FLOAT> matrix2 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix2]);
	if (matrix2.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 0.
		{
			m_Options.m_ColorMatrix.m[1][i] = matrix2[i];
		}
	}
	else
	{
		m_Options.m_ColorMatrix.m[1][1] = tint.g;
	}

	std::vector<FLOAT> matrix3 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix3]);
	if (matrix3.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 0.
		{
			m_Options.m_ColorMatrix.m[2][i] = matrix3[i];
		}
	}
	else
	{
		m_Options.m_ColorMatrix.m[2][2] = tint.b;
	}

	std::vector<FLOAT> matrix4 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix4]);
	if (matrix4.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 0.
		{
			m_Options.m_ColorMatrix.m[3][i] = matrix4[i];
		}
	}
	else
	{
		m_Options.m_ColorMatrix.m[3][3] = alpha / 255.0f;
	}

	std::vector<FLOAT> matrix5 = parser.ReadFloats(section, m_OptionArray[OptionIndexColorMatrix5]);
	if (matrix5.size() == 5)
	{
		for (int i = 0; i < 4; ++i)  // The fifth column must be 1.
		{
			m_Options.m_ColorMatrix.m[4][i] = matrix5[i];
		}
	}

	const WCHAR* flip = parser.ReadString(section, m_OptionArray[OptionIndexImageFlip], L"NONE").c_str();
	if (_wcsicmp(flip, L"NONE") == 0)
	{
		m_Options.m_Flip = Gfx::Util::FlipType::None;
	}
	else if (_wcsicmp(flip, L"HORIZONTAL") == 0)
	{
		m_Options.m_Flip = Gfx::Util::FlipType::Horizontal;
	}
	else if (_wcsicmp(flip, L"VERTICAL") == 0)
	{
		m_Options.m_Flip = Gfx::Util::FlipType::Vertical;
	}
	else if (_wcsicmp(flip, L"BOTH") == 0)
	{
		m_Options.m_Flip = Gfx::Util::FlipType::Both;
	}
	else
	{
		LogErrorF(m_Skin, L"%s=%s (origin) is not valid in [%s]", m_OptionArray[OptionIndexImageFlip], flip, section);
	}

	if (!m_DisableTransform)
	{
		m_Options.m_Rotate = (FLOAT)parser.ReadFloat(section, m_OptionArray[OptionIndexImageRotate], 0.0);
	}

	m_Options.m_UseExifOrientation = parser.ReadBool(section, m_OptionArray[OptionIndexUseExifOrientation], false);
}

bool GeneralImage::LoadImage(const std::wstring& imageName)
{
	if (!m_Skin) return false;

	std::wstring filename = m_Path + imageName;
	m_Skin->MakePathAbsolute(filename);

	// Check extension and if it is missing, add .png
	size_t pos = filename.rfind(L'\\');
	if (filename.find(L'.', (pos == std::wstring::npos) ? 0 : pos + 1) == std::wstring::npos)
	{
		filename += L".png";
	}

	if (m_Bitmap && !m_Bitmap->GetBitmap()->HasFileChanged(filename))
	{
		ApplyTransforms();
		return true;
	}

	ImageOptions info;
	Gfx::D2DBitmap::GetFileInfo(filename, &info);

	if (!info.isValid())
	{
		DisposeImage();
		return false;
	}

	ImageCacheHandle* handle = GetImageCache().Get(info);
	if (!handle)
	{
		auto bitmap = new Gfx::D2DBitmap(filename);

		HRESULT hr = bitmap->Load(m_Skin->GetCanvas());
		if (SUCCEEDED(hr))
		{
			GetImageCache().Put(info, bitmap);
			handle = GetImageCache().Get(info);
			if (!handle) return false;
		}
		else
		{
			delete bitmap;
			bitmap = nullptr;
		}
	}

	if (handle)
	{
		m_Bitmap = handle;

		m_Options.m_Path = info.m_Path;
		m_Options.m_FileSize = info.m_FileSize;
		m_Options.m_FileTime = info.m_FileTime;

		ApplyTransforms();
		return true;
	}

	DisposeImage();

	return false;
}

void GeneralImage::ApplyCrop(Gfx::Util::D2DEffectStream* stream) const
{
	const auto& crop = m_Options.m_Crop;
	if (crop.right - crop.left > 0.0f && crop.bottom - crop.top > 0.0f)
	{
		const FLOAT imageW = (FLOAT)m_Bitmap->GetBitmap()->GetWidth();
		const FLOAT imageH = (FLOAT)m_Bitmap->GetBitmap()->GetHeight();

		FLOAT x = 0.0f;
		FLOAT y = 0.0f;

		switch (m_Options.m_CropMode)
		{
		case ImageOptions::CROPMODE_TL:
		default:
			x = crop.left;
			y = crop.top;
			break;

		case ImageOptions::CROPMODE_TR:
			x = crop.left + imageW;
			y = crop.top;
			break;

		case ImageOptions::CROPMODE_BR:
			x = crop.left + imageW;
			y = crop.top + imageH;
			break;

		case ImageOptions::CROPMODE_BL:
			x = crop.left;
			y = crop.top + imageH;
			break;

		case ImageOptions::CROPMODE_C:
			x = crop.left + (imageW / 2.0f);
			y = crop.top + (imageH / 2.0f);
			break;
		}

		const D2D1_RECT_F rect = D2D1::RectF(x, y, crop.right - crop.left + x, crop.bottom - crop.top + y);
		stream->Crop(m_Skin->GetCanvas(), rect);
	}
}

void GeneralImage::ApplyTransforms()
{
	if (m_BitmapProcessed && m_BitmapProcessed->GetKey() == m_Options) return;

	if (m_BitmapProcessed)
	{
		delete m_BitmapProcessed;
		m_BitmapProcessed = nullptr;
	}

	ImageCacheHandle* handle = GetImageCache().Get(m_Options);
	if (!handle)
	{
		auto& canvas = m_Skin->GetCanvas();
		auto stream = m_Bitmap->GetBitmap()->CreateEffectStream();

		ApplyCrop(stream);

		if (m_Options.m_Rotate != 0.0f) stream->Rotate(canvas, m_Options.m_Rotate);

		stream->Flip(canvas, m_Options.m_Flip);

		if (m_Options.m_UseExifOrientation) stream->ApplyExifOrientation(canvas);

		if (m_Options.m_GreyScale) stream->Tint(canvas, c_GreyScaleMatrix);

		if (!CompareColorMatrix(m_Options.m_ColorMatrix, c_IdentityMatrix)) stream->Tint(canvas, m_Options.m_ColorMatrix);

		auto bitmap = stream->ToBitmap(canvas);
		delete stream;
		stream = nullptr;

		if (bitmap != nullptr)
		{
			GetImageCache().Put(m_Options, bitmap);
			handle = GetImageCache().Get(m_Options);
			if (!handle) return;
		}
	}

	if (handle)
	{
		m_BitmapProcessed = handle;
	}
}

bool GeneralImage::CompareColorMatrix(const D2D1_MATRIX_5X4_F& a, const D2D1_MATRIX_5X4_F& b)
{
	for (int i = 0; i < 5; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			if (a.m[i][j] != b.m[i][j])
			{
				return false;
			}
		}
	}
	return true;
}
