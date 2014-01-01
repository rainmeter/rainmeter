/*
Copyright (C) 2014 Brian Ferguson

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

#ifndef __MEASURESTRING_H__
#define __MEASURESTRING_H__

#include <cstdarg>
#include "Measure.h"

class MeasureString : public Measure
{
public:
	MeasureString(MeterWindow* meterWindow, const WCHAR* name);
	virtual ~MeasureString();

	virtual const WCHAR* GetStringValue();

	virtual UINT GetTypeID() { return TypeID<MeasureString>(); }

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	std::wstring m_String;
};

#endif
