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

#ifndef __MEASUREDISKSPACE_H__
#define __MEASUREDISKSPACE_H__

#include "Measure.h"

class MeasureDiskSpace : public Measure
{
public:
	MeasureDiskSpace(MeterWindow* meterWindow, const WCHAR* name);
	virtual ~MeasureDiskSpace();

	virtual UINT GetTypeID() { return TypeID<MeasureDiskSpace>(); }

	virtual const WCHAR* GetStringValue();

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	std::wstring m_Drive;
	std::wstring m_StringValue;
	bool m_Type;
	bool m_Total;
	bool m_Label;
	bool m_IgnoreRemovable;
	bool m_DiskQuota;

	ULONGLONG m_OldTotalBytes;
};

#endif
