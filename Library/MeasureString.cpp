// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasureString.h"
#include "Rainmeter.h"

MeasureString::MeasureString(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_NumberFormat(LocaleUtil::NumberFormat::Default),
	m_CurrentString(),
	m_PendingString()
{
}

MeasureString::~MeasureString()
{
}

void MeasureString::ReadOptions(ConfigParser::OptionReader& reader)
{
	Measure::ReadOptions(reader);

	m_NumberFormat = ReadNumberFormatOption(reader);

	reader.ReadString<"String">(m_PendingString.emplace(), L"");

	// Publish the initial value so meters can use it before the first measure update. A measure
	// that starts disabled or paused should keep it pending and return an empty string.
	if (!m_Initialized && !m_Disabled && !m_Paused)
	{
		m_CurrentString = std::move(*m_PendingString);
		m_PendingString.reset();
	}
}

void MeasureString::UpdateValue()
{
	if (m_PendingString)
	{
		m_CurrentString = std::move(*m_PendingString);
		m_PendingString.reset();
	}

	m_Value = LocaleUtil::StringToNumber(m_CurrentString.c_str(), m_NumberFormat);
}

std::optional<std::wstring_view> MeasureString::GetStringValue()
{
	return CheckSubstitute(m_CurrentString);
}
