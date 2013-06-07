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
#include "MeterBar.h"
#include "Measure.h"
#include "Error.h"
#include "Litestep.h"
#include "Rainmeter.h"
#include "../Common/Gfx/Canvas.h"

using namespace Gdiplus;

extern Rainmeter* g_Rainmeter;

/*
** The constructor
**
*/
MeterBar::MeterBar(MeterWindow* meterWindow, const WCHAR* name) : Meter(meterWindow, name),
	m_Image(L"BarImage", nullptr, false, meterWindow),
	m_NeedsReload(false),
	m_Color(Color::Green),
	m_Orientation(VERTICAL),
	m_Value(),
	m_Border(),
	m_Flip(false)
{
}

/*
** The destructor
**
*/
MeterBar::~MeterBar()
{
}

/*
** Load the image or create the brush. If image is used get the dimensions
** of the meter from it.
**
*/
void MeterBar::Initialize()
{
	Meter::Initialize();

	// Load the bitmaps if defined
	if (!m_ImageName.empty())
	{
		m_Image.LoadImage(m_ImageName, m_NeedsReload);

		if (m_Image.IsLoaded())
		{
			Bitmap* bitmap = m_Image.GetImage();

			m_W = bitmap->GetWidth();
			m_H = bitmap->GetHeight();
		}
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
void MeterBar::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	// Store the current values so we know if the image needs to be updated
	std::wstring oldImageName = m_ImageName;
	int oldW = m_W;
	int oldH = m_H;

	Meter::ReadOptions(parser, section);

	m_Color = parser.ReadColor(section, L"BarColor", Color::Green);

	m_ImageName = parser.ReadString(section, L"BarImage", L"");
	if (!m_ImageName.empty())
	{
		// Read tinting options
		m_Image.ReadOptions(parser, section);
	}
	else
	{
		m_Image.ClearOptionFlags();
	}

	m_Border = parser.ReadInt(section, L"BarBorder", 0);

	m_Flip = 0!=parser.ReadInt(section, L"Flip", 0);

	const WCHAR* orientation = parser.ReadString(section, L"BarOrientation", L"VERTICAL").c_str();
	if (_wcsicmp(L"VERTICAL", orientation) == 0)
	{
		m_Orientation = VERTICAL;
	}
	else if (_wcsicmp(L"HORIZONTAL", orientation) == 0)
	{
		m_Orientation = HORIZONTAL;
	}
	else
	{
		LogErrorF(L"BarOrientation=%s is not valid in [%s]", orientation, m_Name.c_str());
	}

	if (m_Initialized)
	{
		m_NeedsReload = (wcscmp(oldImageName.c_str(), m_ImageName.c_str()) != 0);

		if (m_NeedsReload ||
			m_Image.IsOptionsChanged())
		{
			Initialize();  // Reload the image
		}
		else if (!m_ImageName.empty())
		{
			// Reset to old dimensions
			m_W = oldW;
			m_H = oldH;
		}
	}
}

/*
** Updates the value(s) from the measures.
**
*/
bool MeterBar::Update()
{
	if (Meter::Update() && !m_Measures.empty())
	{
		m_Value = m_Measures[0]->GetRelativeValue();
		return true;
	}
	return false;
}

/*
** Draws the meter on the double buffer
**
*/
bool MeterBar::Draw(Gfx::Canvas& canvas)
{
	if (!Meter::Draw(canvas)) return false;

	int x = GetX();
	int y = GetY();

	Bitmap* drawBitmap = m_Image.GetImage();

	if (m_Orientation == VERTICAL)
	{
		int barSize = m_H - 2 * m_Border;
		int size = (int)(barSize * m_Value);
		size = min(barSize, size);
		size = max(0, size);

		if (drawBitmap)
		{
			if (m_Flip)
			{
				if (m_Border > 0)
				{
					Rect r2(x, y, m_W, m_Border);
					canvas.DrawBitmap(drawBitmap, r2, Rect(0, 0, m_W, m_Border));
					r2.Y = y + size + m_Border;
					canvas.DrawBitmap(drawBitmap, r2, Rect(0, m_H - m_Border, m_W, m_Border));
				}

				Rect r(x, y + m_Border, m_W, size);
				canvas.DrawBitmap(drawBitmap, r, Rect(0, m_Border, m_W, size));
			}
			else
			{
				if (m_Border > 0)
				{
					Rect r2(x, y + m_H - size - 2 * m_Border, m_W, m_Border);
					canvas.DrawBitmap(drawBitmap, r2, Rect(0, 0, m_W, m_Border));
					r2.Y = y + m_H - m_Border;
					canvas.DrawBitmap(drawBitmap, r2, Rect(0, m_H - m_Border, m_W, m_Border));
				}

				Rect r(x, y + m_H - size - m_Border, m_W, size);
				canvas.DrawBitmap(drawBitmap, r, Rect(0, m_H - size - m_Border, m_W, size));
			}
		}
		else
		{
			SolidBrush brush(m_Color);
			if (m_Flip)
			{
				Rect r(x, y, m_W, size);
				canvas.FillRectangle(r, brush);
			}
			else
			{
				Rect r(x, y + m_H - size, m_W, size);
				canvas.FillRectangle(r, brush);
			}
		}
	}
	else
	{
		int barSize = m_W - 2 * m_Border;
		int size = (int)(barSize * m_Value);
		size = min(barSize, size);
		size = max(0, size);

		if (drawBitmap)
		{
			if (m_Flip)
			{
				if (m_Border > 0)
				{
					Rect r2(x + m_W - size - 2 * m_Border, y, m_Border, m_H);
					canvas.DrawBitmap(drawBitmap, r2, Rect(0, 0, m_Border, m_H));
					r2.X = x + m_W - m_Border;
					canvas.DrawBitmap(drawBitmap, r2, Rect(m_W - m_Border, 0, m_Border, m_H));
				}

				Rect r(x + m_W - size - m_Border, y, size, m_H);
				canvas.DrawBitmap(drawBitmap, r, Rect(m_W - size - m_Border, 0, size, m_H));
			}
			else
			{
				if (m_Border > 0)
				{
					Rect r2(x, y, m_Border, m_H);
					canvas.DrawBitmap(drawBitmap, r2, Rect(0, 0, m_Border, m_H));
					r2.X = x + size + m_Border;
					canvas.DrawBitmap(drawBitmap, r2, Rect(m_W - m_Border, 0, m_Border, m_H));
				}

				Rect r(x + m_Border, y, size, m_H);
				canvas.DrawBitmap(drawBitmap, r, Rect(m_Border, 0, size, m_H));
			}
		}
		else
		{
			SolidBrush brush(m_Color);
			if (m_Flip)
			{
				Rect r(x + m_W - size, y, size, m_H);
				canvas.FillRectangle(r, brush);
			}
			else
			{
				Rect r(x, y, size, m_H);
				canvas.FillRectangle(r, brush);
			}
		}
	}

	return true;
}

