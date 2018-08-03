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

	const D2D1_RECT_F rect = GetMeterRectPadding();
	const FLOAT width = rect.right - rect.left;
	const FLOAT height = rect.bottom - rect.top;
	const FLOAT border = (FLOAT)m_Border;

	Gfx::D2DBitmap* drawBitmap = m_Image.GetImage();

	if (m_Orientation == VERTICAL)
	{
		const FLOAT barSize = height - 2.0f * border;
		FLOAT size = barSize * (FLOAT)m_Value;
		size = min(barSize, size);
		size = max(0.0f, size);

		if (drawBitmap)
		{
			if (m_Flip)
			{
				if (border > 0.0f)
				{
					const auto d = Gfx::Util::ToRectF(rect.left, rect.top, width, border);
					const auto s = Gfx::Util::ToRectF(0.0f, 0.0f, width, border);
					canvas.DrawBitmap(drawBitmap, d, s);

					const auto d2 = Gfx::Util::ToRectF(rect.left, rect.top + size + border, width, border);
					const auto s2 = Gfx::Util::ToRectF(0.0f, height - border, width, border);
					canvas.DrawBitmap(drawBitmap, d2, s2);
				}

				const auto d = Gfx::Util::ToRectF(rect.left, rect.top + border, width, size);
				const auto s = Gfx::Util::ToRectF(0.0f, border, width, size);
				canvas.DrawBitmap(drawBitmap, d, s);
			}
			else
			{
				if (border > 0.0f)
				{
					const auto d = Gfx::Util::ToRectF(rect.left, rect.bottom - size - 2.0f * border, width, border);
					const auto s = Gfx::Util::ToRectF(0.0f, 0.0f, width, border);
					canvas.DrawBitmap(drawBitmap, d, s);

					const auto d2 = Gfx::Util::ToRectF(rect.left, rect.bottom - border, width, border);
					const auto s2 = Gfx::Util::ToRectF(0.0f, height - border, width, border);
					canvas.DrawBitmap(drawBitmap, d2, s2);
				}

				const auto d = Gfx::Util::ToRectF(rect.left, rect.bottom - size - border, width, size);
				const auto s = Gfx::Util::ToRectF(0.0f, height - size - border, width, size);
				canvas.DrawBitmap(drawBitmap, d, s);
			}
		}
		else
		{
			if (m_Flip)
			{
				const auto r = Gfx::Util::ToRectF(rect.left, rect.top, width, size);
				canvas.FillRectangle(r, m_Color);
			}
			else
			{
				const auto r = Gfx::Util::ToRectF(rect.left, rect.bottom - size, width, size);
				canvas.FillRectangle(r, m_Color);
			}
		}
	}
	else
	{
		const FLOAT barSize = width - 2.0f * border;
		FLOAT size = barSize * (FLOAT)m_Value;
		size = min(barSize, size);
		size = max(0.0f, size);

		if (drawBitmap)
		{
			if (m_Flip)
			{
				if (border > 0.0f)
				{
					const auto d = Gfx::Util::ToRectF(rect.right - size - 2.0f * border, rect.top, border, height);
					const auto s = Gfx::Util::ToRectF(0.0f, 0.0f, border, height);
					canvas.DrawBitmap(drawBitmap, d, s);

					const auto d2 = Gfx::Util::ToRectF(rect.right - border, rect.top, border, height);
					const auto s2 = Gfx::Util::ToRectF(width - border, 0.0f, border, height);
					canvas.DrawBitmap(drawBitmap, d2, s2);
				}

				const auto d = Gfx::Util::ToRectF(rect.right - size - border, rect.top, size, height);
				const auto s = Gfx::Util::ToRectF(width - size - border, 0.0f, size, height);
				canvas.DrawBitmap(drawBitmap, d, s);
			}
			else
			{
				if (border > 0.0f)
				{
					const auto d = Gfx::Util::ToRectF(rect.left, rect.top, border, height);
					const auto s = Gfx::Util::ToRectF(0.0f, 0.0f, border, height);
					canvas.DrawBitmap(drawBitmap, d, s);

					const auto d2 = Gfx::Util::ToRectF(rect.left + size + border, rect.top, border, height);
					const auto s2 = Gfx::Util::ToRectF(width - border, 0.0f, border, height);
					canvas.DrawBitmap(drawBitmap, d2, s2);
				}

				const auto d = Gfx::Util::ToRectF(rect.left + border, rect.top, size, height);
				const auto s = Gfx::Util::ToRectF(border, 0.0f, size, height);
				canvas.DrawBitmap(drawBitmap, d, s);
			}
		}
		else
		{
			if (m_Flip)
			{
				const auto r = Gfx::Util::ToRectF(rect.right - size, rect.top, size, height);
				canvas.FillRectangle(r, m_Color);
			}
			else
			{
				const auto r = Gfx::Util::ToRectF(rect.left, rect.top, size, height);
				canvas.FillRectangle(r, m_Color);
			}
		}
	}

	return true;
}
