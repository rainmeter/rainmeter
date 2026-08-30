// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

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

// Read the common options specified in the ini file. The inherited classes must
// call this base implementation if they overwrite this method.
void Section::ReadOptions(ConfigParser& parser, std::wstring_view section)
{
	const int defaultUpdateDivider =
		m_Skin ? m_Skin->GetDefaultUpdateDivider() : 1;
	int updateDivider = parser.ReadInt(section, L"UpdateDivider", defaultUpdateDivider);
	if (updateDivider != m_UpdateDivider)
	{
		m_UpdateCounter = m_UpdateDivider = updateDivider;
	}

	m_DynamicVariables = parser.ReadBool(section, L"DynamicVariables", false);

	parser.ReadString(m_OnUpdateAction, section, L"OnUpdateAction", L"", { .sectionVariables = false });

	const std::wstring& group = parser.ReadString(section, L"Group", L"");
	InitializeGroup(group);
}

void Section::AdvanceUpdateCounter(UINT count)
{
	if (m_UpdateDivider > 0) m_UpdateCounter = std::min(m_UpdateCounter + (int)count, m_UpdateDivider);
}

bool Section::UpdateCounter()
{
	++m_UpdateCounter;
	if (m_UpdateCounter < m_UpdateDivider) return false;
	m_UpdateCounter = 0;

	return true;
}

void Section::DoUpdateAction()
{
	if (!m_OnUpdateAction.empty())
	{
		GetRainmeter().ExecuteActionCommand(m_OnUpdateAction.c_str(), this);
	}
}
