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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "StdAfx.h"
#include "MeterRotator.h"
#include "Measure.h"
#include "Error.h"
#include "Litestep.h"
#include "Rainmeter.h"

using namespace Gdiplus;

extern CRainmeter* Rainmeter;

/*
** CMeterRotator
**
** The constructor
**
*/
CMeterRotator::CMeterRotator(CMeterWindow* meterWindow) : CMeter(meterWindow)
{
	m_Bitmap = NULL;
	m_Value = 0.0;
}

/*
** ~CMeterRotator
**
** The destructor
**
*/
CMeterRotator::~CMeterRotator()
{
	if(m_Bitmap != NULL) delete m_Bitmap;
}

/*
** Initialize
**
** Load the image & configs.
**
*/
void CMeterRotator::Initialize()
{
	CMeter::Initialize();

	// Load the bitmaps if defined
	if(!m_ImageName.empty())
	{
		if (m_Bitmap != NULL) delete m_Bitmap;
		m_Bitmap = new Bitmap(m_ImageName.c_str());
		Status status = m_Bitmap->GetLastStatus();
		if(Ok != status)
		{
			DebugLog(L"Bitmap image not found: %s", m_ImageName.c_str());

			delete m_Bitmap;
			m_Bitmap = NULL;
		}
	}
	else
	{
		if (m_Bitmap)
		{
			delete m_Bitmap;
			m_Bitmap = NULL;
		}
	}
}

/*
** ReadConfig
**
** Read the meter-specific configs from the ini-file.
**
*/
void CMeterRotator::ReadConfig(const WCHAR* section)
{
	// Store the current values so we know if the image needs to be updated
	std::wstring oldImageName = m_ImageName;

	// Read common configs
	CMeter::ReadConfig(section);

	CConfigParser& parser = m_MeterWindow->GetParser();

	m_ImageName = parser.ReadString(section, L"ImageName", L"");
	m_ImageName = m_MeterWindow->MakePathAbsolute(m_ImageName);

	m_OffsetX = parser.ReadFloat(section, L"OffsetX", 0.0);
	m_OffsetY = parser.ReadFloat(section, L"OffsetY", 0.0);
	m_StartAngle = parser.ReadFloat(section, L"StartAngle", 0.0);
	m_RotationAngle = parser.ReadFloat(section, L"RotationAngle", 6.2832);

	m_ValueRemainder = parser.ReadInt(section, L"ValueReminder", 0);		// Typo
	m_ValueRemainder = parser.ReadInt(section, L"ValueRemainder", m_ValueRemainder);

	if (m_Initialized &&
		oldImageName != m_ImageName)
	{
		Initialize();  // Reload the image
	}
}

/*
** Update
**
** Updates the value(s) from the measures.
**
*/
bool CMeterRotator::Update()
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
bool CMeterRotator::Draw(Graphics& graphics)
{
	if(!CMeter::Draw(graphics)) return false;

	// Calculate the center for rotation
	int x = GetX();
	int y = GetY();

	REAL cx = (REAL)(x + m_W / 2.0);
	REAL cy = (REAL)(y + m_H / 2.0);

	// Calculate the rotation
	REAL angle = (REAL)(m_RotationAngle * m_Value + m_StartAngle);

	angle = angle * 180.0f / 3.14159265f;		// Convert to degrees

	graphics.TranslateTransform(cx, cy);
	graphics.RotateTransform(angle);
	graphics.TranslateTransform((REAL)-m_OffsetX, (REAL)-m_OffsetY);

	if(m_Bitmap)
	{
		UINT width = m_Bitmap->GetWidth();
		UINT height = m_Bitmap->GetHeight();

		// Blit the image
		graphics.DrawImage(m_Bitmap, 0, 0, width, height);
	}
	graphics.ResetTransform();

	return true;
}

