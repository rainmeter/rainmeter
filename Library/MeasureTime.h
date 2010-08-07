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
	void TimeToString(WCHAR* buf, size_t bufLen, const WCHAR* format, const struct tm* time);

	std::wstring m_Format;
	LARGE_INTEGER m_DeltaTime;
	LARGE_INTEGER m_Time;
};

#endif
