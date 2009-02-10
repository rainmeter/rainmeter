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
  $Header: /home/cvsroot/Rainmeter/Library/MeasureDiskSpace.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeasureDiskSpace.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.7  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.6  2004/03/13 16:16:12  rainy
  CDROMs are ignored

  Revision 1.5  2002/04/26 18:24:16  rainy
  Modified the Update method to support disabled measures.

  Revision 1.4  2001/10/28 10:22:20  rainy
  GetStringValue uses consts.

  Revision 1.3  2001/09/26 16:27:15  rainy
  Changed the interfaces a bit.

  Revision 1.2  2001/09/01 13:00:10  rainy
  Slight changes in the interface. The value is now measured only once if possible.

  Revision 1.1  2001/08/12 15:35:08  Rainy
  Inital Version


*/

#ifndef __MEASUREDISKSPACE_H__
#define __MEASUREDISKSPACE_H__

#include "Measure.h"

class CMeasureDiskSpace : public CMeasure
{
public:
	CMeasureDiskSpace(CMeterWindow* meterWindow);
	virtual ~CMeasureDiskSpace();

	virtual void ReadConfig(CConfigParser& parser, const WCHAR* section);
	virtual bool Update();
	virtual const WCHAR* GetStringValue(bool autoScale, double scale, int decimals, bool percentual);

private:
	std::wstring m_Drive;
	std::wstring m_LabelName;
	bool m_Total;
	bool m_Label;
};

#endif
