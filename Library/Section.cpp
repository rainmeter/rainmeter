/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "Section.h"
#include "ConfigParser.h"
#include "Rainmeter.h"

Section::Section(Skin* skin, const WCHAR* name) : m_Skin(skin), m_Name(name),
	m_DynamicVariables(false),
	m_UpdateDivider(1),
	m_UpdateCounter(1)
{
}

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
	const int defaultUpdateDivider =
		m_Skin ? m_Skin->GetDefaultUpdateDivider() : 1;
	int updateDivider = parser.ReadInt(section, L"UpdateDivider", defaultUpdateDivider);
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
		GetRainmeter().ExecuteCommand(m_OnUpdateAction.c_str(), m_Skin);
	}
}
