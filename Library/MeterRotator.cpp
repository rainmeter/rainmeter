/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeterRotator.h"
#include "Measure.h"
#include "Util.h"
#include "Rainmeter.h"
#include "../Common/Gfx/Canvas.h"

#define PI	(3.14159265358979323846)
#define CONVERT_TO_DEGREES(X)	((X) * (180.0 / PI))

MeterRotator::MeterRotator(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Image(L"ImageName", nullptr, false, skin),
	m_OffsetX(),
	m_OffsetY(),
	m_StartAngle(),
	m_RotationAngle(PI * 2.0),
	m_ValueRemainder(),
	m_Value()
{
}

MeterRotator::~MeterRotator()
{
}

/*
** Load the image.
**
*/
void MeterRotator::Initialize()
{
	Meter::Initialize();

	// Load the bitmaps if defined
	if (!m_ImageName.empty())
	{
		m_Image.LoadImage(m_ImageName);
	}
	else if (m_Image.IsLoaded())
	{
		m_Image.DisposeImage();
	}
}

/*
** Read the options specified in the ini file.
**
*/
void MeterRotator::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	// Store the current values so we know if the image needs to be updated
	std::wstring oldImageName = m_ImageName;

	Meter::ReadOptions(parser, section);

	m_ImageName = parser.ReadString(section, L"ImageName", L"");
	if (!m_ImageName.empty())
	{
		// Read tinting options
		m_Image.ReadOptions(parser, section);
	}

	m_OffsetX = parser.ReadFloat(section, L"OffsetX", 0.0);
	m_OffsetY = parser.ReadFloat(section, L"OffsetY", 0.0);
	m_StartAngle = parser.ReadFloat(section, L"StartAngle", 0.0);
	m_RotationAngle = parser.ReadFloat(section, L"RotationAngle", PI * 2.0);

	m_ValueRemainder = parser.ReadInt(section, L"ValueReminder", 0);		// Typo
	m_ValueRemainder = parser.ReadInt(section, L"ValueRemainder", m_ValueRemainder);

	if (m_Initialized)
	{
		Initialize();  // Reload the image
	}
}

/*
** Updates the value(s) from the measures.
**
*/
bool MeterRotator::Update()
{
	if (Meter::Update() && !m_Measures.empty())
	{
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
bool MeterRotator::Draw(Gfx::Canvas& canvas)
{
	if (!Meter::Draw(canvas)) return false;

	if (m_Image.IsLoaded())
	{
		// Calculate the center for rotation
		int x = GetX();
		int y = GetY();

		FLOAT cx = (FLOAT)(x + m_W / 2.0);
		FLOAT cy = (FLOAT)(y + m_H / 2.0);

		// Calculate the rotation
		FLOAT angle = (FLOAT)(CONVERT_TO_DEGREES(m_RotationAngle * m_Value + m_StartAngle));

		// TODO: convert to Canvas: canvas.RotateTransform(angle, cx, cy, (REAL)-m_OffsetX, (REAL)-m_OffsetY);
		// NOTE: canvas.RotateTransform does not work at all
		D2D1_MATRIX_3X2_F matrix = D2D1::Matrix3x2F::Translation((FLOAT)(-m_OffsetX), (FLOAT)(-m_OffsetY)) * 
			D2D1::Matrix3x2F::Rotation(angle) * 
			D2D1::Matrix3x2F::Translation(cx, cy);
		canvas.SetTransform(matrix);

		Gfx::D2DBitmap* drawBitmap = m_Image.GetImage();

		FLOAT width = (FLOAT)drawBitmap->GetWidth();
		FLOAT height = (FLOAT)drawBitmap->GetHeight();

		const D2D1_RECT_F rect = D2D1::RectF(0.0f, 0.0f, width, height);
		canvas.DrawBitmap(drawBitmap, rect, rect);

		canvas.ResetTransform();
	}

	return true;
}
