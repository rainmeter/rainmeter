/*
  Copyright (C) 2005 Kimmo Pekkola

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
#include "MeterButton.h"
#include "Measure.h"
#include "Rainmeter.h"
#include "Error.h"

extern CRainmeter* Rainmeter;

using namespace Gdiplus;

enum BUTTON_STATE
{
	BUTTON_STATE_NORMAL,
	BUTTON_STATE_DOWN,
	BUTTON_STATE_HOVER
};

/*
** CMeterButton
**
** The constructor
**
*/
CMeterButton::CMeterButton(CMeterWindow* meterWindow) : CMeter(meterWindow)
{
	for (int i = 0; i < BUTTON_FRAMES; ++i)
	{
		m_Bitmaps[i] = NULL;
	}
	m_Bitmap = NULL;
	m_State = BUTTON_STATE_NORMAL;
	m_Clicked = false;
	m_Executable = false;
}

/*
** ~CMeterButton
**
** The destructor
**
*/
CMeterButton::~CMeterButton()
{
	for (int i = 0; i < BUTTON_FRAMES; ++i)
	{
		if (m_Bitmaps[i] != NULL) delete m_Bitmaps[i];
	}

	if (m_Bitmap != NULL) delete m_Bitmap;
}

/*
** Initialize
**
** Load the image and get the dimensions of the meter from it.
**
*/
void CMeterButton::Initialize()
{
	CMeter::Initialize();

	// Load the bitmaps if defined
	if(!m_ImageName.empty())
	{
		for (int i = 0; i < BUTTON_FRAMES; ++i)
		{
			if (m_Bitmaps[i] != NULL)
			{
				delete m_Bitmaps[i];
				m_Bitmaps[i] = NULL;
			}
		}
		if (m_Bitmap != NULL) delete m_Bitmap;
		m_Bitmap = new Bitmap(m_ImageName.c_str());
		Status status = m_Bitmap->GetLastStatus();
		if(Ok != status)
		{
			DebugLog(L"Bitmap image not found: %s", m_ImageName.c_str());

			delete m_Bitmap;
			m_Bitmap = NULL;
		}
		else
		{
			m_W = m_Bitmap->GetWidth();
			m_H = m_Bitmap->GetHeight();

			if(m_H > m_W)
			{
				m_H = m_H / BUTTON_FRAMES;
			}
			else
			{
				m_W = m_W / BUTTON_FRAMES;
			}

			// Separate the frames
			Graphics desktopGraphics(GetDesktopWindow());

			for (int i = 0; i < BUTTON_FRAMES; ++i)
			{
				Bitmap bitmapPart(m_W, m_H, PixelFormat32bppARGB);
				Graphics graphics(&bitmapPart);
				Rect r(0, 0, m_W, m_H);

				if(m_Bitmap->GetHeight() > m_Bitmap->GetWidth())
				{
					graphics.DrawImage(m_Bitmap, r, 0, m_H * i, m_W, m_H, UnitPixel);
				}
				else
				{
					graphics.DrawImage(m_Bitmap, r, m_W * i, 0, m_W, m_H, UnitPixel);
				}
				m_Bitmaps[i] = new CachedBitmap(&bitmapPart, &graphics);
			}
		}
	}
	else
	{
		for (int i = 0; i < BUTTON_FRAMES; ++i)
		{
			if (m_Bitmaps[i])
			{
				delete m_Bitmaps[i];
				m_Bitmaps[i] = NULL;
			}
		}
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
void CMeterButton::ReadConfig(const WCHAR* section)
{
	// Store the current values so we know if the image needs to be updated
	std::wstring oldImageName = m_ImageName;
	int oldW = m_W;
	int oldH = m_H;

	// Read common configs
	CMeter::ReadConfig(section);

	CConfigParser& parser = m_MeterWindow->GetParser();

	m_ImageName = parser.ReadString(section, L"ButtonImage", L"");
	m_ImageName = m_MeterWindow->MakePathAbsolute(m_ImageName);

	m_Command = parser.ReadString(section, L"ButtonCommand", L"", false);

	if (m_Initialized)
	{
		if (oldImageName != m_ImageName)
		{
			Initialize();  // Reload the image
		}
		else
		{
			// Reset to old dimensions
			m_W = oldW;
			m_H = oldH;
		}
	}
}

/*
** Update
**
** Updates the value(s) from the measures.
**
*/
bool CMeterButton::Update()
{
	return CMeter::Update();
}

/*
** Draw
**
** Draws the meter on the double buffer
**
*/
bool CMeterButton::Draw(Graphics& graphics)
{
	if(!CMeter::Draw(graphics)) return false;

	if (m_Bitmaps[m_State] == NULL) return false;	// Unable to continue

	int x = GetX();
	int y = GetY();

	// Blit the image
	graphics.DrawCachedBitmap(m_Bitmaps[m_State], x, y);
	
	return true;
}

/*
** BindMeasure
**
** Overridden method. The meters need not to be bound on anything
**
*/
void CMeterButton::BindMeasure(std::list<CMeasure*>& measures)
{
	// It's ok not to bind meter to anything
	if (!m_MeasureName.empty())
	{
		CMeter::BindMeasure(measures);
	}
}

/*
** HitTest2
**
** Checks if the given point is inside the button.
**
*/
bool CMeterButton::HitTest2(int px, int py, bool checkAlpha)
{
	int x = GetX();
	int y = GetY();

	if (m_MouseOver &&
		px >= x && px < x + m_W &&
		py >= y && py < y + m_H)
	{
		if (checkAlpha)
		{
			if (m_SolidColor.GetA() > 0 || m_SolidColor2.GetA() > 0)
			{
				return true;
			}

			// Check transparent pixels
			if (m_Bitmap)
			{
				Color color;
				Status status = m_Bitmap->GetPixel(px - x + m_W * m_State, py - y, &color);
				if (status != Ok || color.GetA() > 0)
				{
					return true;
				}
			}
			else
			{
				return true;
			}
		}
		else
		{
			return true;
		}
	}
	return false;
}

bool CMeterButton::MouseUp(POINT pos, CMeterWindow* window)
{
	if (m_State == BUTTON_STATE_DOWN)
	{
		if (window && m_Clicked && m_Executable && HitTest2(pos.x, pos.y, true))
		{
			Rainmeter->ExecuteCommand(m_Command.c_str(), window);
		}
		m_State = BUTTON_STATE_NORMAL;
		m_Clicked = false;
		return true;
	}
	m_Clicked = false;

	return false;
}

bool CMeterButton::MouseDown(POINT pos)
{
	if (m_Executable && HitTest2(pos.x, pos.y, true))
	{
		m_State = BUTTON_STATE_DOWN;
		m_Clicked = true;
		return true;
	}
	return false;
}

bool CMeterButton::MouseMove(POINT pos)
{
	if (m_Clicked == true)
	{
		if (HitTest2(pos.x, pos.y, true))
		{
			if (m_State == BUTTON_STATE_NORMAL)
			{
				m_State = BUTTON_STATE_DOWN;
				return true;
			}
		}
		else
		{
			// If the left button is not down anymore the cliked state needs to be set false
			if ((GetKeyState(VK_LBUTTON) & 0x8000) == 0)
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
		if (HitTest2(pos.x, pos.y, false))
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