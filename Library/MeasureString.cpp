// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasureString.h"
#include "Rainmeter.h"

MeasureString::MeasureString(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_NumberFormat(LocaleUtil::NumberFormat::Default),
	m_String(),
	m_StringValue()
{
}

MeasureString::~MeasureString()
{
}

void MeasureString::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	m_NumberFormat = ReadNumberFormatOption(parser, section);

	m_String = parser.ReadString(section, L"String", L"");

	if (!m_Initialized && !m_Disabled && !m_Paused)
	{
		// This sets the "initial" value of the measure to be more consistent with how
		// other measures work. A measure that is initially disabled/paused should
		// return an empty string.
		m_StringValue = m_String;
	}
}

void MeasureString::UpdateValue()
{
	m_StringValue = m_String;
	m_Value = LocaleUtil::StringToNumber(m_String.c_str(), m_NumberFormat);
}

const WCHAR* MeasureString::GetStringValue()
{
	return CheckSubstitute(m_StringValue.c_str());
}
