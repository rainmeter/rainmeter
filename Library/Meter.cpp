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
  $Header: /home/cvsroot/Rainmeter/Library/Meter.cpp,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: Meter.cpp,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.17  2004/07/11 17:15:56  rainy
  Added relative coordinates.

  Revision 1.16  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.15  2004/03/13 16:17:56  rainy
  Added rotator

  Revision 1.14  2003/02/10 18:12:45  rainy
  Now uses GDI+

  Revision 1.13  2002/12/23 14:25:45  rainy
  Fixed color reading.

  Revision 1.12  2002/07/01 15:33:00  rainy
  Added LINE meter.

  Revision 1.11  2002/05/04 08:12:51  rainy
  Actions can be defined per meter.

  Revision 1.10  2002/04/27 10:28:14  rainy
  Added an error message if the meter is not bound to anything.

  Revision 1.9  2002/04/26 18:22:38  rainy
  Added possibility to hide the meter.
  Added support for Image meter.

  Revision 1.8  2002/03/31 09:58:54  rainy
  Added some comments

  Revision 1.7  2001/12/23 10:15:25  rainy
  Added ParseColor().

  Revision 1.6  2001/10/14 07:32:15  rainy
  In error situations CError is thrown instead just a boolean value.

  Revision 1.5  2001/09/26 16:26:53  rainy
  Changed the interfaces a bit.

  Revision 1.4  2001/09/01 12:59:16  rainy
  Added support for Uptime measure.
  W and H default to 1.

  Revision 1.3  2001/08/19 09:13:38  rainy
  Invert moved to the measures.
  Added PerfMon measure.

  Revision 1.2  2001/08/12 15:41:41  Rainy
  Adjusted Update()'s interface.
  Added invert measure.

  Revision 1.1.1.1  2001/08/11 10:58:19  Rainy
  Added to CVS.

*/
#pragma warning(disable: 4996)

#include "Error.h"
#include "Meter.h"
#include "MeterBitmap.h"
#include "MeterBar.h"
#include "MeterHistogram.h"
#include "MeterString.h"
#include "MeterImage.h"
#include "MeterLine.h"
#include "MeterRoundLine.h"
#include "MeterRotator.h"
#include "MeterButton.h"
#include "Measure.h"
#include "Rainmeter.h"

using namespace Gdiplus;

int CMeter::c_OldX = 0;
int CMeter::c_OldY = 0;

/*
** CMeter
**
** The constructor
**
*/
CMeter::CMeter(CMeterWindow* meterWindow)
{
	m_Measure = NULL;
	m_X = 0;
	m_Y = 0;
	m_W = 0;
	m_H = 0;
	m_RelativeMeter = NULL;
	m_Hidden = false;
	m_SolidBevel = BEVELTYPE_NONE;
	m_MouseOver = false;
	m_UpdateDivider = 1;
	m_UpdateCounter = 0;
	m_RelativeX = POSITION_ABSOLUTE;
	m_RelativeY = POSITION_ABSOLUTE;
	m_MeterWindow = NULL;
	m_SolidAngle = 0.0;
	m_MeterWindow = meterWindow;
}

/*
** ~CMeter
**
** The destructor
**
*/
CMeter::~CMeter()
{
}

/*
** Initialize
**
** Initializes the meter. The base implementation just stores the pointer.
** Usually this method is overwritten by the inherited classes, which load
** bitmaps and such things during initialization.
**
*/
void CMeter::Initialize()
{
}

/*
** GetX
**
** Returns the X-position of the meter.
**
*/
int CMeter::GetX(bool abs)
{
	if (m_RelativeX != POSITION_ABSOLUTE && m_MeterWindow)
	{
		if (m_RelativeMeter == NULL)
		{
			std::list<CMeter*>& meters = m_MeterWindow->GetMeters();
			std::list<CMeter*>::iterator iter = meters.begin();

			// Find this meter
			for ( ; iter != meters.end(); iter++)
			{
				if (*iter == this && iter != meters.begin())
				{
					iter--;
					m_RelativeMeter = (*iter);
					if (m_RelativeX == POSITION_RELATIVE_TL)
					{
						return m_RelativeMeter->GetX(true) + m_X;
					}
					else
					{
						return m_RelativeMeter->GetX(true) + m_RelativeMeter->GetW() + m_X;
					}
				}
			}
		}
		else
		{
			if (m_RelativeX == POSITION_RELATIVE_TL)
			{
				return m_RelativeMeter->GetX(true) + m_X;
			}
			else
			{
				return m_RelativeMeter->GetX(true) + m_RelativeMeter->GetW() + m_X;
			}
		}
	}
	return m_X; 
}

/*
** GetY
**
** Returns the Y-position of the meter.
**
*/
int CMeter::GetY(bool abs)
{
	if (m_RelativeY != POSITION_ABSOLUTE && m_MeterWindow)
	{
		if (m_RelativeMeter == NULL)
		{
			std::list<CMeter*>& meters = m_MeterWindow->GetMeters();
			std::list<CMeter*>::iterator iter = meters.begin();

			// Find this meter
			for ( ; iter != meters.end(); iter++)
			{
				if (*iter == this && iter != meters.begin())
				{
					iter--;
					m_RelativeMeter = (*iter);
					if (m_RelativeY == POSITION_RELATIVE_TL)
					{
						return m_RelativeMeter->GetY() + m_Y;
					}
					else
					{
						return m_RelativeMeter->GetY() + m_RelativeMeter->GetH() + m_Y;
					}
				}
			}
		}
		else
		{
			if (m_RelativeY == POSITION_RELATIVE_TL)
			{
				return m_RelativeMeter->GetY() + m_Y;
			}
			else
			{
				return m_RelativeMeter->GetY() + m_RelativeMeter->GetH() + m_Y;
			}
		}
	}
	return m_Y; 
}

/*
** HitTest
**
** Checks if the given point is inside the meter.
**
*/
bool CMeter::HitTest(int x, int y)
{
	if (x >= GetX() && x <= GetX() + GetW() && y >= GetY() && y <= GetY() + GetH())
	{
		return true;
	}
	return false;
}

/*
** ReadConfig
**
** Reads the meter-specific configs from the ini-file. The base implementation
** reads the common settings for all meters. The inherited classes must call 
** the base implementation if they overwrite this method.
**
*/
void CMeter::ReadConfig(const WCHAR* section)
{
	CConfigParser& parser = m_MeterWindow->GetParser();

	const std::wstring& x = parser.ReadString(section, L"X", L"0");
	if (x.size() > 0)
	{
		m_X = _wtoi(x.c_str());
		if (x[x.size() - 1] == L'r')
		{
			m_RelativeX = POSITION_RELATIVE_TL;
		}
		else if (x[x.size() - 1] == L'R')
		{
			m_RelativeX = POSITION_RELATIVE_BR;
		}
	}

	const std::wstring& y = parser.ReadString(section, L"Y", L"0");
	if (y.size() > 0)
	{
		m_Y = _wtoi(y.c_str());
		if (y[y.size() - 1] == L'r')
		{
			m_RelativeY = POSITION_RELATIVE_TL;
		}
		else if (y[y.size() - 1] == L'R')
		{
			m_RelativeY = POSITION_RELATIVE_BR;
		}
	}

	m_W = parser.ReadInt(section, L"W", 1);
	m_H = parser.ReadInt(section, L"H", 1);

	m_Hidden = 0!=parser.ReadInt(section, L"Hidden", 0);
	m_SolidBevel = (BEVELTYPE)parser.ReadInt(section, L"BevelType", m_SolidBevel);

	m_SolidColor = parser.ReadColor(section, L"SolidColor", Color(0, 0, 0, 0));
	m_SolidColor2 = parser.ReadColor(section, L"SolidColor2", m_SolidColor);
	m_SolidAngle = (Gdiplus::REAL)parser.ReadFloat(section, L"GradientAngle", 0.0);

	m_RightMouseDownAction = parser.ReadString(section, L"RightMouseDownAction", L"");
	m_LeftMouseDownAction = parser.ReadString(section, L"LeftMouseDownAction", L"");
	m_RightMouseUpAction = parser.ReadString(section, L"RightMouseUpAction", L"");
	m_LeftMouseUpAction = parser.ReadString(section, L"LeftMouseUpAction", L"");
	m_MouseOverAction = parser.ReadString(section, L"MouseOverAction", L"");
	m_MouseLeaveAction = parser.ReadString(section, L"MouseLeaveAction", L"");

	m_MeasureName = parser.ReadString(section, L"MeasureName", L"");

	m_UpdateDivider = parser.ReadInt(section, L"UpdateDivider", 1);
	m_UpdateCounter = m_UpdateDivider;

	if (m_W == 0 || m_H == 0)
	{
        throw CError(std::wstring(L"The meter ") + section + L" has zero dimensions.", __LINE__, __FILE__);
	}
}

/*
** BindMeasure
**
** Binds this meter to the given measure. The same measure can be bound to
** several meters but one meter and only be bound to one measure.
**
*/
void CMeter::BindMeasure(std::list<CMeasure*>& measures)
{
	// The meter is not bound to anything
	if (m_MeasureName.empty())
	{
	    throw CError(std::wstring(L"The meter [") + m_Name + L"] is not bound to anything!", __LINE__, __FILE__);
	}

	// Go through the list and check it there is a measure for us
	std::list<CMeasure*>::iterator i = measures.begin();
	for( ; i != measures.end(); i++)
	{
		if(_wcsicmp((*i)->GetName(), m_MeasureName.c_str()) == 0)
		{
			m_Measure = (*i);
			return;
		}
	}

    // Error :)
    throw CError(std::wstring(L"The meter [") + m_Name + L"] cannot be bound with [" + m_MeasureName + L"]!", __LINE__, __FILE__);
}

/*
** Create
**
** Creates the given meter. This is the factory method for the meters.
** If new meters are implemented this method needs to be updated.
**
*/
CMeter* CMeter::Create(const WCHAR* meter, CMeterWindow* meterWindow)
{
	if(_wcsicmp(L"HISTOGRAM", meter) == 0)
	{
		return new CMeterHistogram(meterWindow);
	} 
	else if(_wcsicmp(L"STRING", meter) == 0)
	{
		return new CMeterString(meterWindow);
	} 
	else if(_wcsicmp(L"BAR", meter) == 0)
	{
		return new CMeterBar(meterWindow);
	} 
	else if(_wcsicmp(L"BITMAP", meter) == 0)
	{
		return new CMeterBitmap(meterWindow);
	} 
	else if(_wcsicmp(L"IMAGE", meter) == 0)
	{
		return new CMeterImage(meterWindow);
	} 
	else if(_wcsicmp(L"LINE", meter) == 0)
	{
		return new CMeterLine(meterWindow);
	} 
	else if(_wcsicmp(L"ROUNDLINE", meter) == 0)
	{
		return new CMeterRoundLine(meterWindow);
	} 
	else if(_wcsicmp(L"ROTATOR", meter) == 0)
	{
		return new CMeterRotator(meterWindow);
	} 
	else if(_wcsicmp(L"BUTTON", meter) == 0)
	{
		return new CMeterButton(meterWindow);
	} 

    // Error
    throw CError(std::wstring(L"No such meter: ") + meter, __LINE__, __FILE__);

	return NULL;
}

/*
** Update
**
** Updates the value(s) from the measures. Derived classes should
** only update if this returns true;
*/
bool CMeter::Update()
{
	// Only update the meter's value when the divider is equal to the counter
	m_UpdateCounter++;
	if (m_UpdateCounter < m_UpdateDivider) return false;
	m_UpdateCounter = 0;

	return true;
}

/*
** Draw
**
** Draws the solid background & bevel if such are defined
*/
bool CMeter::Draw()
{
	if (IsHidden()) return false;

	if (m_SolidColor.GetA() != 0 || m_SolidColor2.GetA() != 0)
	{
		Graphics graphics(m_MeterWindow->GetDoubleBuffer());

		int x = GetX();
		int y = GetY();

		if (m_SolidColor.GetValue() == m_SolidColor2.GetValue())
		{
			SolidBrush solid(m_SolidColor);
			graphics.FillRectangle(&solid, x, y, m_W, m_H);
		}
		else
		{
			Rect r(x, y, m_W, m_H);
			LinearGradientBrush gradient(r, m_SolidColor, m_SolidColor2, m_SolidAngle, TRUE);
			graphics.FillRectangle(&gradient, r);
		}
	}

	if (m_SolidBevel != BEVELTYPE_NONE)
	{
		Graphics graphics(m_MeterWindow->GetDoubleBuffer());

		int x = GetX();
		int y = GetY();

		Pen light(Color(255, 255, 255, 255));
		Pen dark(Color(255, 0, 0, 0));

		if (m_SolidBevel == BEVELTYPE_DOWN)
		{
			light.SetColor(Color(255, 0, 0, 0));
			dark.SetColor(Color(255, 255, 255, 255));
		}

		// The bevel is drawn outside the meter
		Rect rect(x - 2, y - 2, m_W + 4, m_H + 4);	
		DrawBevel(graphics, rect, light, dark);
	}

	return true;
}

/*
** DrawBevel
**
** Draws a bevel inside the given area
*/
void CMeter::DrawBevel(Graphics& graphics, Rect& rect, Pen& light, Pen& dark)
{
	int l = rect.GetLeft();
	int r = rect.GetRight() - 1;
	int t = rect.GetTop();
	int b = rect.GetBottom() - 1;

	graphics.DrawLine(&light, l,     t,     l,     b);
	graphics.DrawLine(&light, l,     t,     r,     t);
	graphics.DrawLine(&light, l + 1, t + 1, l + 1, b - 1);
	graphics.DrawLine(&light, l + 1, t + 1, r - 1, l + 1);
	graphics.DrawLine(&dark,  l,     b,     r,     b);
	graphics.DrawLine(&dark,  r,     t,     r,     b);
	graphics.DrawLine(&dark,  l + 1, b - 1, r - 1, b - 1);
	graphics.DrawLine(&dark,  r - 1, t + 1, r - 1, b - 1);
}
