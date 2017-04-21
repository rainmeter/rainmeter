/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeterBar.h"
#include "Measure.h"
#include "Util.h"
#include "Rainmeter.h"
#include "../Common/Gfx/Canvas.h"

using namespace Gdiplus;

MeterBar::MeterBar(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Image(L"BarImage", nullptr, false, skin),
	m_NeedsReload(false),
	m_Color(Color::Green),
	m_Orientation(VERTICAL),
	m_Value(),
	m_Border(),
	m_Flip(false)
{
}

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

			m_W = bitmap->GetWidth() + GetWidthPadding();
			m_H = bitmap->GetHeight() + GetHeightPadding();
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

	m_Flip = parser.ReadBool(section, L"Flip", false);

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
		LogErrorF(this, L"BarOrientation=%s is not valid", orientation);
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

	Gdiplus::Rect meterRect = GetMeterRectPadding();

	Bitmap* drawBitmap = m_Image.GetImage();

	if (m_Orientation == VERTICAL)
	{
		int barSize = meterRect.Height - 2 * m_Border;
		int size = (int)(barSize * m_Value);
		size = min(barSize, size);
		size = max(0, size);

		if (drawBitmap)
		{
			if (m_Flip)
			{
				if (m_Border > 0)
				{
					Rect r2(meterRect.X, meterRect.Y, meterRect.Width, m_Border);
					canvas.DrawBitmap(drawBitmap, r2, Rect(0, 0, meterRect.Width, m_Border));
					r2.Y = meterRect.Y + size + m_Border;
					canvas.DrawBitmap(drawBitmap, r2, Rect(0, meterRect.Height - m_Border, meterRect.Width, m_Border));
				}

				Rect r(meterRect.X, meterRect.Y + m_Border, meterRect.Width, size);
				canvas.DrawBitmap(drawBitmap, r, Rect(0, m_Border, meterRect.Width, size));
			}
			else
			{
				if (m_Border > 0)
				{
					Rect r2(meterRect.X, meterRect.Y + meterRect.Height - size - 2 * m_Border, meterRect.Width, m_Border);
					canvas.DrawBitmap(drawBitmap, r2, Rect(0, 0, meterRect.Width, m_Border));
					r2.Y = meterRect.Y + meterRect.Height - m_Border;
					canvas.DrawBitmap(drawBitmap, r2, Rect(0, meterRect.Height - m_Border, meterRect.Width, m_Border));
				}

				Rect r(meterRect.X, meterRect.Y + meterRect.Height - size - m_Border, meterRect.Width, size);
				canvas.DrawBitmap(drawBitmap, r, Rect(0, meterRect.Height - size - m_Border, meterRect.Width, size));
			}
		}
		else
		{
			SolidBrush brush(m_Color);
			if (m_Flip)
			{
				Rect r(meterRect.X, meterRect.Y, meterRect.Width, size);
				canvas.FillRectangle(r, brush);
			}
			else
			{
				Rect r(meterRect.X, meterRect.Y + meterRect.Height - size, meterRect.Width, size);
				canvas.FillRectangle(r, brush);
			}
		}
	}
	else
	{
		int barSize = meterRect.Width - 2 * m_Border;
		int size = (int)(barSize * m_Value);
		size = min(barSize, size);
		size = max(0, size);

		if (drawBitmap)
		{
			if (m_Flip)
			{
				if (m_Border > 0)
				{
					Rect r2(meterRect.X + meterRect.Width - size - 2 * m_Border, meterRect.Y, m_Border, meterRect.Height);
					canvas.DrawBitmap(drawBitmap, r2, Rect(0, 0, m_Border, meterRect.Height));
					r2.X = meterRect.X + meterRect.Width - m_Border;
					canvas.DrawBitmap(drawBitmap, r2, Rect(meterRect.Width - m_Border, 0, m_Border, meterRect.Height));
				}

				Rect r(meterRect.X + meterRect.Width - size - m_Border, meterRect.Y, size, meterRect.Height);
				canvas.DrawBitmap(drawBitmap, r, Rect(meterRect.Width - size - m_Border, 0, size, meterRect.Height));
			}
			else
			{
				if (m_Border > 0)
				{
					Rect r2(meterRect.X, meterRect.Y, m_Border, meterRect.Height);
					canvas.DrawBitmap(drawBitmap, r2, Rect(0, 0, m_Border, meterRect.Height));
					r2.X = meterRect.X + size + m_Border;
					canvas.DrawBitmap(drawBitmap, r2, Rect(meterRect.Width - m_Border, 0, m_Border, meterRect.Height));
				}

				Rect r(meterRect.X + m_Border, meterRect.Y, size, meterRect.Height);
				canvas.DrawBitmap(drawBitmap, r, Rect(m_Border, 0, size, meterRect.Height));
			}
		}
		else
		{
			SolidBrush brush(m_Color);
			if (m_Flip)
			{
				Rect r(meterRect.X + meterRect.Width - size, meterRect.Y, size, meterRect.Height);
				canvas.FillRectangle(r, brush);
			}
			else
			{
				Rect r(meterRect.X, meterRect.Y, size, meterRect.Height);
				canvas.FillRectangle(r, brush);
			}
		}
	}

	return true;
}
