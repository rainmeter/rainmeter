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
/*
  $Header: /home/cvsroot/Rainmeter/Library/MeterRoundLine.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeterRoundLine.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.4  2004/08/13 15:46:50  rainy
  Reminder->Remainder.

  Revision 1.3  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.2  2003/02/10 18:12:44  rainy
  Now uses GDI+

  Revision 1.1  2002/12/26 17:51:50  rainy
  Initial version
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
	virtual bool Draw();

private:
	bool m_AntiAlias;								// If true, the line is antialiased
	bool m_Solid;
	double m_LineWidth;
	double m_LineLength;
	double m_LineStart;
	double m_StartAngle;
	double m_RotationAngle;
	UINT m_ValueRemainder;
	Gdiplus::Color m_LineColor;
	double m_Value;
};

#endif
