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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __METERBUTTON_H__
#define __METERBUTTON_H__

#include "Meter.h"
#include "TintedImage.h"

#define BUTTON_FRAMES 3

class MeterButton : public Meter
{
public:
	MeterButton(MeterWindow* meterWindow, const WCHAR* name);
	virtual ~MeterButton();

	virtual UINT GetTypeID() { return TypeID<MeterButton>(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

	bool MouseMove(POINT pos);
	bool MouseUp(POINT pos, bool execute);
	bool MouseDown(POINT pos);

	void SetFocus(bool f) { m_Focus = f; }

	bool HitTest2(int px, int py);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);
	
	virtual bool IsFixedSize(bool overwrite = false) { return overwrite; }

private:
	TintedImage m_Image;
	std::wstring m_ImageName;
	bool m_NeedsReload;

	Gdiplus::CachedBitmap* m_Bitmaps[BUTTON_FRAMES];	// Cached bitmaps
	std::wstring m_Command;
	int m_State;
	bool m_Clicked;
	bool m_Focus;
};

#endif
