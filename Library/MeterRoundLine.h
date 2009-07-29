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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef __METERROUNDLINE_H__
#define __METERROUNDLINE_H__

#include "Meter.h"
#include "MeterWindow.h"

class CMeterRoundLine : public CMeter
{
public:
	CMeterRoundLine(CMeterWindow* meterWindow);
	virtual ~CMeterRoundLine();

	virtual void ReadConfig(const WCHAR* section);
	virtual bool Update();
	virtual bool Draw(Gdiplus::Graphics& graphics);

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
