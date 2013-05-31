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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __METERROTATOR_H__
#define __METERROTATOR_H__

#include "Meter.h"
#include "TintedImage.h"

class MeterRotator : public Meter
{
public:
	MeterRotator(MeterWindow* meterWindow, const WCHAR* name);
	virtual ~MeterRotator();

	virtual UINT GetTypeID() { return TypeID<MeterRotator>(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);

private:
	TintedImage m_Image;
	std::wstring m_ImageName;
	bool m_NeedsReload;

	double m_OffsetX;
	double m_OffsetY;
	double m_StartAngle;
	double m_RotationAngle;
	UINT m_ValueRemainder;
	double m_Value;
};

#endif
