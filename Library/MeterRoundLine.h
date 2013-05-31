/*
  Copyright (C) 2002 Kimmo Pekkola

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

#ifndef __METERROUNDLINE_H__
#define __METERROUNDLINE_H__

#include "Meter.h"

class MeterRoundLine : public Meter
{
public:
	MeterRoundLine(MeterWindow* meterWindow, const WCHAR* name);
	virtual ~MeterRoundLine();

	virtual UINT GetTypeID() { return TypeID<MeterRoundLine>(); }

	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

private:
	bool m_Solid;
	double m_LineWidth;
	double m_LineLength;
	double m_LineStart;
	double m_StartAngle;
	double m_RotationAngle;
	bool m_CntrlAngle;
	bool m_CntrlLineStart;
	bool m_CntrlLineLength;
	double m_LineStartShift;
	double m_LineLengthShift;
	UINT m_ValueRemainder;
	Gdiplus::Color m_LineColor;
	double m_Value;
};

#endif
