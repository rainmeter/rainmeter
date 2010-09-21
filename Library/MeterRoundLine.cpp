/*
  Copyright (C) 2009 Kimmo Pekkola, Brian Todoroff

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
#include "MeterRoundLine.h"
#include "Measure.h"
#include "Error.h"

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
	m_LineWidth = 1.0;
	m_LineLength = 20;
	m_LineStart = -1.0;
	m_StartAngle = 0.0;
	m_RotationAngle = 0.0;
	m_ValueRemainder = 0;
	m_Solid = false;
	m_Value = 0.0;
	m_CntrlAngle = true;
	m_CntrlLineStart = false;
	m_CntrlLineLength = false;
	m_LineStartShift = 0;
	m_LineLengthShift = 0;
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
	m_ValueRemainder = parser.ReadInt(section, L"ValueReminder", 0);		// Typo
	m_ValueRemainder = parser.ReadInt(section, L"ValueRemainder", m_ValueRemainder);
	m_LineColor = parser.ReadColor(section, L"LineColor", Color::Black);
	m_Solid = 0!=parser.ReadInt(section, L"Solid", 0);
	m_CntrlAngle = 0!=parser.ReadInt(section, L"ControlAngle", 1);
	m_CntrlLineStart = 0!=parser.ReadInt(section, L"ControlStart", 0);
	m_CntrlLineLength = 0!=parser.ReadInt(section, L"ControlLength", 0);
	m_LineStartShift = parser.ReadFloat(section, L"StartShift", 0);
	m_LineLengthShift = parser.ReadFloat(section, L"LengthShift", 0);
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
			time.QuadPart = (LONGLONG)m_Measure->GetValue();
			m_Value = (double)(time.QuadPart % m_ValueRemainder);
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
bool CMeterRoundLine::Draw(Graphics& graphics)
{
	if(!CMeter::Draw(graphics)) return false;

	// Calculate the center of for the line
	int x = GetX();
	int y = GetY();
	REAL cx = (REAL)(x + m_W / 2.0);
	REAL cy = (REAL)(y + m_H / 2.0);

	if (m_Solid)
	{
		if (1) //m_LineStart > 0.0)
		{
			// Create clipping region
			//GraphicsPath path;
			//path.AddEllipse(REAL(cx - m_LineStart), REAL(cy - m_LineStart), REAL(m_LineStart * 2), REAL(m_LineStart * 2));
			//graphics.SetClip(&path, CombineModeExclude);

			// Calculate the end point of the line
			//double angle = m_RotationAngle * m_Value + m_StartAngle;
			double angle = m_RotationAngle + m_StartAngle;
			if(m_CntrlAngle)
				angle = m_RotationAngle * m_Value + m_StartAngle;
			double lineStart = m_LineStart;
			if(m_CntrlLineStart)
			{
				lineStart = m_LineStartShift * m_Value + m_LineStart;
			}
			double lineLength = m_LineLength;
			if(m_CntrlLineLength)
			{
				lineLength = m_LineLengthShift * m_Value + m_LineLength;
			}
			
			SolidBrush solidBrush(m_LineColor);

			//Create a path to surround the arc
			GraphicsPath path;
			path.AddArc((REAL)(cx - lineStart), (REAL)(cy - lineStart), (REAL)(lineStart * 2.0), (REAL)(lineStart * 2.0), (REAL)(m_StartAngle * 180.0 / PI), (REAL)(m_RotationAngle * m_Value * 180.0 / PI));
			path.AddLine((REAL)lineStart*(REAL)cos(m_StartAngle)+cx,(REAL)lineStart*(REAL)sin(m_StartAngle)+cy,(REAL)lineLength*(REAL)cos(m_StartAngle)+cx,(REAL)lineLength*(REAL)sin(m_StartAngle)+cy);
			path.AddArc((REAL)(cx - lineLength), (REAL)(cy - lineLength), (REAL)(lineLength * 2.0), (REAL)(lineLength * 2.0), (REAL)(m_StartAngle * 180.0 / PI), (REAL)(m_RotationAngle * m_Value * 180.0 / PI));
			path.AddLine((REAL)lineLength*(REAL)cos(angle)+cx,(REAL)lineLength*(REAL)sin(angle)+cy,(REAL)lineStart*(REAL)cos(angle)+cx,(REAL)lineStart*(REAL)sin(angle)+cy);
	
			graphics.FillPath(&solidBrush,&path);
		}
		else
		{
			// Calculate the center of for the line
			SolidBrush solidBrush(m_LineColor);
			graphics.FillPie(&solidBrush, (REAL)(cx - m_LineLength), (REAL)(cy - m_LineLength), (REAL)(m_LineLength * 2.0), (REAL)(m_LineLength * 2.0), (REAL)(m_StartAngle * 180.0 / PI), (REAL)(m_RotationAngle * m_Value * 180.0 / PI));
		}
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
