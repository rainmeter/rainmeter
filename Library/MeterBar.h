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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef __METERBAR_H__
#define __METERBAR_H__

#include "Meter.h"
#include "MeterWindow.h"

class CMeterBar : public CMeter
{
public:
	CMeterBar(CMeterWindow* meterWindow);
	virtual ~CMeterBar();

	virtual void ReadConfig(const WCHAR* section);
	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gdiplus::Graphics& graphics);

private:
	enum ORIENTATION
	{
		HORIZONTAL,
		VERTICAL
	};

	Gdiplus::Color m_Color;			// Color of the bar
	Gdiplus::Bitmap* m_Bitmap;		// The bar bitmap
	ORIENTATION m_Orientation;	// Orientation (i.e. the growth direction)
	std::wstring m_ImageName;	// Name of the bar-image
	double m_Value;
	int m_Border;
	bool m_Flip;
};

#endif
