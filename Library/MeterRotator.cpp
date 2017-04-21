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

using namespace Gdiplus;

#define PI	(3.14159265358979323846)
#define CONVERT_TO_DEGREES(X)	((X) * (180.0 / PI))

MeterRotator::MeterRotator(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Image(L"ImageName", nullptr, false, skin),
	m_NeedsReload(false),
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
		m_Image.LoadImage(m_ImageName, m_NeedsReload);
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
	else
	{
		m_Image.ClearOptionFlags();
	}

	m_OffsetX = parser.ReadFloat(section, L"OffsetX", 0.0);
	m_OffsetY = parser.ReadFloat(section, L"OffsetY", 0.0);
	m_StartAngle = parser.ReadFloat(section, L"StartAngle", 0.0);
	m_RotationAngle = parser.ReadFloat(section, L"RotationAngle", PI * 2.0);

	m_ValueRemainder = parser.ReadInt(section, L"ValueReminder", 0);		// Typo
	m_ValueRemainder = parser.ReadInt(section, L"ValueRemainder", m_ValueRemainder);

	if (m_Initialized)
	{
		m_NeedsReload = (wcscmp(oldImageName.c_str(), m_ImageName.c_str()) != 0);

		if (m_NeedsReload ||
			m_Image.IsOptionsChanged())
		{
			Initialize();  // Reload the image
		}
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

	Gdiplus::Graphics& graphics = canvas.BeginGdiplusContext();

	if (m_Image.IsLoaded())
	{
		// Calculate the center for rotation
		int x = GetX();
		int y = GetY();

		REAL cx = (REAL)(x + m_W / 2.0);
		REAL cy = (REAL)(y + m_H / 2.0);

		// Calculate the rotation
		REAL angle = (REAL)(CONVERT_TO_DEGREES(m_RotationAngle * m_Value + m_StartAngle));

		// TODO: convert to Canvas: canvas.RotateTransform(angle, cx, cy, (REAL)-m_OffsetX, (REAL)-m_OffsetY);
		graphics.TranslateTransform(cx, cy);
		graphics.RotateTransform(angle);
		graphics.TranslateTransform((REAL)-m_OffsetX, (REAL)-m_OffsetY);

		Bitmap* drawBitmap = m_Image.GetImage();

		UINT width = drawBitmap->GetWidth();
		UINT height = drawBitmap->GetHeight();

		// Blit the image
		graphics.DrawImage(drawBitmap, 0, 0, width, height);

		graphics.ResetTransform();
	}

	canvas.EndGdiplusContext();

	return true;
}
