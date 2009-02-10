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
  $Header: /home/cvsroot/Rainmeter/Library/MeasureRegistry.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeasureRegistry.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.4  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.3  2002/04/27 10:28:57  rainy
  Added possibility to use other HKEYs also.

  Revision 1.2  2002/04/26 18:24:15  rainy
  Modified the Update method to support disabled measures.

  Revision 1.1  2001/10/28 09:07:18  rainy
  Inital version

*/

#ifndef __MEASUREREGISTRY_H__
#define __MEASUREREGISTRY_H__

#include "Measure.h"

class CMeasureRegistry : public CMeasure
{
public:
	CMeasureRegistry(CMeterWindow* meterWindow);
	virtual ~CMeasureRegistry();

	virtual void ReadConfig(CConfigParser& parser, const WCHAR* section);
	virtual bool Update();
	virtual const WCHAR* GetStringValue(bool autoScale, double scale, int decimals, bool percentual);

private:
	std::wstring m_RegKeyName;
	std::wstring m_RegValueName;
	std::wstring m_StringValue;
    HKEY m_RegKey;
    HKEY m_HKey;
};

#endif
