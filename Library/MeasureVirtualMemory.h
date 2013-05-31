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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __MEASUREVIRTUALMEMORY_H__
#define __MEASUREVIRTUALMEMORY_H__

#include "Measure.h"

class MeasureVirtualMemory : public Measure
{
public:
	MeasureVirtualMemory(MeterWindow* meterWindow, const WCHAR* name);
	virtual ~MeasureVirtualMemory();

	virtual UINT GetTypeID() { return TypeID<MeasureVirtualMemory>(); }

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	bool m_Total;
};

#endif
