/* Copyright (C) 2009 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeterRoundLine.h"
#include "Measure.h"
#include "../Common/Gfx/Canvas.h"

using namespace Gdiplus;

#define PI	(3.14159265358979323846)
#define CONVERT_TO_DEGREES(X)	((X) * (180.0 / PI))

MeterRoundLine::MeterRoundLine(Skin* skin, const WCHAR* name) : Meter(skin, name),
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

MeterRoundLine::~MeterRoundLine()
{
}

/*
** Read the options specified in the ini file.
**
*/
void MeterRoundLine::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Meter::ReadOptions(parser, section);

	m_LineWidth = parser.ReadFloat(section, L"LineWidth", 1.0);
	m_LineLength = parser.ReadFloat(section, L"LineLength", 20.0);
	m_LineStart = parser.ReadFloat(section, L"LineStart", -1.0);
	m_StartAngle = parser.ReadFloat(section, L"StartAngle", 0.0);
	m_RotationAngle = parser.ReadFloat(section, L"RotationAngle", 6.2832);
	m_ValueRemainder = parser.ReadInt(section, L"ValueReminder", 0);		// Typo
	m_ValueRemainder = parser.ReadInt(section, L"ValueRemainder", m_ValueRemainder);
	m_LineColor = parser.ReadColor(section, L"LineColor", Color::Black);
	m_Solid = parser.ReadBool(section, L"Solid", false);
	m_CntrlAngle = parser.ReadBool(section, L"ControlAngle", true);
	m_CntrlLineStart = parser.ReadBool(section, L"ControlStart", false);
	m_CntrlLineLength = parser.ReadBool(section, L"ControlLength", false);
	m_LineStartShift = parser.ReadFloat(section, L"StartShift", 0.0);
	m_LineLengthShift = parser.ReadFloat(section, L"LengthShift", 0.0);
}

/*
** Updates the value(s) from the measures.
**
*/
bool MeterRoundLine::Update()
{
	if (Meter::Update())
	{
		if (m_Measures.empty())
		{
			m_Value = 1.0;
			return true;
		}

		Measure* measure = m_Measures[0];
		if (m_ValueRemainder > 0)
		{
			LONGLONG time = (LONGLONG)measure->GetValue();
			m_Value = (double)(time % m_ValueRemainder);
			m_Value /= (double)m_ValueRemainder;
		}
		else
		{
			m_Value = measure->GetRelativeValue();
		}
		return true;
	}

	return false;
}


/*
** Draws the meter on the double buffer
**
*/
bool MeterRoundLine::Draw(Gfx::Canvas& canvas)
{
	if (!Meter::Draw(canvas)) return false;

	Gdiplus::Graphics& graphics = canvas.BeginGdiplusContext();

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
		REAL startAngle = (REAL)(fmod(CONVERT_TO_DEGREES(m_StartAngle), 360.0));
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

	canvas.EndGdiplusContext();

	return true;
}

/*
** Overridden method. The roundline meters need not to be bound on anything
**
*/
void MeterRoundLine::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	BindPrimaryMeasure(parser, section, true);
}
