/*
  Copyright (C) 2002 Kimmo Pekkola

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
#include "MeterImage.h"
#include "Measure.h"
#include "Error.h"
#include "Rainmeter.h"

extern CRainmeter* Rainmeter;

using namespace Gdiplus;

#define PI 3.14159265f

// GrayScale Matrix
const Gdiplus::ColorMatrix CMeterImage::c_GreyScaleMatrix = {
	0.299f, 0.299f, 0.299f, 0.0f, 0.0f,
	0.587f, 0.587f, 0.587f, 0.0f, 0.0f,
	0.114f, 0.114f, 0.114f, 0.0f, 0.0f,
	  0.0f,   0.0f,   0.0f, 1.0f, 0.0f,
	  0.0f,   0.0f,   0.0f, 0.0f, 1.0f
};

const Gdiplus::ColorMatrix CMeterImage::c_IdentifyMatrix = {
	1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f, 1.0f
};

/*
** CMeterImage
**
** The constructor
**
*/
CMeterImage::CMeterImage(CMeterWindow* meterWindow) : CMeter(meterWindow)
{
	m_Bitmap = NULL;
	m_BitmapTint = NULL;
	m_NeedsReload = false;
	m_NeedsTinting = false;
	m_NeedsTransform = false;
	m_WidthDefined = false;
	m_HeightDefined = false;
	m_PreserveAspectRatio = false;
	m_hBuffer = NULL;
	m_Modified.dwHighDateTime = 0;
	m_Modified.dwLowDateTime = 0;

	m_GreyScale = false;
	m_ColorMatrix = c_IdentifyMatrix;
	m_Flip = RotateNoneFlipNone;
	m_Rotate = 0.0f;
}

/*
** ~CMeterImage
**
** The destructor
**
*/
CMeterImage::~CMeterImage()
{
	if(m_Bitmap != NULL) delete m_Bitmap;
	if(m_BitmapTint != NULL) delete m_BitmapTint;

	if (m_hBuffer)
	{
		::GlobalFree(m_hBuffer);
	}
}

/*
** Initialize
**
** Load the image and get the dimensions of the meter from it.
**
*/
void CMeterImage::Initialize()
{
	CMeter::Initialize();

	if (!m_DynamicVariables) LoadImage(true);
}

/*
** ReadConfig
**
** Loads the image from disk
**
*/
void CMeterImage::LoadImage(bool bLoadAlways)
{
	// Load the bitmap if defined
	if (!m_ImageName.empty())
	{
		std::wstring filename = m_ImageName;

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
				m_Modified = tmpTime;

				DWORD imageSize = GetFileSize(fileHandle, 0);

				if (imageSize != -1)
				{
					if (m_hBuffer)
					{
						::GlobalFree(m_hBuffer);
					}

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
								if (m_Bitmap) delete m_Bitmap;

								if (m_BitmapTint)
								{
									delete m_BitmapTint;
									m_BitmapTint = NULL;
								}

								m_Bitmap = Bitmap::FromStream(pStream);
								if (m_Bitmap)
								{
									Status status = m_Bitmap->GetLastStatus();
									if(Ok != status)
									{
										DebugLog(L"Unable to create bitmap: %s", filename.c_str());
										delete m_Bitmap;
										m_Bitmap = NULL;
									}
									else
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
								}

								pStream->Release();
							}
						}
					}
					else
					{
						DebugLog(L"Failed to allocate memory: %i bytes", imageSize);
					}
				}
			}
			CloseHandle(fileHandle);
		}
		else
		{
			DebugLog(L"Unable to load image: %s", filename.c_str());
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

			Bitmap* bitmap = (m_BitmapTint) ? m_BitmapTint : m_Bitmap;

			// Calculate size of the meter
			int imageW = bitmap->GetWidth();
			int imageH = bitmap->GetHeight();

			if (m_WidthDefined)
			{
				if (!m_HeightDefined)
				{
					m_H = (imageW == 0) ? 0 : m_W * imageH / imageW;
				}
			}
			else
			{
				if (m_HeightDefined)
				{
					m_W = (imageH == 0) ? 0 : m_H * imageW / imageH;
				}
				else
				{
					m_W = imageW;
					m_H = imageH;
				}
			}
		}
	}
	else
	{
		if (m_Bitmap)
		{
			delete m_Bitmap;
			m_Bitmap = NULL;
		}
		if (m_BitmapTint)
		{
			delete m_BitmapTint;
			m_BitmapTint = NULL;
		}
	}
}

/*
** ApplyTint
**
** This will apply the Greyscale matrix and the color tinting.
**
*/
void CMeterImage::ApplyTint()
{
	ImageAttributes ImgAttr;
	ImgAttr.SetColorMatrix(&m_ColorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

	if (m_BitmapTint) delete m_BitmapTint;

	Rect r(0, 0, m_Bitmap->GetWidth(), m_Bitmap->GetHeight());
	m_BitmapTint = new Bitmap(r.Width, r.Height, PixelFormat32bppARGB);

	Graphics graphics(m_BitmapTint);

	if (m_GreyScale)
	{
		Bitmap* gray = TurnGreyscale();
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
Bitmap* CMeterImage::TurnGreyscale()
{
	ImageAttributes ImgAttr;
	ImgAttr.SetColorMatrix(&c_GreyScaleMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

	// We need a blank bitmap to paint our greyscale to in case of alpha
	Rect r(0, 0, m_Bitmap->GetWidth(), m_Bitmap->GetHeight());
	Bitmap* bitmap = new Bitmap(r.Width, r.Height, PixelFormat32bppARGB);

	Graphics graphics(bitmap);
	graphics.DrawImage(m_Bitmap, r, 0, 0, r.Width, r.Height, UnitPixel, &ImgAttr);

	return bitmap;
}

/*
** ApplyTransform
**
** This will apply the flipping and rotating.
**
*/
void CMeterImage::ApplyTransform()
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

		if (m_BitmapTint) delete m_BitmapTint;
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

		if (m_BitmapTint) delete m_BitmapTint;
		m_BitmapTint = transform;
	}
}

/*
** ReadConfig
**
** Read the meter-specific configs from the ini-file.
**
*/
void CMeterImage::ReadConfig(const WCHAR* section)
{
	// Store the current values so we know if the image needs to be tinted or transformed
	bool oldGreyScale = m_GreyScale;
	ColorMatrix oldColorMatrix = m_ColorMatrix;
	RotateFlipType oldFlip = m_Flip;
	REAL oldRotate = m_Rotate;

	// Read common configs
	CMeter::ReadConfig(section);

	CConfigParser& parser = m_MeterWindow->GetParser();

	m_Path = parser.ReadString(section, L"Path", L"");
	if (!m_Path.empty())
	{
		WCHAR ch = m_Path[m_Path.length() - 1];
		if (ch != L'\\' && ch != L'/')
		{
			m_Path += L"\\";
		}
	}

	if (!m_Initialized || !m_Measure)
	{
		std::wstring oldImageName = m_ImageName;

		m_ImageName = parser.ReadString(section, L"ImageName", L"");
		m_ImageName = m_MeterWindow->MakePathAbsolute(m_Path + m_ImageName);

		if (m_DynamicVariables)
		{
			m_NeedsReload = (oldImageName != m_ImageName);
		}
	}

	m_PreserveAspectRatio = 0!=parser.ReadInt(section, L"PreserveAspectRatio", 0);

	if (-1 != (int)parser.ReadFormula(section, L"W", -1))
	{
		m_WidthDefined = true;
	}
	if (-1 != (int)parser.ReadFormula(section, L"H", -1))
	{
		m_HeightDefined = true;
	}

	m_GreyScale = 0!=parser.ReadInt(section, L"Greyscale", 0);

	Color tint = parser.ReadColor(section, L"ImageTint", Color::White);
	int alpha = parser.ReadInt(section, L"ImageAlpha", tint.GetAlpha());  // for backwards compatibility
	alpha = min(255, alpha);
	alpha = max(0, alpha);

	if (alpha != tint.GetAlpha())
	{
		tint = Color(alpha, tint.GetRed(), tint.GetGreen(), tint.GetBlue());
	}

	m_ColorMatrix = c_IdentifyMatrix;

	// Read in the Color Matrix
	// It has to be read in like this because it crashes when reading over 17 floats
	// at one time. The parser does it fine, but after putting the returned values
	// into the Color Matrix the next time the parser is used it crashes.
	std::vector<Gdiplus::REAL> matrix = parser.ReadFloats(section, L"ColorMatrix1");
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

	matrix = parser.ReadFloats(section, L"ColorMatrix2");
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

	matrix = parser.ReadFloats(section, L"ColorMatrix3");
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

	matrix = parser.ReadFloats(section, L"ColorMatrix4");
	if (matrix.size() == 5)
	{
		for(int i = 0; i < 5; ++i)
		{
			m_ColorMatrix.m[3][i] = matrix[i];
		}
	}
	else
	{
		m_ColorMatrix.m[3][3] = (REAL)tint.GetAlpha() / 255.0f;
	}

	matrix = parser.ReadFloats(section, L"ColorMatrix5");
	if (matrix.size() == 5)
	{
		for(int i = 0; i < 5; ++i)
		{
			m_ColorMatrix.m[4][i] = matrix[i];
		}
	}

	m_NeedsTinting = (oldGreyScale != m_GreyScale || !CompareColorMatrix(oldColorMatrix, m_ColorMatrix));

	std::wstring flip;
	flip = parser.ReadString(section, L"ImageFlip", L"NONE");

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
		throw CError(std::wstring(L"ImageFlip=") + flip + L" is not valid in meter [" + m_Name + L"].", __LINE__, __FILE__);
	}

	m_Rotate = (REAL)parser.ReadFloat(section, L"ImageRotate", 0.0);

	m_NeedsTransform = (oldFlip != m_Flip || oldRotate != m_Rotate);
}

/*
** CompareColorMatrix
**
** Compares the two given color matrices.
**
*/
bool CMeterImage::CompareColorMatrix(const Gdiplus::ColorMatrix& a, const Gdiplus::ColorMatrix& b)
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

/*
** Update
**
** Updates the value(s) from the measures.
**
*/
bool CMeterImage::Update()
{
	if (CMeter::Update())
	{
		if (m_Measure)  //read from the measure
		{
			std::wstring val = m_Measure->GetStringValue(false, 1, 0, false);
			if (!val.empty())
			{
				val = m_MeterWindow->MakePathAbsolute(m_Path + val);
				if (val != m_ImageName)
				{
					m_ImageName = val;
					LoadImage(true);
				}
				else
				{
					LoadImage(false);
				}
			}
			return true;
		}
		else if (m_DynamicVariables)  //read from the skin
		{
			LoadImage(m_NeedsReload);
			return true;
		}
	}
	return false;
}

/*
** Draw
**
** Draws the meter on the double buffer
**
*/
bool CMeterImage::Draw(Graphics& graphics)
{
	if(!CMeter::Draw(graphics)) return false;

	if (m_Bitmap != NULL)
	{
		Bitmap* drawBitmap = (m_BitmapTint) ? m_BitmapTint : m_Bitmap;

		// Copy the image over the doublebuffer
		int x = GetX();
		int y = GetY();
		int imageW = drawBitmap->GetWidth();
		int imageH = drawBitmap->GetHeight();

		int drawW, drawH;

		if (m_PreserveAspectRatio)
		{
			if (imageW == 0 || imageH == 0 || m_W == 0 || m_H == 0) return true;

			REAL imageRatio = imageW / (REAL)imageH;
			REAL meterRatio = m_W / (REAL)m_H;

			if (imageRatio >= meterRatio)
			{
				drawW = m_W;
				drawH = m_W * imageH / imageW;
			}
			else
			{
				drawW = m_H * imageW / imageH;
				drawH = m_H;
			}

			// Centering
			x += (m_W - drawW) / 2;
			y += (m_H - drawH) / 2;
		}
		else
		{
			drawW = m_W;
			drawH = m_H;
		}

		Rect r(x, y, drawW, drawH);
		graphics.DrawImage(drawBitmap, r, 0, 0, imageW, imageH, UnitPixel);
	}

	return true;
}

/*
** BindMeasure
**
** Overridden method. The Image meters need not to be bound on anything
**
*/
void CMeterImage::BindMeasure(std::list<CMeasure*>& measures)
{
	// It's ok not to bind image meter to anything
	if (!m_MeasureName.empty())
	{
		CMeter::BindMeasure(measures);
	}
}
