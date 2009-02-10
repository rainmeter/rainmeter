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
  $Header: /home/cvsroot/Rainmeter/Library/MeterBar.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeterBar.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.6  2004/07/11 17:16:11  rainy
  Added BarBorder.

  Revision 1.5  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.4  2003/02/10 18:12:45  rainy
  Now uses GDI+

  Revision 1.3  2002/03/31 09:58:54  rainy
  Added some comments

  Revision 1.2  2001/09/26 16:26:23  rainy
  Small adjustement to the interfaces.

  Revision 1.1  2001/09/01 12:56:25  rainy
  Initial version.


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
	virtual bool Draw();

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
