/*
  Copyright (C) 2001 Kimmo Pekkola

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

#include "MeterBitmap.h"
#include "Measure.h"
#include "Error.h"
#include "Rainmeter.h"

using namespace Gdiplus;

extern CRainmeter* Rainmeter;

/*
** CMeterBitmap
**
** The constructor
**
*/
CMeterBitmap::CMeterBitmap(CMeterWindow* meterWindow) : CMeter(meterWindow)
{
	m_Bitmap = NULL;
	m_FrameCount = 1;
	m_ZeroFrame = false;
	m_Align = ALIGN_LEFT;
	m_Extend = false;
	m_Separation = 0;
	m_Digits = 0;
	m_Value = 0;
}

/*
** ~CMeterBitmap
**
** The destructor
**
*/
CMeterBitmap::~CMeterBitmap()
{
	if(m_Bitmap != NULL) delete m_Bitmap;
}

/*
** Initialize
**
** Load the image and get the dimensions of the meter from it.
**
*/
void CMeterBitmap::Initialize()
{
	CMeter::Initialize();

	// Load the bitmaps if defined
	if(!m_ImageName.empty())
	{
		m_Bitmap = new Bitmap(m_ImageName.c_str());
		Status status = m_Bitmap->GetLastStatus();
		if(Ok != status)
		{
            throw CError(std::wstring(L"Bitmap image not found: ") + m_ImageName, __LINE__, __FILE__);
		}

		m_W = m_Bitmap->GetWidth();
		m_H = m_Bitmap->GetHeight();

		if(m_H > m_W)
		{
			m_H = m_H / m_FrameCount;
		}
		else
		{
			m_W = m_W / m_FrameCount;
		}
	}
}

/*
** HitTest
**
** Checks if the given point is inside the meter.
**
*/
bool CMeterBitmap::HitTest(int x, int y)
{
	if (m_Extend)
	{
		int value = (int)m_Value;
		value = max(0, value);		// Only positive integers are supported

		int tmpValue = value;
		
		// Calc the number of numbers
		int numOfNums = 0;

		if (m_Digits > 0)
		{
			numOfNums = m_Digits;
		}
		else
		{
			do
			{
				numOfNums ++;
				if (m_FrameCount == 1)
				{
					tmpValue /= 2;
				}
				else
				{
					tmpValue /= m_FrameCount;
				}
			} while (tmpValue > 0);
		}

		Rect rect(GetX(), GetY(), m_W * numOfNums + (numOfNums - 1) * m_Separation, m_H);

		if (m_Align == ALIGN_CENTER)
		{
			rect.Offset(-rect.Width / 2, 0);
		}
		else if (m_Align == ALIGN_RIGHT)
		{
			rect.Offset(-rect.Width, 0);
		}

		if (rect.Contains(x, y))
		{
			return true;
		}
		return false;
	}
	else
	{
		return CMeter::HitTest(x, y);
	}
}

/*
** ReadConfig
**
** Read the meter-specific configs from the ini-file.
**
*/
void CMeterBitmap::ReadConfig(const WCHAR* section)
{
	// Read common configs
	CMeter::ReadConfig(section);

	CConfigParser& parser = m_MeterWindow->GetParser();

	m_ImageName = parser.ReadString(section, L"BitmapImage", L"");
	m_ImageName = Rainmeter->FixPath(m_ImageName, PATH_FOLDER_CURRENT_SKIN, m_MeterWindow->GetSkinName());

	m_FrameCount = parser.ReadInt(section, L"BitmapFrames", 1);
	m_ZeroFrame = 0!=parser.ReadInt(section, L"BitmapZeroFrame", 0);

	m_Separation = parser.ReadInt(section, L"BitmapSeparation", 0);
	m_Extend = 0!=parser.ReadInt(section, L"BitmapExtend", 0);
	m_Digits = parser.ReadInt(section, L"BitmapDigits", 0);

	std::wstring align;
	align = parser.ReadString(section, L"BitmapAlign", L"LEFT");
	
	if(_wcsicmp(align.c_str(), L"LEFT") == 0)
	{
		m_Align = ALIGN_LEFT;
	}
	else if(_wcsicmp(align.c_str(), L"RIGHT") == 0)
	{
		m_Align = ALIGN_RIGHT;
	}
	else if(_wcsicmp(align.c_str(), L"CENTER") == 0)
	{
		m_Align = ALIGN_CENTER;
	}
	else
	{
        throw CError(std::wstring(L"No such BitmapAlign: ") + align, __LINE__, __FILE__);
	}
}

/*
** Update
**
** Updates the value(s) from the measures.
**
*/
bool CMeterBitmap::Update()
{
	if (CMeter::Update() && m_Measure)
	{
		if (m_Extend)
		{
			m_Value = m_Measure->GetValue();
		}
		else
		{
			m_Value = m_Measure->GetRelativeValue();
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
bool CMeterBitmap::Draw()
{
	if(!CMeter::Draw()) return false;

	int newY, newX;

	if(m_FrameCount == 0 || m_Bitmap == NULL) return false;	// Unable to continue

	int x = GetX();
	int y = GetY();

	if (m_Extend)
	{
		int value = (int)m_Value;
		value = max(0, value);		// Only positive integers are supported

		int tmpValue = value;
		// Calc the number of numbers
		int numOfNums = 0;

		if (m_Digits > 0)
		{
			numOfNums = m_Digits;
		}
		else
		{
			do
			{
				numOfNums ++;
				if (m_FrameCount == 1)
				{
					tmpValue /= 2;
				}
				else
				{
					tmpValue /= m_FrameCount;
				}
			} while (tmpValue > 0);
		}

		// Blit the images
		int offset;
		Graphics graphics(m_MeterWindow->GetDoubleBuffer());
		
		if (m_Align == ALIGN_RIGHT)
		{
			offset = 0;
		}
		else if (m_Align == ALIGN_CENTER)
		{
			offset = numOfNums * (m_W + m_Separation) / 2;
		}
		else
		{
			offset = numOfNums * (m_W + m_Separation);
		}

		do
		{
			offset = offset - (m_W + m_Separation);

			Rect r(x + offset, y, m_W, m_H);

			if(m_Bitmap->GetHeight() > m_Bitmap->GetWidth())
			{
				newX = 0;
				newY = m_H * (value % m_FrameCount);
			}
			else
			{
				newX = m_W * (value % m_FrameCount);
				newY = 0;
			}

			graphics.DrawImage(m_Bitmap, r, newX, newY, m_W, m_H, UnitPixel);
			if (m_FrameCount == 1)
			{
				value /= 2;
			}
			else
			{
				value /= m_FrameCount;
			}
			numOfNums--;
		} while (numOfNums > 0);
	}
	else
	{
		int frame = 0;

		if (m_ZeroFrame)
		{
			// Use the first frame only if the value is zero
			if (m_Value > 0)
			{
				frame = (int)(m_Value * (m_FrameCount - 1));
			}
		}
		else
		{
			// Select the correct frame linearly
			frame = (int)(m_Value * m_FrameCount);
		}

		if(m_Bitmap->GetHeight() > m_Bitmap->GetWidth())
		{
			newX = 0;
			newY = frame * m_H;
		}
		else
		{
			newX = frame * m_W;
			newY = 0;
		}

		// Blit the image
		Graphics graphics(m_MeterWindow->GetDoubleBuffer());
		Rect r(x, y, m_W, m_H);
		graphics.DrawImage(m_Bitmap, r, newX, newY, m_W, m_H, UnitPixel);
	}

	return true;
}

