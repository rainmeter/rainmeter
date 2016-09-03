/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureLoop.h"
#include "Rainmeter.h"

MeasureLoop::MeasureLoop(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_StartValue(1),
	m_EndValue(100),
	m_Increment(1),
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
	int oldStart = m_StartValue, oldEnd = m_EndValue, oldInc = m_Increment, oldCount = m_LoopCount;
	bool oldInvert = m_Invert;

	Measure::ReadOptions(parser, section);

	m_StartValue = parser.ReadInt(section, L"StartValue", 1);
	m_EndValue = parser.ReadInt(section, L"EndValue", 100);
	m_Increment = parser.ReadInt(section, L"Increment", 1);

	m_LoopCount = parser.ReadInt(section, L"LoopCount", 0);

	if (!m_Initialized || oldStart != m_StartValue || oldEnd != m_EndValue ||
		oldInc != m_Increment || oldCount != m_LoopCount || oldInvert != m_Invert)
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

		if ((m_Increment > 0 && m_Value >= m_EndValue) ||
			(m_Increment <= 0 && m_Value <= m_EndValue))
		{
			if (!m_Invert)
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
			else
			{
				// For inverted measures, the opposite needs to happen.
				// Display |m_StartValue| for exactly one update cycle
				// before starting over (if necessary)
				if (!m_HasOverRun && (m_LoopCount <= 0 || m_LoopCounter <= m_LoopCount))
				{
					m_Value = m_EndValue;
					++m_LoopCounter;
					m_HasOverRun = true;
				}
				else
				{
					m_Value = m_StartValue;
					m_HasOverRun = false;
				}
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
