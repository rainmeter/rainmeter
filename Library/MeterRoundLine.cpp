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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "MeterRoundLine.h"
#include "Measure.h"
#include "Error.h"

using namespace Gdiplus;

#define PI	(3.14159265358979323846)
#define CONVERT_TO_DEGREES(X)	((X) * (180.0 / PI))

/*
** CMeterRoundLine
**
** The constructor
**
*/
CMeterRoundLine::CMeterRoundLine(CMeterWindow* meterWindow, const WCHAR* name) : CMeter(meterWindow, name),
	m_Solid(false),
	m_LineWidth(1.0),
	m_LineLength(20.0),
	m_LineStart(-1.0),
	m_StartAngle(),
	m_RotationAngle(6.2832),
	m_CntrlAngle(true),
	m_CntrlLineStart(false),
	m_CntrlLineLength(false),
	m_LineStartShift(),
	m_LineLengthShift(),
	m_ValueRemainder(),
	m_LineColor(Color::Black),
	m_Value()
{
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
void CMeterRoundLine::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	// Read common configs
	CMeter::ReadConfig(parser, section);

	m_LineWidth = parser.ReadFloat(section, L"LineWidth", 1.0);
	m_LineLength = parser.ReadFloat(section, L"LineLength", 20.0);
	m_LineStart = parser.ReadFormula(section, L"LineStart", -1.0);
	m_StartAngle = parser.ReadFloat(section, L"StartAngle", 0.0);
	m_RotationAngle = parser.ReadFloat(section, L"RotationAngle", 6.2832);
	m_ValueRemainder = parser.ReadInt(section, L"ValueReminder", 0);		// Typo
	m_ValueRemainder = parser.ReadInt(section, L"ValueRemainder", m_ValueRemainder);
	m_LineColor = parser.ReadColor(section, L"LineColor", Color::Black);
	m_Solid = 0!=parser.ReadInt(section, L"Solid", 0);
	m_CntrlAngle = 0!=parser.ReadInt(section, L"ControlAngle", 1);
	m_CntrlLineStart = 0!=parser.ReadInt(section, L"ControlStart", 0);
	m_CntrlLineLength = 0!=parser.ReadInt(section, L"ControlLength", 0);
	m_LineStartShift = parser.ReadFloat(section, L"StartShift", 0.0);
	m_LineLengthShift = parser.ReadFloat(section, L"LengthShift", 0.0);
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
			LONGLONG time = (LONGLONG)m_Measure->GetValue();
			m_Value = (double)(time % m_ValueRemainder);
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
	if (!CMeter::Draw(graphics)) return false;

	// Calculate the center of for the line
	int x = GetX();
	int y = GetY();
	double cx = x + m_W / 2.0;
	double cy = y + m_H / 2.0;

	double lineStart = ((m_CntrlLineStart) ? m_LineStartShift * m_Value : 0) + m_LineStart;
	double lineLength = ((m_CntrlLineLength) ? m_LineLengthShift * m_Value : 0) + m_LineLength;

	// Calculate the end point of the line
	double angle = ((m_CntrlAngle) ? m_RotationAngle * m_Value : m_RotationAngle) + m_StartAngle;
	double e_cos = cos(angle);
	double e_sin = sin(angle);

	REAL sx = (REAL)(e_cos * lineStart + cx);
	REAL sy = (REAL)(e_sin * lineStart + cy);
	REAL ex = (REAL)(e_cos * lineLength + cx);
	REAL ey = (REAL)(e_sin * lineLength + cy);

	if (m_Solid)
	{
		REAL startAngle = (REAL)(CONVERT_TO_DEGREES(m_StartAngle));
		REAL sweepAngle = (REAL)(CONVERT_TO_DEGREES(m_RotationAngle * m_Value));

		// Calculate the start point of the line
		double s_cos = cos(m_StartAngle);
		double s_sin = sin(m_StartAngle);

		//Create a path to surround the arc
		GraphicsPath path;
		path.AddArc((REAL)(cx - lineStart), (REAL)(cy - lineStart), (REAL)(lineStart * 2.0), (REAL)(lineStart * 2.0), startAngle, sweepAngle);
		path.AddLine((REAL)(lineStart * s_cos + cx), (REAL)(lineStart * s_sin + cy), (REAL)(lineLength * s_cos + cx), (REAL)(lineLength * s_sin + cy));
		path.AddArc((REAL)(cx - lineLength), (REAL)(cy - lineLength), (REAL)(lineLength * 2.0), (REAL)(lineLength * 2.0), startAngle, sweepAngle);
		path.AddLine(ex, ey, sx, sy);

		SolidBrush solidBrush(m_LineColor);
		graphics.FillPath(&solidBrush, &path);
	}
	else
	{
		Pen pen(m_LineColor, (REAL)m_LineWidth);
		graphics.DrawLine(&pen, sx, sy, ex, ey);
	}

	return true;
}
