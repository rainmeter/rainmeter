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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "MeterRotator.h"
#include "Measure.h"
#include "Error.h"
#include "Litestep.h"
#include "Rainmeter.h"

using namespace Gdiplus;

#define PI	(3.14159265358979323846)
#define CONVERT_TO_DEGREES(X)	((X) * (180.0 / PI))

extern CRainmeter* Rainmeter;

/*
** The constructor
**
*/
CMeterRotator::CMeterRotator(CMeterWindow* meterWindow, const WCHAR* name) : CMeter(meterWindow, name),
	m_NeedsReload(false),
	m_OffsetX(),
	m_OffsetY(),
	m_StartAngle(),
	m_RotationAngle(6.2832),
	m_ValueRemainder(),
	m_Value()
{
}

/*
** The destructor
**
*/
CMeterRotator::~CMeterRotator()
{
}

/*
** Load the image & configs.
**
*/
void CMeterRotator::Initialize()
{
	CMeter::Initialize();

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
** Read the meter-specific configs from the ini-file.
**
*/
void CMeterRotator::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	// Store the current values so we know if the image needs to be updated
	std::wstring oldImageName = m_ImageName;

	// Read common configs
	CMeter::ReadConfig(parser, section);

	m_ImageName = parser.ReadString(section, L"ImageName", L"");
	if (!m_ImageName.empty())
	{
		m_MeterWindow->MakePathAbsolute(m_ImageName);

		// Read tinting configs
		m_Image.ReadConfig(parser, section);
	}
	else
	{
		m_Image.ClearConfigFlags();
	}

	m_OffsetX = parser.ReadFloat(section, L"OffsetX", 0.0);
	m_OffsetY = parser.ReadFloat(section, L"OffsetY", 0.0);
	m_StartAngle = parser.ReadFormula(section, L"StartAngle", 0.0);
	m_RotationAngle = parser.ReadFormula(section, L"RotationAngle", 6.2832);

	m_ValueRemainder = parser.ReadInt(section, L"ValueReminder", 0);		// Typo
	m_ValueRemainder = parser.ReadInt(section, L"ValueRemainder", m_ValueRemainder);

	if (m_Initialized)
	{
		m_NeedsReload = (wcscmp(oldImageName.c_str(), m_ImageName.c_str()) != 0);

		if (m_NeedsReload ||
			m_Image.IsConfigsChanged())
		{
			Initialize();  // Reload the image
		}
	}
}

/*
** Updates the value(s) from the measures.
**
*/
bool CMeterRotator::Update()
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
** Draws the meter on the double buffer
**
*/
bool CMeterRotator::Draw(Graphics& graphics)
{
	if (!CMeter::Draw(graphics)) return false;

	if (m_Image.IsLoaded())
	{
		// Calculate the center for rotation
		int x = GetX();
		int y = GetY();

		REAL cx = (REAL)(x + m_W / 2.0);
		REAL cy = (REAL)(y + m_H / 2.0);

		// Calculate the rotation
		REAL angle = (REAL)(CONVERT_TO_DEGREES(m_RotationAngle * m_Value + m_StartAngle));

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

	return true;
}
