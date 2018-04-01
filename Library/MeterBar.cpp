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

MeterBar::MeterBar(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Image(L"BarImage", nullptr, false, skin),
	m_Color(D2D1::ColorF(D2D1::ColorF::Green)),
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
		m_Image.LoadImage(m_ImageName);

		if (m_Image.IsLoaded())
		{
			Gfx::D2DBitmap* bitmap = m_Image.GetImage();

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

	Meter::ReadOptions(parser, section);

	m_Color = Gfx::Util::ToColorF(parser.ReadColor(section, L"BarColor", Gdiplus::Color::Green));

	m_ImageName = parser.ReadString(section, L"BarImage", L"");
	if (!m_ImageName.empty())
	{
		// Read tinting options
		m_Image.ReadOptions(parser, section);
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
		Initialize();  // Reload the image
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

	D2D1_RECT_F meterRect = GetMeterRectPadding();
	FLOAT drawW = meterRect.right - meterRect.left;
	FLOAT drawH = meterRect.bottom - meterRect.top;

	Gfx::D2DBitmap* drawBitmap = m_Image.GetImage();

	if (m_Orientation == VERTICAL)
	{
		int barSize = (int)(drawH - 2 * m_Border);
		int size = (int)(barSize * m_Value);
		size = min(barSize, size);
		size = max(0, size);

		if (drawBitmap)
		{
			if (m_Flip)
			{
				if (m_Border > 0)
				{
					D2D1_RECT_F r2 = meterRect;
					r2.bottom = r2.top + m_Border;
					canvas.DrawBitmap(drawBitmap, r2, D2D1::RectF(0, 0, meterRect.right, (FLOAT)m_Border));
					r2.top = meterRect.top + size + m_Border;
					r2.bottom = r2.top + m_Border;
					canvas.DrawBitmap(drawBitmap, r2, D2D1::RectF(0, drawH - m_Border, drawW, (FLOAT)m_Border));
				}

				D2D1_RECT_F r = { meterRect.left, meterRect.top + m_Border, meterRect.right, meterRect.top + m_Border + size };
				canvas.DrawBitmap(drawBitmap, r, D2D1::RectF(0, (FLOAT)m_Border, drawW, (FLOAT)(m_Border + size)));
			}
			else
			{
				if (m_Border > 0)
				{
					D2D1_RECT_F r2 = { meterRect.left, meterRect.bottom - size - 2 * m_Border, meterRect.right, meterRect.bottom - size - m_Border};
					canvas.DrawBitmap(drawBitmap, r2, D2D1::RectF(0, 0, drawW, (FLOAT)m_Border));
					r2.top = meterRect.bottom - m_Border;
					r2.bottom = r2.top + m_Border;
					canvas.DrawBitmap(drawBitmap, r2, D2D1::RectF(0, drawH - m_Border, drawW, (FLOAT)m_Border));
				}

				D2D1_RECT_F r = { meterRect.left, meterRect.bottom - size - m_Border, meterRect.right, meterRect.bottom - m_Border};
				canvas.DrawBitmap(drawBitmap, r, D2D1::RectF(0, drawH - size - m_Border, drawW, (FLOAT)size));
			}
		}
		else
		{
			if (m_Flip)
			{
				D2D1_RECT_F r = { meterRect.left, meterRect.top, meterRect.right, meterRect.top + size};
				canvas.FillRectangle(r, m_Color);
			}
			else
			{
				D2D1_RECT_F r = { meterRect.left, meterRect.bottom - size, meterRect.right, meterRect.bottom };
				canvas.FillRectangle(r, m_Color);
			}
		}
	}
	else
	{
		int barSize = (int)(drawW - 2 * m_Border);
		int size = (int)(barSize * m_Value);
		size = min(barSize, size);
		size = max(0, size);

		if (drawBitmap)
		{
			if (m_Flip)
			{
				if (m_Border > 0)
				{
					D2D1_RECT_F r2 = { meterRect.right - size - 2 * m_Border, meterRect.top,  meterRect.right - size - m_Border, meterRect.bottom };
					canvas.DrawBitmap(drawBitmap, r2, D2D1::RectF(0, 0, (FLOAT)m_Border, drawH));
					r2.left = meterRect.right - m_Border;
					r2.right = r2.left + m_Border;
					canvas.DrawBitmap(drawBitmap, r2, D2D1::RectF(drawW - m_Border, 0, drawW, drawH + m_Border));
				}

				D2D1_RECT_F r = { meterRect.right - size - m_Border, meterRect.top, meterRect.right - m_Border, meterRect.bottom };
				canvas.DrawBitmap(drawBitmap, r, D2D1::RectF(drawW - size - m_Border, 0, drawW - m_Border, drawH));
			}
			else
			{
				if (m_Border > 0)
				{
					D2D1_RECT_F r2 = { meterRect.left, meterRect.top, meterRect.left + m_Border, meterRect.bottom };
					canvas.DrawBitmap(drawBitmap, r2, D2D1::RectF(0, 0, (FLOAT)m_Border, drawH));
					r2.left = meterRect.left + size + m_Border;
					r2.right = r2.left + m_Border;
					canvas.DrawBitmap(drawBitmap, r2, D2D1::RectF(drawW - m_Border, 0, drawW, drawH));
				}

				D2D1_RECT_F r = { meterRect.left + m_Border, meterRect.top, meterRect.left + m_Border + size, meterRect.bottom };
				canvas.DrawBitmap(drawBitmap, r, D2D1::RectF((FLOAT)m_Border, 0, (FLOAT)(m_Border + size), drawH));
			}
		}
		else
		{
			if (m_Flip)
			{
				D2D1_RECT_F r = { meterRect.right - size, meterRect.top, meterRect.right, meterRect.bottom};
				canvas.FillRectangle(r, m_Color);
			}
			else
			{
				D2D1_RECT_F r = { meterRect.left, meterRect.top, meterRect.left + size, meterRect.bottom };
				canvas.FillRectangle(r, m_Color);
			}
		}
	}

	return true;
}
