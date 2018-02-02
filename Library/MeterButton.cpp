/* Copyright (C) 2005 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeterButton.h"
#include "Measure.h"
#include "Rainmeter.h"
#include "../Common/Gfx/Canvas.h"

using namespace Gdiplus;

enum BUTTON_STATE
{
	BUTTON_STATE_NORMAL,
	BUTTON_STATE_DOWN,
	BUTTON_STATE_HOVER
};

MeterButton::MeterButton(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Image(L"ButtonImage", nullptr, true, skin),
	m_BitmapsRects(),
	m_State(BUTTON_STATE_NORMAL),
	m_Clicked(false),
	m_Focus(false)
{
}

MeterButton::~MeterButton()
{
}

/*
** Load the image and get the dimensions of the meter from it.
**
*/
void MeterButton::Initialize()
{
	Meter::Initialize();

	// Load the bitmaps if defined
	if (!m_ImageName.empty())
	{
		m_Image.LoadImage(m_ImageName);

		if (m_Image.IsLoaded())
		{
			Gfx::D2DBitmap* bitmap = m_Image.GetImage();

			int bitmapW = bitmap->GetWidth();
			int bitmapH = bitmap->GetHeight();

			m_W = bitmapW;
			m_H = bitmapH;

			if (m_H > m_W)
			{
				m_H /= BUTTON_FRAMES;
			}
			else
			{
				m_W /= BUTTON_FRAMES;
			}

			// Separate the frames
			for (int i = 0; i < BUTTON_FRAMES; ++i)
			{
				Bitmap bitmapPart(m_W, m_H, PixelFormat32bppPARGB);
				Graphics graphics(&bitmapPart);

				if (bitmapH > bitmapW)
				{
					m_BitmapsRects[i] = Gdiplus::Rect(0, m_H * i, m_W, m_H);
				}
				else
				{
					m_BitmapsRects[i] = Gdiplus::Rect(m_W * i, 0, m_W, m_H);
				}
			}

			m_W += GetWidthPadding();
			m_H += GetHeightPadding();
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
void MeterButton::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	// Store the current values so we know if the image needs to be updated
	std::wstring oldImageName = m_ImageName;
	Meter::ReadOptions(parser, section);

	m_ImageName = parser.ReadString(section, L"ButtonImage", L"");
	if (!m_ImageName.empty())
	{
		// Read tinting options
		m_Image.ReadOptions(parser, section);
	}

	m_Command = parser.ReadString(section, L"ButtonCommand", L"", false);

	if (m_Initialized)
	{
		Initialize();  // Reload the image
	}
}

/*
** Updates the value(s) from the measures.
**
*/
bool MeterButton::Update()
{
	return Meter::Update();
}

/*
** Draws the meter on the double buffer
**
*/
bool MeterButton::Draw(Gfx::Canvas& canvas)
{
	if (!Meter::Draw(canvas)) return false;
	
	const auto image = m_Image.GetImage();
	Gdiplus::Rect meterRect = GetMeterRectPadding();

	if(image)
	{
		canvas.DrawBitmap(image, Rect(meterRect.X, meterRect.Y, m_W, m_H), m_BitmapsRects[m_State]);
	}

	return true;
}

/*
** Overridden method. The meters need not to be bound on anything
**
*/
void MeterButton::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	BindPrimaryMeasure(parser, section, true);
}

/*
** Checks if the given point is inside the button.
**
*/
bool MeterButton::HitTest2(int px, int py)
{
	int x = GetX();
	int y = GetY();

	if (m_MouseOver &&
		px >= x && px < x + m_W &&
		py >= y && py < y + m_H)
	{
		if (m_SolidColor.GetA() != 0 || m_SolidColor2.GetA() != 0)
		{
			return true;
		}

		// Check transparent pixels
		if (m_Image.IsLoaded())
		{
			Rect meterRect = GetMeterRectPadding();
			int ix = meterRect.Width * m_State;
			px = px - meterRect.X + ix;
			py = py - meterRect.Y;
			if (px >= ix && px < ix + meterRect.Width &&
				py >= 0 && py < meterRect.Height)
			{
				auto bitmap = m_Image.GetImage();
				if (!bitmap) return false;
				D2D1_COLOR_F color;
				bitmap->GetPixel(m_Skin->GetCanvas(), px, py, color);
				return color.a != 0;
				/*Color color;
				Status status = m_Image.GetImage()->GetPixel(px, py, &color);
				if (status != Ok || color.GetA() != 0)
				{
					return true;
				}*/
			}
		}
		else
		{
			return true;
		}
	}
	return false;
}

bool MeterButton::MouseUp(POINT pos, bool execute)
{
	if (m_State == BUTTON_STATE_DOWN)
	{
		if (execute && m_Clicked && m_Focus && HitTest2(pos.x, pos.y))
		{
			GetRainmeter().ExecuteCommand(m_Command.c_str(), m_Skin);
		}
		m_State = BUTTON_STATE_NORMAL;
		m_Clicked = false;
		return true;
	}
	m_Clicked = false;

	return false;
}

bool MeterButton::MouseDown(POINT pos)
{
	if (m_Focus && HitTest2(pos.x, pos.y))
	{
		m_State = BUTTON_STATE_DOWN;
		m_Clicked = true;
		return true;
	}
	return false;
}

bool MeterButton::MouseMove(POINT pos)
{
	if (m_Clicked)
	{
		if (HitTest2(pos.x, pos.y))
		{
			if (m_State == BUTTON_STATE_NORMAL)
			{
				m_State = BUTTON_STATE_DOWN;
				return true;
			}
		}
		else
		{
			// If the left button is not down anymore the clicked state needs to be set false
			if (!IsLButtonDown())
			{
				m_Clicked = false;
			}

			if (m_State == BUTTON_STATE_DOWN)
			{
				m_State = BUTTON_STATE_NORMAL;
				return true;
			}
		}
	}
	else
	{
		if (HitTest2(pos.x, pos.y))
		{
			if (m_State == BUTTON_STATE_NORMAL)
			{
				m_State = BUTTON_STATE_HOVER;
				return true;
			}
		}
		else
		{
			if (m_State == BUTTON_STATE_HOVER)
			{
				m_State = BUTTON_STATE_NORMAL;
				return true;
			}
		}
	}
	return false;
}
