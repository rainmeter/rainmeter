// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "Section.h"
#include "ConfigParser.h"
#include "Rainmeter.h"

Section::Section(Skin* skin, const WCHAR* name) :
	m_Name(name),
	m_ID(IniNameRegistry::InternSection(name)),
	m_DynamicVariables(false),
	m_UpdateDivider(1),
	m_UpdateCounter(1),
	m_Skin(skin)
{
}

Section::~Section()
{
}

void Section::ReadOptions(ConfigParser& parser, bool allowMeterStyle)
{
	ConfigParser::OptionReader optionReader(parser, m_Name, m_ID, allowMeterStyle);
	ReadOptions(optionReader);
}

// Read the common options specified in the ini file. The inherited classes must
// call this base implementation if they overwrite this method.
void Section::ReadOptions(ConfigParser::OptionReader& reader)
{
	auto& parser = reader.GetParser();
	const int defaultUpdateDivider =
		m_Skin ? m_Skin->GetDefaultUpdateDivider() : 1;
	int updateDivider = parser.ReadInt<"UpdateDivider">(m_ID, defaultUpdateDivider);
	if (updateDivider != m_UpdateDivider)
	{
		m_UpdateCounter = m_UpdateDivider = updateDivider;
	}

	m_DynamicVariables = parser.ReadBool<"DynamicVariables">(m_ID, false);

	parser.ReadString<"OnUpdateAction">(m_OnUpdateAction, m_ID, L"", { .sectionVariables = false });

	const std::wstring& group = parser.ReadString<"Group">(m_ID, L"");
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
