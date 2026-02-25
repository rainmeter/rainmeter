/* Copyright (C) 2014 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureString.h"
#include "Rainmeter.h"

MeasureString::MeasureString(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_String(),
	m_StringValue()
{
}

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
	m_StringValue = m_String;
	m_Value = _wtof(m_String.c_str());
}

/*
** Returns the string value of the measure.
**
*/
const WCHAR* MeasureString::GetStringValue()
{
	return CheckSubstitute(m_StringValue .c_str());
}
