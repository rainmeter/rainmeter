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

#ifndef __MEASUREREGISTRY_H__
#define __MEASUREREGISTRY_H__

#include "Measure.h"

class MeasureRegistry : public Measure
{
public:
	MeasureRegistry(MeterWindow* meterWindow, const WCHAR* name);
	virtual ~MeasureRegistry();

	virtual UINT GetTypeID() { return TypeID<MeasureRegistry>(); }

	virtual const WCHAR* GetStringValue();

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	std::wstring m_RegKeyName;
	std::wstring m_RegValueName;
	std::wstring m_StringValue;
    HKEY m_RegKey;
    HKEY m_HKey;
};

#endif
