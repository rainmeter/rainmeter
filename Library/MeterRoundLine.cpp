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

#define PI	(3.14159265358979323846)
#define CONVERT_TO_DEGREES(X)	((X) * (180.0 / PI))

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
	const FLOAT w = (FLOAT)m_W;
	const FLOAT h = (FLOAT)m_H;

	// Calculate the center of for the line
	const FLOAT cx = x + m_W / 2.0f;
	const FLOAT cy = y + m_H / 2.0f;

	const FLOAT lineStart = (FLOAT)(((m_CntrlLineStart) ? m_LineStartShift * m_Value : 0.0) + m_LineStart);
	const FLOAT lineLength = (FLOAT)(((m_CntrlLineLength) ? m_LineLengthShift * m_Value : 0.0) + m_LineLength);

	const FLOAT calculatedAngle = (FLOAT)(m_RotationAngle * m_Value);
	const FLOAT angle = calculatedAngle + (FLOAT)m_StartAngle;

	const FLOAT s_cos = cos((FLOAT)m_StartAngle);
	const FLOAT s_sin = sin((FLOAT)m_StartAngle);
	const FLOAT e_cos = cos((FLOAT)angle);
	const FLOAT e_sin = sin((FLOAT)angle);

	const FLOAT sOuterX = s_cos * lineLength + cx;
	const FLOAT sOuterY = s_sin * lineLength + cy;
	const FLOAT eOuterX = e_cos * lineLength + cx;
	const FLOAT eOuterY = e_sin * lineLength + cy;
	const FLOAT sInnerX = s_cos * lineStart + cx;
	const FLOAT sInnerY = s_sin * lineStart + cy;
	const FLOAT eInnerX = e_cos * lineStart + cx;
	const FLOAT eInnerY = e_sin * lineStart + cy;

	const D2D1_SWEEP_DIRECTION eSweep = m_RotationAngle > 0.0 ?
		D2D1_SWEEP_DIRECTION_CLOCKWISE :
		D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;

	const D2D1_ARC_SIZE arcSize = abs(calculatedAngle) < PI ?
		D2D1_ARC_SIZE_SMALL :
		D2D1_ARC_SIZE_LARGE;

	if (m_CntrlAngle)
	{
		// Special processing for 'out of bounds' angles
		if (abs(calculatedAngle) >= (2.0f * PI))
		{
			Gfx::Ellipse outer(cx, cy, lineLength, lineLength);
			Gfx::Ellipse inner(cx, cy, lineStart, lineStart);
			outer.CombineWith(&inner, D2D1_COMBINE_MODE_XOR);
			outer.SetFill(m_LineColor);
			outer.SetStrokeWidth(0.0f);
			canvas.DrawGeometry(outer, 0, 0);
			return true;
		}

		if (m_Solid)
		{
			const D2D1_SWEEP_DIRECTION sSweep = m_RotationAngle > 0.0 ?
				D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE :
				D2D1_SWEEP_DIRECTION_CLOCKWISE;

			Gfx::Path roundline(sOuterX, sOuterY, D2D1_FILL_MODE_ALTERNATE);
			roundline.SetFill(m_LineColor);
			roundline.SetStrokeWidth(0.0f);

			if (lineLength > 0.0f)
			{
				roundline.AddArc(
					eOuterX,
					eOuterY,
					lineLength,
					lineLength,
					0.0f,
					eSweep,
					arcSize);
			}
			else
			{
				roundline.AddLine(eOuterX, eOuterY);
			}

			roundline.AddLine(eInnerX, eInnerY);

			if (lineStart > 0.0f)
			{
				roundline.AddArc(
					sInnerX,
					sInnerY,
					lineStart,
					lineStart,
					0.0f,
					sSweep,
					arcSize);
			}
			else
			{
				roundline.AddLine(sInnerX, sInnerY);
			}

			roundline.Close(D2D1_FIGURE_END_CLOSED);
			canvas.DrawGeometry(roundline, 0, 0);
		}
		else
		{
			Gfx::Line line(eInnerX, eInnerY, eOuterX, eOuterY);
			line.SetStrokeFill(m_LineColor);
			line.SetStrokeWidth((FLOAT)m_LineWidth);
			canvas.DrawGeometry(line, 0, 0);
		}
	}
	else
	{
		Gfx::Path roundlineOuter(sOuterX, sOuterY, D2D1_FILL_MODE_ALTERNATE);
		Gfx::Path roundlineInner(sInnerX, sInnerY, D2D1_FILL_MODE_ALTERNATE);
		roundlineOuter.SetFill(m_LineColor);
		roundlineOuter.SetStrokeWidth(0.0f);

		if (lineLength > 0.0f)
		{
			roundlineOuter.AddArc(
				eOuterX,
				eOuterY,
				lineLength,
				lineLength,
				0.0f,
				eSweep,
				arcSize);
		}
		else
		{
			roundlineOuter.AddLine(eOuterX, eOuterY);
		}

		if (lineStart > 0.0f)
		{
			roundlineInner.AddArc(
				eInnerX,
				eInnerY,
				lineStart,
				lineStart,
				0.0f,
				eSweep,
				arcSize);
		}
		else
		{
			roundlineInner.AddLine(eInnerX, eInnerY);
		}

		roundlineOuter.Close(D2D1_FIGURE_END_CLOSED);
		roundlineInner.Close(D2D1_FIGURE_END_CLOSED);
		roundlineOuter.CombineWith(&roundlineInner, D2D1_COMBINE_MODE_XOR);
		canvas.DrawGeometry(roundlineOuter, 0, 0);
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
