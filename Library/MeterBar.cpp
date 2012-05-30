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

using namespace Gdiplus;

extern CRainmeter* Rainmeter;

/*
** The constructor
**
*/
CMeterBar::CMeterBar(CMeterWindow* meterWindow, const WCHAR* name) : CMeter(meterWindow, name),
	m_Image(L"BarImage"),
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
CMeterBar::~CMeterBar()
{
}

/*
** Load the image or create the brush. If image is used get the dimensions
** of the meter from it.
**
*/
void CMeterBar::Initialize()
{
	CMeter::Initialize();

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
** Read the meter-specific configs from the ini-file.
**
*/
void CMeterBar::ReadOptions(CConfigParser& parser, const WCHAR* section)
{
	// Store the current values so we know if the image needs to be updated
	std::wstring oldImageName = m_ImageName;
	int oldW = m_W;
	int oldH = m_H;

	// Read common configs
	CMeter::ReadOptions(parser, section);

	m_Color = parser.ReadColor(section, L"BarColor", Color::Green);

	m_ImageName = parser.ReadString(section, L"BarImage", L"");
	if (!m_ImageName.empty())
	{
		m_MeterWindow->MakePathAbsolute(m_ImageName);

		// Read tinting configs
		m_Image.ReadOptions(parser, section);
	}
	else
	{
		m_Image.ClearOptionFlags();
	}

	m_Border = parser.ReadInt(section, L"BarBorder", 0);

	m_Flip = parser.ReadInt(section, L"Flip", 0) == 1;

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
		LogWithArgs(LOG_ERROR, L"BarOrientation=%s is not valid in [%s]", orientation, m_Name.c_str());
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
bool CMeterBar::Update()
{
	if (CMeter::Update() && m_Measure)
	{
		m_Value = m_Measure->GetRelativeValue();
		return true;
	}
	return false;
}

/*
** Draws the meter on the double buffer
**
*/
bool CMeterBar::Draw(Graphics& graphics)
{
	if (!CMeter::Draw(graphics)) return false;

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
					graphics.DrawImage(drawBitmap, r2, 0, 0, m_W, m_Border, UnitPixel);
					r2.Y = y + size + m_Border;
					graphics.DrawImage(drawBitmap, r2, 0, m_H - m_Border, m_W, m_Border, UnitPixel);
				}

				// Blit the image
				Rect r(x, y + m_Border, m_W, size);
				graphics.DrawImage(drawBitmap, r, 0, m_Border, m_W, size, UnitPixel);
			}
			else
			{
				if (m_Border > 0)
				{
					Rect r2(x, y + m_H - size - 2 * m_Border, m_W, m_Border);
					graphics.DrawImage(drawBitmap, r2, 0, 0, m_W, m_Border, UnitPixel);
					r2.Y = y + m_H - m_Border;
					graphics.DrawImage(drawBitmap, r2, 0, m_H - m_Border, m_W, m_Border, UnitPixel);
				}

				// Blit the image
				Rect r(x, y + m_H - size - m_Border, m_W, size);
				graphics.DrawImage(drawBitmap, r, 0, m_H - size - m_Border, m_W, size, UnitPixel);
			}
		}
		else
		{
			SolidBrush brush(m_Color);
			if (m_Flip)
			{
				Rect r(x, y, m_W, size);
				graphics.FillRectangle(&brush, r);
			}
			else
			{
				Rect r(x, y + m_H - size, m_W, size);
				graphics.FillRectangle(&brush, r);
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
					graphics.DrawImage(drawBitmap, r2, 0, 0, m_Border, m_H, UnitPixel);
					r2.X = x + m_W - m_Border;
					graphics.DrawImage(drawBitmap, r2, m_W - m_Border, 0, m_Border, m_H, UnitPixel);
				}

				// Blit the image
				Rect r(x + m_W - size - m_Border, y, size, m_H);
				graphics.DrawImage(drawBitmap, r, m_W - size - m_Border, 0, size, m_H, UnitPixel);
			}
			else
			{
				if (m_Border > 0)
				{
					Rect r2(x, y, m_Border, m_H);
					graphics.DrawImage(drawBitmap, r2, 0, 0, m_Border, m_H, UnitPixel);
					r2.X = x + size + m_Border;
					graphics.DrawImage(drawBitmap, r2, m_W - m_Border, 0, m_Border, m_H, UnitPixel);
				}

				// Blit the image
				Rect r(x + m_Border, y, size, m_H);
				graphics.DrawImage(drawBitmap, r, m_Border, 0, size, m_H, UnitPixel);
			}
		}
		else
		{
			SolidBrush brush(m_Color);
			if (m_Flip)
			{
				Rect r(x + m_W - size, y, size, m_H);
				graphics.FillRectangle(&brush, r);
			}
			else
			{
				Rect r(x, y, size, m_H);
				graphics.FillRectangle(&brush, r);
			}
		}
	}

	return true;
}

