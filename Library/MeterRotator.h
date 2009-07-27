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

#ifndef __METERROTATOR_H__
#define __METERROTATOR_H__

#include "Meter.h"
#include "MeterWindow.h"

class CMeterRotator : public CMeter
{
public:
	CMeterRotator(CMeterWindow* meterWindow);
	virtual ~CMeterRotator();

	virtual void ReadConfig(const WCHAR* section);
	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gdiplus::Graphics& graphics);

private:
	Gdiplus::Bitmap* m_Bitmap;		// The bar bitmap
	std::wstring m_ImageName;		// Name of the image
	double m_OffsetX;
	double m_OffsetY;
	double m_StartAngle;
	double m_RotationAngle;
	UINT m_ValueRemainder;
	double m_Value;
};

#endif
