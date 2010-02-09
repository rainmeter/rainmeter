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

/*
** CMeterImage
**
** The constructor
**
*/
CMeterImage::CMeterImage(CMeterWindow* meterWindow) : CMeter(meterWindow)
{
	m_Bitmap = NULL;
	m_NeedsUpdate = false;
	m_WidthDefined = false;
	m_HeightDefined = false;
	m_PreserveAspectRatio = false;
	m_hBuffer = NULL;
	m_Modified.dwHighDateTime = 0;
	m_Modified.dwLowDateTime = 0;
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
		HANDLE fileHandle = CreateFile(filename.c_str(), GENERIC_READ, NULL, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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
								}

								pStream->Release();
							}
						}
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
			// Calculate size of the meter
			int imageW = m_Bitmap->GetWidth();
			int imageH = m_Bitmap->GetHeight();

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
}

/*
** ReadConfig
**
** Read the meter-specific configs from the ini-file.
**
*/
void CMeterImage::ReadConfig(const WCHAR* section)
{
	// Read common configs
	CMeter::ReadConfig(section);

	CConfigParser& parser = m_MeterWindow->GetParser();

	m_Path = parser.ReadString(section, L"Path", L"");
	if (!m_Path.empty())
	{
		if (m_Path[m_Path.length() - 1] != L'\\')
		{
			m_Path += L"\\";
		}
	}

	if (!m_Measure)
	{
		std::wstring oldImageName = m_ImageName;

		m_ImageName = parser.ReadString(section, L"ImageName", L"");
		m_ImageName = m_MeterWindow->MakePathAbsolute(m_Path + m_ImageName);

		if (m_DynamicVariables)
		{
			m_NeedsUpdate = (oldImageName != m_ImageName);
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
			LoadImage(m_NeedsUpdate);
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
		// Copy the image over the doublebuffer
		int x = GetX();
		int y = GetY();
		int imageW = m_Bitmap->GetWidth();
		int imageH = m_Bitmap->GetHeight();

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
		graphics.DrawImage(m_Bitmap, r, 0, 0, imageW, imageH, UnitPixel);
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

