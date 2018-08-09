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

#define PI	(3.14159265358979323846f)
#define CONVERT_TO_DEGREES(X)	((FLOAT)((X) * (180.0f / PI)))

MeterRoundLine::MeterRoundLine(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Solid(false),
	m_LineWidth(1.0),
	m_LineLength(20.0),
	m_LineStart(-1.0),
	m_StartAngle(0.0),
	m_RotationAngle(6.2832),
	m_CntrlAngle(true),
	m_CntrlLineStart(false),
	m_CntrlLineLength(false),
	m_LineStartShift(0.0),
	m_LineLengthShift(0.0),
	m_ValueRemainder(0U),
	m_LineColor(D2D1::ColorF(D2D1::ColorF::Black)),
	m_Value(0.0)
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
	m_ValueRemainder = parser.ReadUInt(section, L"ValueReminder", 0U);		// Typo
	m_ValueRemainder = parser.ReadUInt(section, L"ValueRemainder", m_ValueRemainder);
	m_LineColor = parser.ReadColor(section, L"LineColor", D2D1::ColorF(D2D1::ColorF::Black));
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

	const FLOAT x = (FLOAT)GetX();
	const FLOAT y = (FLOAT)GetY();

	// Calculate the center of for the line
	const FLOAT cx = x + (FLOAT)m_W / 2.0f;
	const FLOAT cy = y + (FLOAT)m_H / 2.0f;

	const FLOAT lineStart = (FLOAT)(((m_CntrlLineStart) ? m_LineStartShift * m_Value : 0.0) + m_LineStart);
	const FLOAT lineLength = (FLOAT)(((m_CntrlLineLength) ? m_LineLengthShift * m_Value : 0.0) + m_LineLength);

	const FLOAT angle = (FLOAT)(((m_CntrlAngle) ? m_RotationAngle * m_Value : m_RotationAngle) + m_StartAngle);
	const FLOAT e_cos = std::cos(angle);
	const FLOAT e_sin = std::sin(angle);

	const FLOAT sx = e_cos * lineStart + cx;
	const FLOAT sy = e_sin * lineStart + cy;
	const FLOAT ex = e_cos * lineLength + cx;
	const FLOAT ey = e_sin * lineLength + cy;

	if (m_Solid)
	{
		const FLOAT sweepAngle = std::fmodf(CONVERT_TO_DEGREES(m_RotationAngle * m_Value), 360.0f);

		const D2D1_SWEEP_DIRECTION sweepInnerDir = sweepAngle > 0.0f ?
			D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE : D2D1_SWEEP_DIRECTION_CLOCKWISE;

		const D2D1_SWEEP_DIRECTION sweepOuterDir = (D2D1_SWEEP_DIRECTION)(1 - sweepInnerDir);

		const D2D1_ARC_SIZE arcSize = std::abs(sweepAngle) < 180.0f ?
			D2D1_ARC_SIZE_SMALL : D2D1_ARC_SIZE_LARGE;

		const FLOAT s_cos = std::cos((FLOAT)m_StartAngle + 0.00001f);  // Offset angle in case drawing points are too close to each other
		const FLOAT s_sin = std::sin((FLOAT)m_StartAngle + 0.00001f);

		const FLOAT ix = lineStart * s_cos + cx;
		const FLOAT iy = lineStart * s_sin + cy;
		const FLOAT ox = lineLength * s_cos + cx;
		const FLOAT oy = lineLength * s_sin + cy;

		Gfx::Path path(ix, iy, D2D1_FILL_MODE_ALTERNATE);
		path.SetFill(m_LineColor);
		path.SetStrokeWidth(0.0f);

		path.AddLine(ox, oy);
		path.AddArc(ex, ey, lineLength, lineLength, sweepAngle, sweepOuterDir, arcSize);
		path.AddLine(sx, sy);
		path.AddArc(ix, iy, lineStart, lineStart, sweepAngle, sweepInnerDir, arcSize);

		path.Close(D2D1_FIGURE_END_CLOSED);
		canvas.DrawGeometry(path, 0, 0);
	}
	else
	{
		Gfx::Line line(sx, sy, ex, ey);
		line.SetStrokeFill(m_LineColor);
		line.SetStrokeWidth((FLOAT)m_LineWidth);
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
