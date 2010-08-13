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

#ifndef __METERBUTTON_H__
#define __METERBUTTON_H__

#include "Meter.h"
#include "MeterWindow.h"

#define BUTTON_FRAMES 3

class CMeterButton : public CMeter
{
public:
	CMeterButton(CMeterWindow* meterWindow);
	virtual ~CMeterButton();

	virtual void ReadConfig(const WCHAR* section);
	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gdiplus::Graphics& graphics);
	virtual void BindMeasure(std::list<CMeasure*>& measures);

	bool MouseMove(POINT pos);
	bool MouseUp(POINT pos, CMeterWindow* window);
	bool MouseDown(POINT pos);

	void SetExecutable(bool exec) { m_Executable = exec; }
	bool IsExecutable() { return m_Executable; }

private:
	bool HitTest2(int px, int py, bool checkAlpha);

	Gdiplus::Bitmap* m_Bitmap;	// The bitmap
	Gdiplus::CachedBitmap* m_Bitmaps[BUTTON_FRAMES];	// The cached bitmaps
	std::wstring m_ImageName;	// Name of the image
	std::wstring m_Command;	// Command to be executed
	int m_State;
	bool m_Clicked;
	bool m_Executable;
};

#endif
