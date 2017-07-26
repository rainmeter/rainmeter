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
#include "../Common/Gfx/Shapes/Path.h"
#include "../Common/Gfx/Shapes/Ellipse.h"
#include "../Common/Gfx/Shapes/Line.h"

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
	m_LineStart = parser.ReadFloat(section, L"LineStart", 0.0);
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

	// Calculate the center of for the line
	int x = GetX();
	int y = GetY();

	double lineStart = ((m_CntrlLineStart) ? m_LineStartShift * m_Value : 0) + m_LineStart;
	double lineLength = ((m_CntrlLineLength) ? m_LineLengthShift * m_Value : 0) + m_LineLength;

	FLOAT cx = x + m_W / 2.0;
	FLOAT cy = y + m_H / 2.0;

	double angle = ((m_CntrlAngle) ? m_RotationAngle * m_Value : m_RotationAngle) + m_StartAngle;

	if(angle >= 2 * PI)
	{
		Gfx::Ellipse outer(cx, cy, lineLength, lineLength);
		Gfx::Ellipse inner(cx, cy, lineStart, lineStart);
		outer.CombineWith(&inner, D2D1_COMBINE_MODE_EXCLUDE);
		outer.SetFill(m_LineColor);
		outer.SetStrokeWidth(0);
		canvas.DrawGeometry(outer, 0, 0);
		return true;
	}

	FLOAT sox = cos(m_StartAngle) * lineLength + cx;
	FLOAT soy = sin(m_StartAngle) * lineLength + cy;

	FLOAT eox = cos(angle) * lineLength + cx;
	FLOAT eoy = sin(angle) * lineLength + cy;

	FLOAT eix = cos(angle) * lineStart + cx;
	FLOAT eiy = sin(angle) * lineStart + cy;

	FLOAT six = cos(m_StartAngle) * lineStart + cx;
	FLOAT siy = sin(m_StartAngle) * lineStart + cy;

	if (m_Solid)
	{
		Gfx::Path roundline(sox, soy, D2D1_FILL_MODE_ALTERNATE);
		roundline.SetFill(m_LineColor);
		roundline.SetStrokeWidth(0);

		if (lineLength > 0)
		{
			roundline.AddArc(eox, eoy, lineLength, lineLength, 0, m_RotationAngle > 0 ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
				abs(angle - m_StartAngle) < PI ? D2D1_ARC_SIZE_SMALL : D2D1_ARC_SIZE_LARGE);
		}
		else
		{
			roundline.AddLine(eox, eoy); // BWC
		}

		roundline.AddLine(eix, eiy);
		if (lineStart > 0)
		{
			roundline.AddArc(six, siy, lineStart, lineStart, 0, m_RotationAngle > 0 ? D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE : D2D1_SWEEP_DIRECTION_CLOCKWISE,
				abs(angle - m_StartAngle) < PI ? D2D1_ARC_SIZE_SMALL : D2D1_ARC_SIZE_LARGE);
		}
		else
		{
			roundline.AddLine(six, siy); // BWC
		}

		roundline.Close(D2D1_FIGURE_END_CLOSED);
		canvas.DrawGeometry(roundline, 0, 0);
	}
	else
	{
		Gfx::Line line(eix, eiy, eox, eoy);
		line.SetStrokeFill(m_LineColor);
		line.SetStrokeWidth(m_LineWidth);
		canvas.DrawGeometry(line, 0, 0);
	}

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
