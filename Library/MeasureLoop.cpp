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
#include "../Common/MathParser.h"

MeasureLoop::MeasureLoop(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_StartValue(1),
	m_EndValue(100),
	m_Increment(1),
	m_IncSign(false),
	m_LoopCount(0),
	m_LoopCounter(0),
	m_SkipFirst(true)
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

		// Warn the user if the |m_EndValue| is never reached
		double temp = abs(fmod((m_EndValue - m_StartValue), m_Increment));
		if (temp != 0)
		{
			LogWarningF(this, L"EndValue=%i will never be reached", m_EndValue);
		}
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

	if (m_LoopCount == 0 || m_LoopCounter < m_LoopCount)
	{
		m_Value += m_Increment;

		// |m_Value| is beyond |m_EndValue|, so start loop over
		if ((m_IncSign && m_Value > m_EndValue) || (!m_IncSign && m_Value < m_EndValue))
		{
			m_Value = m_StartValue;
			++m_LoopCounter;
		}
	}
}

void MeasureLoop::Reset()
{
	m_Value = m_StartValue;
	m_LoopCounter = 0;
	m_SkipFirst = true;

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
