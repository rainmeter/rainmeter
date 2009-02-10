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
  $Header: /home/cvsroot/Rainmeter/Library/MeasureNetTotal.cpp,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeasureNetTotal.cpp,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.7  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.6  2003/12/05 15:50:10  Rainy
  Multi-instance changes.

  Revision 1.5  2003/02/10 18:13:34  rainy
  Changed the way stats are gathered.

  Revision 1.4  2002/12/23 14:26:07  rainy
  Added cumulative statistics measuring.

  Revision 1.3  2002/07/01 15:34:38  rainy
  The measuring is done in the base class.

  Revision 1.2  2002/04/26 18:24:15  rainy
  Modified the Update method to support disabled measures.

  Revision 1.1  2002/04/01 15:35:27  rainy
  Initial version.


*/
#pragma warning(disable: 4996)

#include "MeasureNetTotal.h"

/*
** CMeasureNetTotal
**
** The constructor
**
*/
CMeasureNetTotal::CMeasureNetTotal(CMeterWindow* meterWindow) : CMeasureNet(meterWindow)
{
	m_FirstTime = true;
	m_TotalOctets = 0;
}

/*
** ~CMeasureNetTotal
**
** The destructor
**
*/
CMeasureNetTotal::~CMeasureNetTotal()
{
}

/*
** Update
**
** Updates the current net total value. 
**
*/
bool CMeasureNetTotal::Update()
{
	if (!CMeasureNet::PreUpdate()) return false;

	if(c_Table == NULL) return false;

	if (m_Cumulative)
	{
		m_Value = (double)GetNetStatsValue(NET_TOTAL).QuadPart;
	}
	else
	{
		DWORD value = 0;

		if (!m_FirstTime) 
		{
			value = GetNetOctets(NET_TOTAL);
			if (value > m_TotalOctets)
			{
				DWORD tmpValue = value;
				value -= m_TotalOctets;
				m_TotalOctets = tmpValue;
			}
			else
			{
				m_TotalOctets = value;
				value = 0;
			}
		}
		else
		{
			m_TotalOctets = GetNetOctets(NET_TOTAL);
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
void CMeasureNetTotal::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	CMeasure::ReadConfig(parser, section);
	CMeasureNet::ReadConfig(parser, section, NET_TOTAL);
}
