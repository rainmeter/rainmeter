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
  $Header: /home/cvsroot/Rainmeter/Library/MeasureNetOut.cpp,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeasureNetOut.cpp,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.16  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.15  2003/12/05 15:50:10  Rainy
  Multi-instance changes.

  Revision 1.14  2003/02/10 18:13:34  rainy
  Changed the way stats are gathered.

  Revision 1.13  2002/12/23 14:26:07  rainy
  Added cumulative statistics measuring.

  Revision 1.12  2002/07/01 15:34:39  rainy
  The measuring is done in the base class.

  Revision 1.11  2002/04/26 18:24:16  rainy
  Modified the Update method to support disabled measures.

  Revision 1.10  2002/04/01 15:37:25  rainy
  Moved part of the implementation to the base class.

  Revision 1.9  2002/03/31 09:58:54  rainy
  Added some comments

  Revision 1.8  2001/12/23 10:17:56  rainy
  The firstTime is now a member variable.

  Revision 1.7  2001/10/28 10:21:58  rainy
  Changes in the GetStat()

  Revision 1.6  2001/10/14 07:29:39  rainy
  Minor monifications to remove few warnings with VC.NET

  Revision 1.5  2001/09/26 16:27:14  rainy
  Changed the interfaces a bit.

  Revision 1.4  2001/09/01 13:00:10  rainy
  Slight changes in the interface. The value is now measured only once if possible.

  Revision 1.3  2001/08/19 09:14:55  rainy
  Added support for value invert.
  Also the CMeasure's ReadConfig is executed.

  Revision 1.2  2001/08/12 15:44:49  Rainy
  Adjusted Update()'s interface.
  Net throughput is now measured in bytes per second and not bits.

  Revision 1.1.1.1  2001/08/11 10:58:19  Rainy
  Added to CVS.

*/
#pragma warning(disable: 4996)

#include "MeasureNetOut.h"

/*
** CMeasureNetOut
**
** The constructor
**
*/
CMeasureNetOut::CMeasureNetOut(CMeterWindow* meterWindow) : CMeasureNet(meterWindow)
{
	m_FirstTime = true;
	m_OutOctets = 0;
}

/*
** ~CMeasureNetOut
**
** The destructor
**
*/
CMeasureNetOut::~CMeasureNetOut()
{
}

/*
** Update
**
** Updates the current net out value. 
**
*/
bool CMeasureNetOut::Update()
{
	if (!CMeasureNet::PreUpdate()) return false;

	if(c_Table == NULL) return false;

	if (m_Cumulative)
	{
		m_Value = (double)GetNetStatsValue(NET_OUT).QuadPart;
	}
	else
	{
		DWORD value = 0;

		if (!m_FirstTime) 
		{
			value = GetNetOctets(NET_OUT);
			if (value > m_OutOctets)
			{
				DWORD tmpValue = value;
				value -= m_OutOctets;
				m_OutOctets = tmpValue;
			}
			else
			{
				m_OutOctets = value;
				value = 0;
			}
		}
		else
		{
			m_OutOctets = GetNetOctets(NET_OUT);
			m_FirstTime = false;
		}

		m_Value = value;
	}

	return PostUpdate();
}

/*
** ReadConfig
**
** Reads the measure specific configs.
**
*/
void CMeasureNetOut::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	CMeasure::ReadConfig(parser, section);
	CMeasureNet::ReadConfig(parser, section, NET_OUT);
}
