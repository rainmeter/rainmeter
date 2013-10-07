/*
  Copyright (C) 2013 spx

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
#include "Section.h"
#include "ConfigParser.h"
#include "Rainmeter.h"

/*
** The constructor
**
*/
Section::Section(MeterWindow* meterWindow, const WCHAR* name) : m_MeterWindow(meterWindow), m_Name(name),
	m_DynamicVariables(false),
	m_UpdateDivider(1),
	m_UpdateCounter(1)
{
}

/*
** The destructor
**
*/
Section::~Section()
{
}

/*
** Read the common options specified in the ini file. The inherited classes must
** call this base implementation if they overwrite this method.
**
*/
void Section::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	int updateDivider = parser.ReadInt(section, L"UpdateDivider", 1);
	if (updateDivider != m_UpdateDivider)
	{
		m_UpdateCounter = m_UpdateDivider = updateDivider;
	}

	m_DynamicVariables = parser.ReadBool(section, L"DynamicVariables", false);

	m_OnUpdateAction = parser.ReadString(section, L"OnUpdateAction", L"", false);

	const std::wstring& group = parser.ReadString(section, L"Group", L"");
	InitializeGroup(group);
}

/*
** Updates the counter value
**
*/
bool Section::UpdateCounter()
{
	++m_UpdateCounter;
	if (m_UpdateCounter < m_UpdateDivider) return false;
	m_UpdateCounter = 0;

	return true;
}

/*
** Execute OnUpdateAction if action is set
**
*/
void Section::DoUpdateAction()
{
	if (!m_OnUpdateAction.empty())
	{
		GetRainmeter().ExecuteCommand(m_OnUpdateAction.c_str(), m_MeterWindow);
	}
}
