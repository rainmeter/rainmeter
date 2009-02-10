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
/*
  $Header: /home/cvsroot/Rainmeter/Library/MeterBar.cpp,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeterBar.cpp,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.11  2004/07/11 17:16:11  rainy
  Added BarBorder.

  Revision 1.10  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.9  2003/02/10 18:12:45  rainy
  Now uses GDI+

  Revision 1.8  2002/07/01 15:32:51  rainy
  Removed include to lsapi.h

  Revision 1.7  2002/05/04 08:13:41  rainy
  Fixed vertical bar drawing.

  Revision 1.6  2002/04/26 18:22:03  rainy
  Added possibility to hide the meter.

  Revision 1.5  2002/03/31 09:58:54  rainy
  Added some comments

  Revision 1.4  2001/12/23 10:14:51  rainy
  Hex color values are now also supported.

  Revision 1.3  2001/10/14 07:32:33  rainy
  In error situations CError is thrown instead just a boolean value.

  Revision 1.2  2001/09/26 16:26:24  rainy
  Small adjustement to the interfaces.

  Revision 1.1  2001/09/01 12:56:25  rainy
  Initial version.


*/

#pragma warning(disable: 4786)
#pragma warning(disable: 4996)

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
		m_Bitmap = new Bitmap(m_ImageName.c_str());
		Status status = m_Bitmap->GetLastStatus();
		if(Ok != status)
		{
            throw CError(std::wstring(L"Bitmap image not found: ") + m_ImageName, __LINE__, __FILE__);
		}

		m_W = m_Bitmap->GetWidth();
		m_H = m_Bitmap->GetHeight();
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
	// Read common configs
	CMeter::ReadConfig(section);

	CConfigParser& parser = m_MeterWindow->GetParser();

	m_Color = parser.ReadColor(section, L"BarColor", Color::Green);

	m_ImageName = parser.ReadString(section, L"BarImage", L"");
	m_ImageName = Rainmeter->FixPath(m_ImageName, PATH_FOLDER_CURRENT_SKIN, m_MeterWindow->GetSkinName());

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
        throw CError(std::wstring(L"No such BarOrientation: ") + orientation, __LINE__, __FILE__);
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
bool CMeterBar::Draw()
{
	if(!CMeter::Draw()) return false;

	int x = GetX();
	int y = GetY();

	Graphics graphics(m_MeterWindow->GetDoubleBuffer());

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

