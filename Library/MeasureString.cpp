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

#include "StdAfx.h"
#include "MeasureString.h"
#include "Rainmeter.h"

/*
** The constructor
**
*/
MeasureString::MeasureString(MeterWindow* meterWindow, const WCHAR* name) : Measure(meterWindow, name),
	m_String()
{
}

/*
** The destructor
**
*/
MeasureString::~MeasureString()
{
}

/*
** Read the options specified in the ini file.
**
*/
void MeasureString::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);
	
	m_String = parser.ReadString(section, L"String", L"");
}

/*
** Converts the string to a number (if possible).
**
*/
void MeasureString::UpdateValue()
{
	m_Value = _wtof(m_String.c_str());
}

/*
** Returns the string value of the measure.
**
*/
const WCHAR* MeasureString::GetStringValue()
{
	return CheckSubstitute(m_String.c_str());
}
