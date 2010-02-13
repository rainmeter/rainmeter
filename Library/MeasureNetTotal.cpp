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

#include "StdAfx.h"
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
		m_Value = (double)GetNetStatsValue(NET_TOTAL);
	}
	else
	{
		ULONG64 value = 0;

		if (!m_FirstTime) 
		{
			value = GetNetOctets(NET_TOTAL);
			if (value > m_TotalOctets)
			{
				ULONG64 tmpValue = value;
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

		m_Value = (double)value;
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
