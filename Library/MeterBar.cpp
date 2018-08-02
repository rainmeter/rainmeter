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

	m_Color = parser.ReadColor(section, L"BarColor", D2D1::ColorF(D2D1::ColorF::Green));

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

	const D2D1_RECT_F meterRect = GetMeterRectPadding();
	const FLOAT drawW = meterRect.right - meterRect.left;
	const FLOAT drawH = meterRect.bottom - meterRect.top;

	Gfx::D2DBitmap* drawBitmap = m_Image.GetImage();

	int barSize = 0;
	int size = 0;
	FLOAT sizeF = 0.0f;
	const FLOAT borderF = (FLOAT)m_Border;

	if (m_Orientation == VERTICAL)
	{
		barSize = (int)(drawH - 2.0 * borderF);
		size = (int)(barSize * m_Value);
		size = min(barSize, size);
		sizeF = (FLOAT)max(0, size);

		if (drawBitmap)
		{
			if (m_Flip)
			{
				if (borderF > 0.0f)
				{
					D2D1_RECT_F r2 = meterRect;
					r2.bottom = r2.top + borderF;
					canvas.DrawBitmap(drawBitmap, r2, D2D1::RectF(0.0f, 0.0f, meterRect.right, borderF));

					r2.top = meterRect.top + sizeF + borderF;
					r2.bottom = r2.top + borderF;
					canvas.DrawBitmap(drawBitmap, r2, D2D1::RectF(0.0f, drawH - borderF, drawW, borderF));
				}

				const D2D1_RECT_F r = D2D1::RectF(
					meterRect.left,
					meterRect.top + borderF,
					meterRect.right,
					meterRect.top + borderF + sizeF);
				canvas.DrawBitmap(drawBitmap, r, D2D1::RectF(0.0f, borderF, drawW, borderF + sizeF));
			}
			else
			{
				if (borderF > 0.0f)
				{
					D2D1_RECT_F r2 = D2D1::RectF(
						meterRect.left,
						meterRect.bottom - sizeF - 2.0f * borderF,
						meterRect.right,
						meterRect.bottom - sizeF - borderF);
					canvas.DrawBitmap(drawBitmap, r2, D2D1::RectF(0.0f, 0.0f, drawW, borderF));

					r2.top = meterRect.bottom - borderF;
					r2.bottom = r2.top + borderF;
					canvas.DrawBitmap(drawBitmap, r2, D2D1::RectF(0.0f, drawH - borderF, drawW, borderF));
				}

				const D2D1_RECT_F r = D2D1::RectF(
					meterRect.left,
					meterRect.bottom - sizeF - borderF,
					meterRect.right,
					meterRect.bottom - borderF);
				canvas.DrawBitmap(drawBitmap, r, D2D1::RectF(0.0f, drawH - sizeF - borderF, drawW, sizeF));
			}
		}
		else
		{
			if (m_Flip)
			{
				canvas.FillRectangle(
					D2D1::RectF(meterRect.left, meterRect.top, meterRect.right, meterRect.top + sizeF),
					m_Color);
			}
			else
			{
				canvas.FillRectangle(
					D2D1::RectF(meterRect.left, meterRect.bottom - sizeF, meterRect.right, meterRect.bottom),
					m_Color);
			}
		}
	}
	else
	{
		barSize = (int)(drawW - 2.0 * borderF);
		size = (int)(barSize * m_Value);
		size = min(barSize, size);
		sizeF = (FLOAT)max(0, size);

		if (drawBitmap)
		{
			if (m_Flip)
			{
				if (borderF > 0.0f)
				{
					D2D1_RECT_F r2 = D2D1::RectF(
						meterRect.right - sizeF - 2.0f * borderF,
						meterRect.top,
						meterRect.right - sizeF - borderF,
						meterRect.bottom);
					canvas.DrawBitmap(drawBitmap, r2, D2D1::RectF(0.0f, 0.0f, borderF, drawH));

					r2.left = meterRect.right - borderF;
					r2.right = r2.left + borderF;
					canvas.DrawBitmap(drawBitmap, r2, D2D1::RectF(drawW - borderF, 0.0f, drawW, drawH + borderF));
				}

				const D2D1_RECT_F r = D2D1::RectF(
					meterRect.right - sizeF - borderF,
					meterRect.top,
					meterRect.right - borderF,
					meterRect.bottom);
				canvas.DrawBitmap(drawBitmap, r, D2D1::RectF(drawW - sizeF - borderF, 0.0f, drawW - borderF, drawH));
			}
			else
			{
				if (borderF > 0.0f)
				{
					D2D1_RECT_F r2 = D2D1::RectF(meterRect.left, meterRect.top, meterRect.left + borderF, meterRect.bottom);
					canvas.DrawBitmap(drawBitmap, r2, D2D1::RectF(0, 0, (FLOAT)m_Border, drawH));

					r2.left = meterRect.left + size + m_Border;
					r2.right = r2.left + m_Border;
					canvas.DrawBitmap(drawBitmap, r2, D2D1::RectF(drawW - borderF, 0.0f, drawW, drawH));
				}

				const D2D1_RECT_F r = D2D1::RectF(
					meterRect.left + borderF,
					meterRect.top,
					meterRect.left + borderF + sizeF,
					meterRect.bottom);
				canvas.DrawBitmap(drawBitmap, r, D2D1::RectF(borderF, 0.0f, borderF + sizeF, drawH));
			}
		}
		else
		{
			if (m_Flip)
			{
				canvas.FillRectangle(
					D2D1::RectF(meterRect.right - sizeF, meterRect.top, meterRect.right, meterRect.bottom),
					m_Color);
			}
			else
			{
				canvas.FillRectangle(
					D2D1::RectF(meterRect.left, meterRect.top, meterRect.left + sizeF, meterRect.bottom),
					m_Color);
			}
		}
	}

	return true;
}
