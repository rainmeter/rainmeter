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
  $Header: /home/cvsroot/Rainmeter/Library/MeasureNetIn.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeasureNetIn.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.10  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.9  2003/02/10 18:13:34  rainy
  Changed the way stats are gathered.

  Revision 1.8  2002/12/23 14:26:07  rainy
  Added cumulative statistics measuring.

  Revision 1.7  2002/04/26 18:24:16  rainy
  Modified the Update method to support disabled measures.

  Revision 1.6  2002/04/01 15:37:25  rainy
  Moved part of the implementation to the base class.

  Revision 1.5  2001/12/23 10:17:56  rainy
  The firstTime is now a member variable.

  Revision 1.4  2001/09/26 16:27:15  rainy
  Changed the interfaces a bit.

  Revision 1.3  2001/09/01 13:00:10  rainy
  Slight changes in the interface. The value is now measured only once if possible.

  Revision 1.2  2001/08/12 15:44:49  Rainy
  Adjusted Update()'s interface.
  Net throughput is now measured in bytes per second and not bits.

  Revision 1.1.1.1  2001/08/11 10:58:19  Rainy
  Added to CVS.

*/

#ifndef __METERNET_H__
#define __METERNET_H__

#include "MeasureNet.h"

class CMeasureNetIn : public CMeasureNet
{
public:
	CMeasureNetIn(CMeterWindow* meterWindow);
	virtual ~CMeasureNetIn();

	virtual void ReadConfig(CConfigParser& parser, const WCHAR* section);
	virtual bool Update();

private:
	bool m_FirstTime;
	DWORD m_InOctets;
};

#endif
