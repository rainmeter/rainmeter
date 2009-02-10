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
  $Header: /home/cvsroot/Rainmeter/Library/MeasureTime.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeasureTime.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.5  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.4  2004/03/13 16:17:42  rainy
  Added timezones

  Revision 1.3  2003/02/10 18:13:14  rainy
  Returns the time in secs as value.

  Revision 1.2  2002/04/26 18:24:15  rainy
  Modified the Update method to support disabled measures.

  Revision 1.1  2001/12/23 10:11:08  rainy
  Initial version.

*/

#ifndef __MEASURETIME_H__
#define __MEASURETIME_H__

#include "Measure.h"

class CMeasureTime : public CMeasure
{
public:
	CMeasureTime(CMeterWindow* meterWindow);
	virtual ~CMeasureTime();

	virtual void ReadConfig(CConfigParser& parser, const WCHAR* section);
	virtual bool Update();
	virtual const WCHAR* GetStringValue(bool autoScale, double scale, int decimals, bool percentual);
	
private:
	std::wstring m_Format;
	LARGE_INTEGER m_DeltaTime;
	LARGE_INTEGER m_Time;
};

#endif
