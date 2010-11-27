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

#include "StdAfx.h"
#include "TintedImage.h"
#include "Error.h"
#include "Litestep.h"

using namespace Gdiplus;

#define PI 3.14159265f

// GrayScale Matrix
const Gdiplus::ColorMatrix CTintedImage::c_GreyScaleMatrix = {
	0.299f, 0.299f, 0.299f, 0.0f, 0.0f,
	0.587f, 0.587f, 0.587f, 0.0f, 0.0f,
	0.114f, 0.114f, 0.114f, 0.0f, 0.0f,
	  0.0f,   0.0f,   0.0f, 1.0f, 0.0f,
	  0.0f,   0.0f,   0.0f, 0.0f, 1.0f
};

const Gdiplus::ColorMatrix CTintedImage::c_IdentifyMatrix = {
	1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 1.0f
};

/*
** CTintedImage
**
** The constructor.
**
** If disableTransform is true, following configs are ignored:
**  - ImageRotate
**
*/
CTintedImage::CTintedImage(bool disableTransform) : m_DisableTransform(disableTransform),
	m_ColorMatrix(c_IdentifyMatrix)
{
	SetConfigAttributes(L"Image", L"");

	m_Bitmap = NULL;
	m_BitmapTint = NULL;

	m_hBuffer = NULL;
	m_Modified.dwHighDateTime = 0;
	m_Modified.dwLowDateTime = 0;

	m_NeedsTinting = false;
	m_NeedsTransform = false;

	m_GreyScale = false;
	m_Flip = RotateNoneFlipNone;
	m_Rotate = 0.0f;
}

/*
** ~CTintedImage
**
** The destructor
**
*/
CTintedImage::~CTintedImage()
{
	DisposeImage();
}

/*
** DisposeImage
**
** Disposes the image buffers.
**
*/
void CTintedImage::DisposeImage()
{
	delete m_Bitmap;
	m_Bitmap = NULL;

	delete m_BitmapTint;
	m_BitmapTint = NULL;

	if (m_hBuffer)
	{
		::GlobalFree(m_hBuffer);
		m_hBuffer = NULL;
	}

	m_Modified.dwHighDateTime = 0;
	m_Modified.dwLowDateTime = 0;
}

/*
** LoadImage
**
** Loads the image from disk
**
*/
void CTintedImage::LoadImage(const std::wstring& imageName, bool bLoadAlways)
{
	// Load the bitmap if defined
	if (!imageName.empty())
	{
		std::wstring filename = imageName;

		// Check extension and if it is missing, add .png
		size_t pos = filename.find_last_of(L"\\");
		if (pos == std::wstring::npos) pos = 0;
		if (std::wstring::npos == filename.find(L'.', pos))
		{
			filename += L".png";
		}

		// Read the bitmap to memory so that it's not locked by GDI+
		HANDLE fileHandle = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if (fileHandle != INVALID_HANDLE_VALUE)
		{
			// Compare the timestamp and filename to check if the file has been changed (don't load if it's not)
			FILETIME tmpTime;
			GetFileTime(fileHandle, NULL, NULL, &tmpTime);
			if (bLoadAlways || CompareFileTime(&tmpTime, &m_Modified) != 0)
			{
				DisposeImage();
				m_Modified = tmpTime;

				DWORD imageSize = GetFileSize(fileHandle, NULL);

				if (imageSize != INVALID_FILE_SIZE)
				{
					m_hBuffer = ::GlobalAlloc(GMEM_MOVEABLE, imageSize);
					if (m_hBuffer)
					{
						void* pBuffer = ::GlobalLock(m_hBuffer);
						if (pBuffer)
						{
							DWORD readBytes;
							ReadFile(fileHandle, pBuffer, imageSize, &readBytes, NULL);
							::GlobalUnlock(m_hBuffer);

							IStream* pStream = NULL;
							if (::CreateStreamOnHGlobal(m_hBuffer, FALSE, &pStream) == S_OK)
							{
								m_Bitmap = Bitmap::FromStream(pStream);
								pStream->Release();

								if (m_Bitmap && Ok == m_Bitmap->GetLastStatus())
								{
									// Check whether the new image needs tinting (or flipping, rotating)
									if (!m_NeedsTinting)
									{
										if (m_GreyScale || !CompareColorMatrix(m_ColorMatrix, c_IdentifyMatrix))
										{
											m_NeedsTinting = true;
										}
									}
									if (!m_NeedsTransform)
									{
										if (m_Flip != RotateNoneFlipNone || m_Rotate != 0.0f)
										{
											m_NeedsTransform = true;
										}
									}
								}
								else  // failed
								{
									delete m_Bitmap;
									m_Bitmap = NULL;
								}
							}
						}

						if (!m_Bitmap)
						{
							DebugLog(L"Unable to create %s: %s", m_ConfigName.c_str(), filename.c_str());
							DisposeImage();
						}
					}
					else
					{
						DebugLog(L"Unable to allocate memory ( %i bytes ) for %s: %s", imageSize, m_ConfigName.c_str(), filename.c_str());
					}
				}
				else
				{
					DebugLog(L"Unable to get %s's file size: %s", m_ConfigName.c_str(), filename.c_str());
				}
			}
			CloseHandle(fileHandle);
		}
		else
		{
			DebugLog(L"Unable to load %s: %s", m_ConfigName.c_str(), filename.c_str());
			DisposeImage();
		}

		if (m_Bitmap)
		{
			// We need a copy of the image if has tinting (or flipping, rotating)
			if (m_NeedsTinting || m_NeedsTransform)
			{
				ApplyTint();
				m_NeedsTinting = false;

				ApplyTransform();
				m_NeedsTransform = false;
			}
		}
	}
	else if (IsLoaded())
	{
		DisposeImage();
	}
}

/*
** ApplyTint
**
** This will apply the Greyscale matrix and the color tinting.
**
*/
void CTintedImage::ApplyTint()
{
	ImageAttributes ImgAttr;
	ImgAttr.SetColorMatrix(&m_ColorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

	delete m_BitmapTint;

	Rect r(0, 0, m_Bitmap->GetWidth(), m_Bitmap->GetHeight());
	m_BitmapTint = new Bitmap(r.Width, r.Height, PixelFormat32bppARGB);

	Graphics graphics(m_BitmapTint);

	if (m_GreyScale)
	{
		Bitmap* gray = TurnGreyscale(m_Bitmap);
		graphics.DrawImage(gray, r, 0, 0, r.Width, r.Height, UnitPixel, &ImgAttr);
		delete gray;
	}
	else
	{
		graphics.DrawImage(m_Bitmap, r, 0, 0, r.Width, r.Height, UnitPixel, &ImgAttr);
	}
}

/*
** TurnGreyscale
**
** Turns the image greyscale by applying a greyscale color matrix.
** Note that the returned bitmap image must be freed by caller.
**
*/
Bitmap* CTintedImage::TurnGreyscale(Bitmap* source)
{
	ImageAttributes ImgAttr;
	ImgAttr.SetColorMatrix(&c_GreyScaleMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

	// We need a blank bitmap to paint our greyscale to in case of alpha
	Rect r(0, 0, source->GetWidth(), source->GetHeight());
	Bitmap* bitmap = new Bitmap(r.Width, r.Height, PixelFormat32bppARGB);

	Graphics graphics(bitmap);
	graphics.DrawImage(source, r, 0, 0, r.Width, r.Height, UnitPixel, &ImgAttr);

	return bitmap;
}

/*
** ApplyTransform
**
** This will apply the flipping and rotating.
**
*/
void CTintedImage::ApplyTransform()
{
	if (m_Rotate != 0.0f)
	{
		Bitmap* original = (m_BitmapTint) ? m_BitmapTint : m_Bitmap;

		REAL originalW = (REAL)original->GetWidth();
		REAL originalH = (REAL)original->GetHeight();

		REAL cos_f = cos(m_Rotate * PI / 180.0f), sin_f = sin(m_Rotate * PI / 180.0f);

		REAL transformW = fabs(originalW * cos_f) + fabs(originalH * sin_f);
		REAL transformH = fabs(originalW * sin_f) + fabs(originalH * cos_f);

		Bitmap* transform = new Bitmap((int)(transformW + 0.5f), (int)(transformH + 0.5f), PixelFormat32bppARGB);

		Graphics graphics(transform);
		graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);

		REAL cx = transformW / 2.0f;
		REAL cy = transformH / 2.0f;

		Matrix rotateMatrix;
		rotateMatrix.RotateAt(m_Rotate, PointF(cx, cy));

		graphics.SetTransform(&rotateMatrix);

		if (m_Flip != RotateNoneFlipNone)
		{
			original->RotateFlip(m_Flip);
		}

		RectF r(cx - originalW / 2.0f, cy - originalH / 2.0f, originalW, originalH);
		graphics.DrawImage(original, r, -0.5f, -0.5f, originalW + 1.0f, originalH + 1.0f, UnitPixel);  // Makes the anti-aliased edge

		if (m_Flip != RotateNoneFlipNone)
		{
			original->RotateFlip(RotateNoneFlipNone);
		}

		delete m_BitmapTint;
		m_BitmapTint = transform;
	}
	else if (m_Flip != RotateNoneFlipNone)
	{
		Bitmap* original = (m_BitmapTint) ? m_BitmapTint : m_Bitmap;

		Rect r(0, 0, original->GetWidth(), original->GetHeight());
		Bitmap* transform = new Bitmap(r.Width, r.Height, PixelFormat32bppARGB);

		Graphics graphics(transform);

		original->RotateFlip(m_Flip);

		graphics.DrawImage(original, r, 0, 0, r.Width, r.Height, UnitPixel);

		original->RotateFlip(RotateNoneFlipNone);

		delete m_BitmapTint;
		m_BitmapTint = transform;
	}
}

/*
** SetConfigAttributes
**
** Sets own attributes.
**
*/
void CTintedImage::SetConfigAttributes(const WCHAR* name, const WCHAR* prefix)
{
	if (name)
	{
		m_ConfigName = name;
	}

	if (prefix)
	{
		(m_ConfigGreyscale    = prefix) += L"Greyscale";
		(m_ConfigImageTint    = prefix) += L"ImageTint";
		(m_ConfigImageAlpha   = prefix) += L"ImageAlpha";
		(m_ConfigColorMatrix1 = prefix) += L"ColorMatrix1";
		(m_ConfigColorMatrix2 = prefix) += L"ColorMatrix2";
		(m_ConfigColorMatrix3 = prefix) += L"ColorMatrix3";
		(m_ConfigColorMatrix4 = prefix) += L"ColorMatrix4";
		(m_ConfigColorMatrix5 = prefix) += L"ColorMatrix5";
		(m_ConfigImageFlip    = prefix) += L"ImageFlip";
		(m_ConfigImageRotate  = prefix) += L"ImageRotate";
	}
}

/*
** ReadConfig
**
** Read the meter-specific configs from the ini-file.
**
*/
void CTintedImage::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	// Store the current values so we know if the image needs to be tinted or transformed
	bool oldGreyScale = m_GreyScale;
	ColorMatrix oldColorMatrix = m_ColorMatrix;
	RotateFlipType oldFlip = m_Flip;
	REAL oldRotate = m_Rotate;

	m_GreyScale = 0!=parser.ReadInt(section, m_ConfigGreyscale.c_str(), 0);

	Color tint = parser.ReadColor(section, m_ConfigImageTint.c_str(), Color::White);
	int alpha = parser.ReadInt(section, m_ConfigImageAlpha.c_str(), tint.GetAlpha());  // for backwards compatibility
	alpha = min(255, alpha);
	alpha = max(0, alpha);

	m_ColorMatrix = c_IdentifyMatrix;

	// Read in the Color Matrix
	// It has to be read in like this because it crashes when reading over 17 floats
	// at one time. The parser does it fine, but after putting the returned values
	// into the Color Matrix the next time the parser is used it crashes.
	std::vector<Gdiplus::REAL> matrix = parser.ReadFloats(section, m_ConfigColorMatrix1.c_str());
	if (matrix.size() == 5)
	{
		for (int i = 0; i < 5; ++i)
		{
			m_ColorMatrix.m[0][i] = matrix[i];
		}
	}
	else
	{
		m_ColorMatrix.m[0][0] = (REAL)tint.GetRed() / 255.0f;
	}

	matrix = parser.ReadFloats(section, m_ConfigColorMatrix2.c_str());
	if (matrix.size() == 5)
	{
		for(int i = 0; i < 5; ++i)
		{
			m_ColorMatrix.m[1][i] = matrix[i];
		}
	}
	else
	{
		m_ColorMatrix.m[1][1] = (REAL)tint.GetGreen() / 255.0f;
	}

	matrix = parser.ReadFloats(section, m_ConfigColorMatrix3.c_str());
	if (matrix.size() == 5)
	{
		for(int i = 0; i < 5; ++i)
		{
			m_ColorMatrix.m[2][i] = matrix[i];
		}
	}
	else
	{
		m_ColorMatrix.m[2][2] = (REAL)tint.GetBlue() / 255.0f;
	}

	matrix = parser.ReadFloats(section, m_ConfigColorMatrix4.c_str());
	if (matrix.size() == 5)
	{
		for(int i = 0; i < 5; ++i)
		{
			m_ColorMatrix.m[3][i] = matrix[i];
		}
	}
	else
	{
		m_ColorMatrix.m[3][3] = (REAL)alpha / 255.0f;
	}

	matrix = parser.ReadFloats(section, m_ConfigColorMatrix5.c_str());
	if (matrix.size() == 5)
	{
		for(int i = 0; i < 5; ++i)
		{
			m_ColorMatrix.m[4][i] = matrix[i];
		}
	}

	m_NeedsTinting = (oldGreyScale != m_GreyScale || !CompareColorMatrix(oldColorMatrix, m_ColorMatrix));

	std::wstring flip = parser.ReadString(section, m_ConfigImageFlip.c_str(), L"NONE");
	if(_wcsicmp(flip.c_str(), L"NONE") == 0)
	{
		m_Flip = RotateNoneFlipNone;
	}
	else if(_wcsicmp(flip.c_str(), L"HORIZONTAL") == 0)
	{
		m_Flip = RotateNoneFlipX;
	}
	else if(_wcsicmp(flip.c_str(), L"VERTICAL") == 0)
	{
		m_Flip = RotateNoneFlipY;
	}
	else if(_wcsicmp(flip.c_str(), L"BOTH") == 0)
	{
		m_Flip = RotateNoneFlipXY;
	}
	else
	{
		std::wstring error = m_ConfigImageFlip + L"=";
		error += flip;
		error += L" is not valid in meter [";
		error += section;
		error += L"].";
		throw CError(error, __LINE__, __FILE__);
	}

	if (!m_DisableTransform)
	{
		m_Rotate = (REAL)parser.ReadFloat(section, m_ConfigImageRotate.c_str(), 0.0);
	}

	m_NeedsTransform = (oldFlip != m_Flip || oldRotate != m_Rotate);
}

/*
** CompareColorMatrix
**
** Compares the two given color matrices.
**
*/
bool CTintedImage::CompareColorMatrix(const Gdiplus::ColorMatrix& a, const Gdiplus::ColorMatrix& b)
{
	for (int i = 0; i < 5; ++i)
	{
		for (int j = 0; j < 5; ++j)
		{
			if (a.m[i][j] != b.m[i][j])
			{
				return false;
			}
		}
	}
	return true;
}
