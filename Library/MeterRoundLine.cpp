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
/*
  $Header: /home/cvsroot/Rainmeter/Library/MeterRoundLine.cpp,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeterRoundLine.cpp,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.5  2004/08/13 15:46:50  rainy
  Reminder->Remainder.

  Revision 1.4  2004/07/11 17:18:07  rainy
  Relative coordinate support.

  Revision 1.3  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.2  2003/02/10 18:12:44  rainy
  Now uses GDI+

  Revision 1.1  2002/12/26 18:15:41  rainy
  Initial version

*/
#pragma warning(disable: 4996)

#include "MeterRoundLine.h"
#include "Measure.h"
#include "Error.h"
#include <crtdbg.h>
#include <math.h>
#include <gdiplus.h>

using namespace Gdiplus;
#define PI 3.14159265

/*
** CMeterRoundLine
**
** The constructor
**
*/
CMeterRoundLine::CMeterRoundLine(CMeterWindow* meterWindow) : CMeter(meterWindow)
{
	m_AntiAlias = false;
	m_LineWidth = 1.0;
	m_LineLength = 20;
	m_LineStart = -1.0;
	m_StartAngle = 0.0;
	m_RotationAngle = 0.0;
	m_ValueRemainder = 0;
	m_Solid = false;
	m_Value = 0.0;
}

/*
** ~CMeterRoundLine
**
** The destructor
**
*/
CMeterRoundLine::~CMeterRoundLine()
{
}

/*
** ReadConfig
**
** Read the meter-specific configs from the ini-file.
**
*/
void CMeterRoundLine::ReadConfig(const WCHAR* section)
{
	// Read common configs
	CMeter::ReadConfig(section);

	CConfigParser& parser = m_MeterWindow->GetParser();

	m_LineWidth = parser.ReadFloat(section, L"LineWidth", 1.0);
	m_LineLength = parser.ReadFloat(section, L"LineLength", 20.0);
	m_LineStart = parser.ReadFloat(section, L"LineStart", -1.0);
	m_StartAngle = parser.ReadFloat(section, L"StartAngle", 0.0);
	m_RotationAngle = parser.ReadFloat(section, L"RotationAngle", 6.2832);
	m_AntiAlias = 0!=parser.ReadInt(section, L"AntiAlias", 0);
	m_ValueRemainder = parser.ReadInt(section, L"ValueReminder", 0);		// Typo
	m_ValueRemainder = parser.ReadInt(section, L"ValueRemainder", m_ValueRemainder);
	m_LineColor = parser.ReadColor(section, L"LineColor", Color::Black);
	m_Solid = 0!=parser.ReadInt(section, L"Solid", 0);
}

/*
** Update
**
** Updates the value(s) from the measures.
**
*/
bool CMeterRoundLine::Update()
{
	if (CMeter::Update() && m_Measure)
	{
		if (m_ValueRemainder > 0)
		{
			LARGE_INTEGER time;
			time.QuadPart = m_Measure->GetValue();
			m_Value = time.QuadPart % m_ValueRemainder;
			m_Value /= (double)m_ValueRemainder;
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
bool CMeterRoundLine::Draw()
{
	if(!CMeter::Draw()) return false;

	Graphics graphics(m_MeterWindow->GetDoubleBuffer());		//GDI+
	if (m_AntiAlias)
	{
		graphics.SetSmoothingMode(SmoothingModeAntiAlias);
	}

	// Calculate the center of for the line
	int x = GetX();
	int y = GetY();
	REAL cx = (REAL)(x + m_W / 2.0);
	REAL cy = (REAL)(y + m_H / 2.0);

	if (m_Solid)
	{
		if (m_LineStart > 0.0)
		{
			// Create clipping region
			GraphicsPath path;
			path.AddEllipse(REAL(cx - m_LineStart), REAL(cy - m_LineStart), REAL(m_LineStart * 2), REAL(m_LineStart * 2));
			graphics.SetClip(&path, CombineModeExclude);
		}

		// Calculate the center of for the line
		SolidBrush solidBrush(m_LineColor);
		graphics.FillPie(&solidBrush, (REAL)(cx - m_LineLength), (REAL)(cy - m_LineLength), (REAL)(m_LineLength * 2.0), (REAL)(m_LineLength * 2.0), (REAL)(m_StartAngle * 180.0 / PI), (REAL)(m_RotationAngle * m_Value * 180.0 / PI));
	}
	else
	{
		REAL x, y;

		Pen pen(m_LineColor, (REAL)m_LineWidth);

		// Calculate the end point of the line
		double angle = m_RotationAngle * m_Value + m_StartAngle;

		x = (REAL)cos(angle);
		y = (REAL)sin(angle);

		// Set the length
		x = x * (REAL)m_LineLength + cx;
		y = y * (REAL)m_LineLength + cy;

		if (m_LineStart > 0.0)
		{
			cx = (REAL)cos(angle) * (REAL)m_LineStart + cx;
			cy = (REAL)sin(angle) * (REAL)m_LineStart + cy;
		}

		graphics.DrawLine(&pen, cx, cy, x, y);
	}

	return true;
}
