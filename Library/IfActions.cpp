/*
  Copyright (C) 2013 Brian Ferguson

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
#include "Measure.h"
#include "IfActions.h"
#include "Rainmeter.h"
#include "../Common/MathParser.h"

IfActions::IfActions() :
	m_AboveValue(0.0f),
	m_BelowValue(0.0f),
	m_EqualValue(0),
	m_AboveAction(),
	m_BelowAction(),
	m_EqualAction(),
	m_AboveCommitted(false),
	m_BelowCommitted(false),
	m_EqualCommitted(false),
	m_Conditions()
{
}

IfActions::~IfActions()
{
}

void IfActions::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	m_AboveAction = parser.ReadString(section, L"IfAboveAction", L"", false);
	m_AboveValue = parser.ReadFloat(section, L"IfAboveValue", 0.0f);

	m_BelowAction = parser.ReadString(section, L"IfBelowAction", L"", false);
	m_BelowValue = parser.ReadFloat(section, L"IfBelowValue", 0.0f);

	m_EqualAction = parser.ReadString(section, L"IfEqualAction", L"", false);
	m_EqualValue = (int64_t)parser.ReadFloat(section, L"IfEqualValue", 0.0f);
}

void IfActions::ReadConditionOptions(ConfigParser& parser, const WCHAR* section)
{
	std::wstring condition = parser.ReadString(section, L"IfCondition", L"");
	std::wstring tAction = parser.ReadString(section, L"IfTrueAction", L"", false);
	std::wstring fAction = parser.ReadString(section, L"IfFalseAction", L"", false);
	if (!condition.empty() && (!tAction.empty() || !fAction.empty()))
	{
		int i = 1;
		do
		{
			if (m_Conditions.size() > (i - 1))
			{
				m_Conditions[i - 1].Set(condition, tAction, fAction);
			}
			else
			{
				m_Conditions.emplace_back(condition, tAction, fAction);
			}

			// Check for IfCondition2/IfTrueAction2/IfFalseAction2 ... etc.
			std::wstring key = L"IfTrueAction" + std::to_wstring(++i);
			tAction = parser.ReadString(section, key.c_str(), L"", false);
			key = L"IfFalseAction" + std::to_wstring(i);
			fAction = parser.ReadString(section, key.c_str(), L"", false);

			key = L"IfCondition" + std::to_wstring(i);
			condition = parser.ReadString(section, key.c_str(), L"");

		}
		while (!condition.empty() && (!tAction.empty() || !fAction.empty()));
	}
	else
	{
		m_Conditions.clear();
	}
}

void IfActions::DoIfActions(Measure& measure, double value)
{
	if (!m_EqualAction.empty())
	{
		if ((int64_t)value == m_EqualValue)
		{
			if (!m_EqualCommitted)
			{
				m_EqualCommitted = true;		// To avoid infinite loop from !Update
				GetRainmeter().ExecuteCommand(m_EqualAction.c_str(), measure.GetMeterWindow());
			}
		}
		else
		{
			m_EqualCommitted = false;
		}
	}

	if (!m_AboveAction.empty())
	{
		if (value > m_AboveValue)
		{
			if (!m_AboveCommitted)
			{
				m_AboveCommitted = true;		// To avoid infinite loop from !Update
				GetRainmeter().ExecuteCommand(m_AboveAction.c_str(), measure.GetMeterWindow());
			}
		}
		else
		{
			m_AboveCommitted = false;
		}
	}

	if (!m_BelowAction.empty())
	{
		if (value < m_BelowValue)
		{
			if (!m_BelowCommitted)
			{
				m_BelowCommitted = true;		// To avoid infinite loop from !Update
				GetRainmeter().ExecuteCommand(m_BelowAction.c_str(), measure.GetMeterWindow());
			}
		}
		else
		{
			m_BelowCommitted = false;
		}
	}

	int i = 0;
	for (auto& item : m_Conditions)
	{
		++i;
		if (!item.condition.empty() && (!item.tAction.empty() || !item.fAction.empty()))
		{
			double result = 0.0f;
			const WCHAR* errMsg = MathParser::Parse(
				item.condition.c_str(), &result, measure.GetCurrentMeasureValue, &measure);
			if (errMsg != nullptr)
			{
				if (!item.parseError)
				{
					if (i == 1)
					{
						LogErrorF(&measure, L"%s: IfCondition=%s", errMsg, item.condition.c_str());
					}
					else
					{
						LogErrorF(&measure, L"%s: IfCondition%i=%s", errMsg, i, item.condition.c_str());
					}
					item.parseError = true;
				}
			}
			else
			{
				item.parseError = false;

				if (result == 1.0f)			// "True"
				{
					GetRainmeter().ExecuteCommand(item.tAction.c_str(), measure.GetMeterWindow());
				}
				else if (result == 0.0f)	// "False"
				{
					GetRainmeter().ExecuteCommand(item.fAction.c_str(), measure.GetMeterWindow());
				}
			}
		}
	}
}

void IfActions::SetState(double value)
{
	// Set IfAction committed state to false if condition is not met with value = 0
	if (m_EqualValue != 0)
	{
		m_EqualCommitted = false;
	}

	if (m_AboveValue <= 0.0)
	{
		m_AboveCommitted = false;
	}

	if (m_BelowValue >= 0.0)
	{
		m_BelowCommitted = false;
	}
}
