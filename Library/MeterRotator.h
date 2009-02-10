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
/*
  $Header: /home/cvsroot/Rainmeter/Library/MeterRotator.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeterRotator.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.3  2004/08/13 15:46:43  rainy
  Added LineStart.
  Reminder->Remainder.

  Revision 1.2  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.1  2004/03/13 16:18:43  rainy
  Initial version

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
	virtual bool Draw();

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
