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

#pragma warning(disable: 4786)
#pragma warning(disable: 4996)

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
	m_DimensionsDefined = false;
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

	LoadImage(true);
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
			if (!m_DimensionsDefined)
			{
				m_W = m_Bitmap->GetWidth();
				m_H = m_Bitmap->GetHeight();
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

	m_ImageName = parser.ReadString(section, L"ImageName", L"");
	
	m_Path = parser.ReadString(section, L"Path", L"");
	if (!m_Path.empty())
	{
		if (m_Path[m_Path.length() - 1] != L'\\')
		{
			m_Path += L"\\";
		}
	}
	m_ImageName = m_MeterWindow->MakePathAbsolute(m_Path + m_ImageName);

	if (-1 != parser.ReadInt(section, L"W", -1) && -1 != parser.ReadInt(section, L"H", -1))
	{
		m_DimensionsDefined = true;
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
	if (CMeter::Update() && m_Measure)
	{
		std::wstring val = m_Measure->GetStringValue(false, 1, 0, false);
		if (!val.empty())
		{
			// Load the new image
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
	return false;
}

/*
** Draw
**
** Draws the meter on the double buffer
**
*/
bool CMeterImage::Draw()
{
	if(!CMeter::Draw()) return false;

	if (m_Bitmap != NULL)
	{
		Graphics graphics(m_MeterWindow->GetDoubleBuffer());

		// Copy the image over the doublebuffer
		int x = GetX();
		int y = GetY();
		Rect r(x, y, m_W, m_H);
		graphics.DrawImage(m_Bitmap, r, 0, 0, m_Bitmap->GetWidth(), m_Bitmap->GetHeight(), UnitPixel);
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
	try
	{
		CMeter::BindMeasure(measures);
	}
	catch(CError)
	{
		// Do nothing (ignore errors)
	}
}

