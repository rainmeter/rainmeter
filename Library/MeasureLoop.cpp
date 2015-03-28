/*
Copyright (C) 2015 Brian Ferguson

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
#include "MeasureLoop.h"
#include "Rainmeter.h"

MeasureLoop::MeasureLoop(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_StartValue(1),
	m_EndValue(100),
	m_Increment(1),
	m_IncSign(false),
	m_LoopCount(0),
	m_LoopCounter(0),
	m_SkipFirst(true),
	m_HasOverRun(false)
{
}

MeasureLoop::~MeasureLoop()
{
}

void MeasureLoop::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	double oldStart = m_StartValue, oldEnd = m_EndValue, oldInc = m_Increment;

	Measure::ReadOptions(parser, section);

	m_StartValue = parser.ReadInt(section, L"StartValue", 1);
	m_EndValue = parser.ReadInt(section, L"EndValue", 100);
	m_IncSign = (m_Increment = parser.ReadInt(section, L"Increment", 1)) > 0;

	m_LoopCount = parser.ReadInt(section, L"LoopCount", 0);

	if (!m_Initialized || oldStart != m_StartValue || oldEnd != m_EndValue || oldInc != m_Increment)
	{
		Reset();
	}
}

void MeasureLoop::UpdateValue()
{
	// Skip the first update of the loop, otherwise the |m_StartValue| gets skipped
	if (m_SkipFirst)
	{
		m_SkipFirst = false;
		return;
	}

	if (m_LoopCount <= 0 || m_LoopCounter < m_LoopCount)
	{
		m_Value += m_Increment;

		if ((m_IncSign && m_Value >= m_EndValue) || (!m_IncSign && m_Value <= m_EndValue))
		{
			// |m_Value| has overrun. Display the |m_EndValue| for exactly
			// one update cycle before starting over (if necessary)
			if (m_HasOverRun && (m_LoopCount <= 0 || m_LoopCounter <= m_LoopCount))
			{
				m_Value = m_StartValue;
				m_HasOverRun = false;
			}
			else
			{
				m_Value = m_EndValue;
				++m_LoopCounter;
				m_HasOverRun = true;
			}
		}
	}
}

void MeasureLoop::Reset()
{
	m_Value = m_StartValue;
	m_LoopCounter = 0;
	m_SkipFirst = true;
	m_HasOverRun = false;

	if (m_StartValue < m_EndValue)
	{
		m_MinValue = m_StartValue;
		m_MaxValue = m_EndValue;
	}
	else
	{
		m_MinValue = m_EndValue;
		m_MaxValue = m_StartValue;
	}
}

void MeasureLoop::Command(const std::wstring& command)
{
	if (_wcsicmp(command.c_str(), L"Reset") == 0)
	{
		Reset();
		return;
	}

	LogWarningF(this, L"!CommandMeasure: Not supported");
}
