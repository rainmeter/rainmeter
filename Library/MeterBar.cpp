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

#include "StdAfx.h"
#include "MeterBar.h"
#include "Measure.h"
#include "Error.h"
#include "Litestep.h"
#include "Rainmeter.h"

using namespace Gdiplus;

extern CRainmeter* Rainmeter;

/*
** CMeterBar
**
** The constructor
**
*/
CMeterBar::CMeterBar(CMeterWindow* meterWindow) : CMeter(meterWindow)
{
	m_Color = 0;
	m_Bitmap = NULL;
	m_Value = 0.0;
	m_Border = 0;
	m_Flip = false;
}

/*
** ~CMeterBar
**
** The destructor
**
*/
CMeterBar::~CMeterBar()
{
	if(m_Bitmap != NULL) delete m_Bitmap;
}

/*
** Initialize
**
** Load the image or create the brush. If image is used get the dimensions
** of the meter from it.
**
*/
void CMeterBar::Initialize()
{
	CMeter::Initialize();

	// Load the bitmaps if defined
	if(!m_ImageName.empty())
	{
		if (m_Bitmap != NULL) delete m_Bitmap;
		m_Bitmap = new Bitmap(m_ImageName.c_str());
		Status status = m_Bitmap->GetLastStatus();
		if(Ok != status)
		{
			DebugLog(L"Bitmap image not found: %s", m_ImageName.c_str());

			delete m_Bitmap;
			m_Bitmap = NULL;
		}
		else
		{
			m_W = m_Bitmap->GetWidth();
			m_H = m_Bitmap->GetHeight();
		}
	}
	else
	{
		if (m_Bitmap)
		{
			delete m_Bitmap;
			m_Bitmap = NULL;
		}
	}
}

/*
** ReadConfig
**
** Read the meter-specific configs from the ini-file.
**
*/
void CMeterBar::ReadConfig(const WCHAR* section)
{
	// Store the current values so we know if the image needs to be updated
	std::wstring oldImageName = m_ImageName;
	int oldW = m_W;
	int oldH = m_H;

	// Read common configs
	CMeter::ReadConfig(section);

	CConfigParser& parser = m_MeterWindow->GetParser();

	m_Color = parser.ReadColor(section, L"BarColor", Color::Green);

	m_ImageName = parser.ReadString(section, L"BarImage", L"");
	m_ImageName = m_MeterWindow->MakePathAbsolute(m_ImageName);

	m_Border = parser.ReadInt(section, L"BarBorder", 0);

	m_Flip = parser.ReadInt(section, L"Flip", 0) == 1;

	std::wstring orientation;
	orientation = parser.ReadString(section, L"BarOrientation", L"VERTICAL");

	if(_wcsicmp(L"VERTICAL", orientation.c_str()) == 0)
	{
		m_Orientation = VERTICAL;
	} 
	else if(_wcsicmp(L"HORIZONTAL", orientation.c_str()) == 0)
	{
		m_Orientation = HORIZONTAL;
	}
	else
	{
		throw CError(std::wstring(L"BarOrientation=") + orientation + L" is not valid in meter [" + m_Name + L"].", __LINE__, __FILE__);
	}

	if (m_Initialized)
	{
		if (oldImageName != m_ImageName)
		{
			Initialize();  // Reload the image
		}
		else if (!m_ImageName.empty())
		{
			// Reset to old dimensions
			m_W = oldW;
			m_H = oldH;
		}
	}
}

/*
** Update
**
** Updates the value(s) from the measures.
**
*/
bool CMeterBar::Update()
{
	if (CMeter::Update() && m_Measure)
	{
		m_Value = m_Measure->GetRelativeValue();
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
bool CMeterBar::Draw(Graphics& graphics)
{
	if(!CMeter::Draw(graphics)) return false;

	int x = GetX();
	int y = GetY();

	if(m_Orientation == VERTICAL)
	{
		int size = (int)((m_H - 2 * m_Border) * m_Value);
		size = min(m_H - 2 * m_Border, size);
		size = max(0, size);

		if (m_Bitmap)
		{
			if (m_Flip) 
			{
				if (m_Border > 0)
				{
					Rect r2(x, y, m_W, m_Border);
					graphics.DrawImage(m_Bitmap, r2, 0, 0, m_W, m_Border, UnitPixel);
					r2.Y = y + size + m_Border;
					graphics.DrawImage(m_Bitmap, r2, 0, m_H - m_Border, m_W, m_Border, UnitPixel);
				}

				// Blit the image
				Rect r(x, y + m_Border, m_W, size);
				graphics.DrawImage(m_Bitmap, r, 0, m_Border, m_W, size, UnitPixel);
			}
			else
			{
				if (m_Border > 0)
				{
					Rect r2(x, y + m_H - size - 2 * m_Border, m_W, m_Border);
					graphics.DrawImage(m_Bitmap, r2, 0, 0, m_W, m_Border, UnitPixel);
					r2.Y = y + m_H - m_Border;
					graphics.DrawImage(m_Bitmap, r2, 0, m_H - m_Border, m_W, m_Border, UnitPixel);
				}

				// Blit the image
				Rect r(x, y + m_H - size - m_Border, m_W, size);
				graphics.DrawImage(m_Bitmap, r, 0, m_H - size - m_Border, m_W, size, UnitPixel);
			}
		}
		else
		{
			SolidBrush brush(m_Color);
			if (m_Flip) 
			{
				Rect r(x, y, m_W, size);
				graphics.FillRectangle(&brush, r);
			}
			else
			{
				Rect r(x, y + m_H - size, m_W, size);
				graphics.FillRectangle(&brush, r);
			}
		}
	}
	else
	{
		int size = (int)((m_W - 2 * m_Border) * m_Value);
		size = min(m_W - 2 * m_Border, size);
		size = max(0, size);

		if (m_Bitmap)
		{
			if (m_Flip) 
			{
				if (m_Border > 0)
				{
					Rect r2(x + m_W - size - 2 * m_Border, y, m_Border, m_H);
					graphics.DrawImage(m_Bitmap, r2, 0, 0, m_Border, m_H, UnitPixel);
					r2.X = x + m_W - m_Border;
					graphics.DrawImage(m_Bitmap, r2, m_W - m_Border, 0, m_Border, m_H, UnitPixel);
				}

				// Blit the image
				Rect r(x + m_W - size - m_Border, y, size, m_H);
				graphics.DrawImage(m_Bitmap, r, m_W - size - m_Border, 0, size, m_H, UnitPixel);
			}
			else
			{
				if (m_Border > 0)
				{
					Rect r2(x, y, m_Border, m_H);
					graphics.DrawImage(m_Bitmap, r2, 0, 0, m_Border, m_H, UnitPixel);
					r2.X = x + size + m_Border;
					graphics.DrawImage(m_Bitmap, r2, m_W - m_Border, 0, m_Border, m_H, UnitPixel);
				}

				// Blit the image
				Rect r(x + m_Border, y, size, m_H);
				graphics.DrawImage(m_Bitmap, r, m_Border, 0, size, m_H, UnitPixel);
			}
		}
		else
		{
			SolidBrush brush(m_Color);
			if (m_Flip) 
			{
				Rect r(x + m_W - size, y, size, m_H);
				graphics.FillRectangle(&brush, r);
			}
			else
			{
				Rect r(x, y, size, m_H);
				graphics.FillRectangle(&brush, r);
			}
		}
	}

	return true;
}

