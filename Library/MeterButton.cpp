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

#pragma warning(disable: 4786)
#pragma warning(disable: 4996)

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
	for (int i = 0; i < BUTTON_FRAMES; i++)
	{
		m_Bitmaps[i] = NULL;;
	}
	m_Bitmap = NULL;
	m_State = BUTTON_STATE_NORMAL;
	m_Clicked = false;
}

/*
** ~CMeterButton
**
** The destructor
**
*/
CMeterButton::~CMeterButton()
{
	for (int i = 0; i < BUTTON_FRAMES; i++)
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
		m_Bitmap = new Bitmap(m_ImageName.c_str());
		Status status = m_Bitmap->GetLastStatus();
		if(Ok != status)
		{
            throw CError(std::wstring(L"Bitmap image not found: ") + m_ImageName, __LINE__, __FILE__);
		}

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

		for (int i = 0; i < BUTTON_FRAMES; i++)
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

/*
** ReadConfig
**
** Read the meter-specific configs from the ini-file.
**
*/
void CMeterButton::ReadConfig(const WCHAR* section)
{
	// Read common configs
	CMeter::ReadConfig(section);

	CConfigParser& parser = m_MeterWindow->GetParser();

	m_ImageName = parser.ReadString(section, L"ButtonImage", L"");
	m_ImageName = m_MeterWindow->MakePathAbsolute(m_ImageName);

	m_Command = parser.ReadString(section, L"ButtonCommand", L"");
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
bool CMeterButton::Draw()
{
	if(!CMeter::Draw()) return false;

	if (m_Bitmaps[m_State] == NULL) return false;	// Unable to continue

	int x = GetX();
	int y = GetY();

	// Blit the image
	Graphics graphics(m_MeterWindow->GetDoubleBuffer());
	graphics.DrawCachedBitmap(m_Bitmaps[m_State], x, y);

	// TEST
/*	HDC desktopDC = GetDC(GetDesktopWindow());
	HDC dc = CreateCompatibleDC(desktopDC);
	HBITMAP bm = CreateCompatibleBitmap(desktopDC, 200, 200);
	ReleaseDC(GetDesktopWindow(), desktopDC);
	HBITMAP oldBM = (HBITMAP)SelectObject(dc, bm);

	Graphics dgfx(GetDesktopWindow());
	Bitmap nbg(200, 200, PixelFormat32bppARGB);
	Bitmap nbm(L"button.png");

	Graphics gfx1(&nbg);
	Graphics gfx2(dc);

	CachedBitmap cbm1(&nbm, &gfx1);
	CachedBitmap cbm2(&nbm, &gfx2);

	Rect r(0, 0, nbm.GetWidth(), nbm.GetHeight());
	int i;

	SolidBrush solid(Color(255, 155, 123, 232));

	DWORD time = GetTickCount();
	for (i = 0; i < 1000000; i++)
	{
		gfx1.DrawImage(&nbm, r, 0, 0, nbm.GetWidth(), nbm.GetHeight(), UnitPixel);
//		gfx1.FillRectangle(&solid, 0, 0, nbm.GetWidth(), nbm.GetHeight());			// 2156
	}
	DebugLog("Bitmap 2 Bitmap: %i", GetTickCount() - time);							// 469

	time = GetTickCount();
	for (i = 0; i < 1000000; i++)
	{
		gfx1.DrawCachedBitmap(&cbm1, 0, 0);
	}
	DebugLog("CachedBitmap 2 Bitmap: %i", GetTickCount() - time);					// 125

	time = GetTickCount();
	for (i = 0; i < 1000000; i++)
	{
		gfx2.DrawImage(&nbm, r, 0, 0, nbm.GetWidth(), nbm.GetHeight(), UnitPixel);
//		gfx2.FillRectangle(&solid, 0, 0, nbm.GetWidth(), nbm.GetHeight());			// 797
	}
	DebugLog("Bitmap 2 CachedBitmap: %i", GetTickCount() - time);					// 469

	time = GetTickCount();
	for (i = 0; i < 1000000; i++)
	{
		gfx2.DrawCachedBitmap(&cbm1, 0, 0);
	}
	DebugLog("CachedBitmap 2 CachedBitmap: %i", GetTickCount() - time);				// 109

	time = GetTickCount();
	for (i = 0; i < 1000000; i++)
	{
		gfx2.DrawCachedBitmap(&cbm2, 0, 0);
	}
	DebugLog("CachedBitmap 2 CachedBitmap 2: %i", GetTickCount() - time);			// 125

	SelectObject(dc, oldBM);
	DeleteDC(dc);
	DeleteObject(bm);
*/
	// ~TEST
	
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

bool CMeterButton::MouseUp(POINT pos, CMeterWindow* window)
{
	int x = GetX();
	int y = GetY();

	if (m_State == BUTTON_STATE_DOWN)
	{
		if (m_Clicked &&
			pos.x >= x && pos.x <= x + m_W &&
			pos.y >= y && pos.y <= y + m_H)
		{
			Color color;
			m_Bitmap->GetPixel(pos.x - x + m_W * m_State, pos.y - y, &color);

			if (color.GetA() > 0)
			{
				// Do a delayed execute or ortherwise !RainmeterRefresh crashes
				PostMessage(window->GetWindow(), WM_DELAYED_EXECUTE, (WPARAM)NULL, (LPARAM)m_Command.c_str());
			}
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
	int x = GetX();
	int y = GetY();

	if (pos.x >= x && pos.x <= x + m_W &&
		pos.y >= y && pos.y <= y + m_H)
	{
		Color color;
		m_Bitmap->GetPixel(pos.x - x + m_W * m_State, pos.y - y, &color);

		if (color.GetA() > 0)
		{
			m_State = BUTTON_STATE_DOWN;
			m_Clicked = true;
			return true;
		}
	}
	return false;
}

bool CMeterButton::MouseMove(POINT pos)
{
	int x = GetX();
	int y = GetY();

	if (m_Clicked == true)
	{
		if (pos.x >= x && pos.x <= x + m_W &&
			pos.y >= y && pos.y <= y + m_H)
		{
			Color color;
			m_Bitmap->GetPixel(pos.x - x + m_W * m_State, pos.y - y, &color);

			if (color.GetA() > 0)
			{
				if (m_State == BUTTON_STATE_NORMAL)
				{
					m_State = BUTTON_STATE_DOWN;
					return true;
				}
			}
		}
		else
		{
			if (m_State == BUTTON_STATE_DOWN)
			{
				m_State = BUTTON_STATE_NORMAL;
				return true;
			}
		}
	}
	else
	{
		if (pos.x >= x && pos.x <= x + m_W &&
			pos.y >= y && pos.y <= y + m_H)
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