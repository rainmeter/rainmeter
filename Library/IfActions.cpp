// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "Measure.h"
#include "IfActions.h"
#include "Rainmeter.h"
#include "../Common/MathParser.h"
#include "Pcre.h"

void IfActions::ReadOptions(ConfigParser& parser, std::wstring_view section)
{
	std::wstring aboveAction, belowAction, equalAction;
	parser.ReadString(aboveAction, section, L"IfAboveAction", L"", { .sectionVariables = false });
	parser.ReadString(belowAction, section, L"IfBelowAction", L"", { .sectionVariables = false });
	parser.ReadString(equalAction, section, L"IfEqualAction", L"", { .sectionVariables = false });

	if (aboveAction.empty() && belowAction.empty() && equalAction.empty())
	{
		m_ValueActions.reset();
		return;
	}

	if (!m_ValueActions) m_ValueActions = std::make_unique<ValueActions>();

	m_ValueActions->aboveAction = std::move(aboveAction);
	m_ValueActions->aboveValue = parser.ReadFloat(section, L"IfAboveValue", 0.0);

	m_ValueActions->belowAction = std::move(belowAction);
	m_ValueActions->belowValue = parser.ReadFloat(section, L"IfBelowValue", 0.0);

	m_ValueActions->equalAction = std::move(equalAction);
	m_ValueActions->equalValue = (int64_t)parser.ReadFloat(section, L"IfEqualValue", 0.0);
}

void IfActions::ReadConditionOptions(ConfigParser& parser, std::wstring_view section)
{
	std::wstring condition = parser.ReadString(section, L"IfCondition", L"");
	std::wstring match = parser.ReadString(section, L"IfMatch", L"");
	if (condition.empty() && match.empty())
	{
		m_ExpressionActions.reset();
		return;
	}

	if (!m_ExpressionActions) m_ExpressionActions = std::make_unique<ExpressionActions>();

	auto& actions = *m_ExpressionActions;

	// IfCondition options
	actions.conditionMode = parser.ReadBool(section, L"IfConditionMode", false);

	if (!condition.empty())
	{
		std::wstring tAction = parser.ReadString(section, L"IfTrueAction", L"", { .sectionVariables = false });
		std::wstring fAction = parser.ReadString(section, L"IfFalseAction", L"", { .sectionVariables = false });
		if (!tAction.empty() || !fAction.empty())
		{
			size_t i = 1;
			do
			{
				if (actions.conditions.size() > (i - 1))
				{
					actions.conditions[i - 1].Set(condition, tAction, fAction);
				}
				else
				{
					actions.conditions.emplace_back(condition, tAction, fAction);
				}

				// Check for IfCondition2/IfTrueAction2/IfFalseAction2 ... etc.
				const std::wstring num = std::to_wstring(++i);

				std::wstring key = L"IfCondition" + num;
				condition = parser.ReadString(section, key.c_str(), L"");
				if (condition.empty()) break;

				key = L"IfTrueAction" + num;
				tAction = parser.ReadString(section, key.c_str(), L"", { .sectionVariables = false });
				key = L"IfFalseAction" + num;
				fAction = parser.ReadString(section, key.c_str(), L"", { .sectionVariables = false });
			}
			while (!tAction.empty() || !fAction.empty());
		}
		else
		{
			actions.conditions.clear();
		}
	}
	else
	{
		actions.conditions.clear();
	}

	// IfMatch options
	actions.matchMode = parser.ReadBool(section, L"IfMatchMode", false);

	if (!match.empty())
	{
		std::wstring tAction = parser.ReadString(section, L"IfMatchAction", L"", { .sectionVariables = false });
		std::wstring fAction = parser.ReadString(section, L"IfNotMatchAction", L"", { .sectionVariables = false });
		if (!tAction.empty() || !fAction.empty())
		{
			size_t i = 1;
			do
			{
				if (actions.matches.size() > (i - 1))
				{
					actions.matches[i - 1].Set(match, tAction, fAction);
				}
				else
				{
					actions.matches.emplace_back(match, tAction, fAction);
				}

				// Check for IfMatch2/IfMatchAction2/IfNotMatchAction2 ... etc.
				const std::wstring num = std::to_wstring(++i);

				std::wstring key = L"IfMatch" + num;
				match = parser.ReadString(section, key.c_str(), L"");
				if (match.empty()) break;

				key = L"IfMatchAction" + num;
				tAction = parser.ReadString(section, key.c_str(), L"", { .sectionVariables = false });
				key = L"IfNotMatchAction" + num;
				fAction = parser.ReadString(section, key.c_str(), L"", { .sectionVariables = false });
			} while (!tAction.empty() || !fAction.empty());
		}
		else
		{
			actions.matches.clear();
		}
	}
	else
	{
		actions.matches.clear();
	}

	if (actions.conditions.empty() && actions.matches.empty())
	{
		m_ExpressionActions.reset();
	}
}

void IfActions::DoValueActions(Measure& measure, double value)
{
	auto& actions = *m_ValueActions;

	// IfEqual
	if (!actions.equalAction.empty())
	{
		if ((int64_t)value == actions.equalValue)
		{
			if (!actions.equalCommitted)
			{
				actions.equalCommitted = true;		// To avoid infinite loop from !Update
				GetRainmeter().ExecuteActionCommand(actions.equalAction.c_str(), &measure);
			}
		}
		else
		{
			actions.equalCommitted = false;
		}
	}

	// IfAbove
	if (!actions.aboveAction.empty())
	{
		if (value > actions.aboveValue)
		{
			if (!actions.aboveCommitted)
			{
				actions.aboveCommitted = true;		// To avoid infinite loop from !Update
				GetRainmeter().ExecuteActionCommand(actions.aboveAction.c_str(), &measure);
			}
		}
		else
		{
			actions.aboveCommitted = false;
		}
	}

	// IfBelow
	if (!actions.belowAction.empty())
	{
		if (value < actions.belowValue)
		{
			if (!actions.belowCommitted)
			{
				actions.belowCommitted = true;		// To avoid infinite loop from !Update
				GetRainmeter().ExecuteActionCommand(actions.belowAction.c_str(), &measure);
			}
		}
		else
		{
			actions.belowCommitted = false;
		}
	}
}

void IfActions::DoIfActions(Measure& measure, double value)
{
	if (m_ValueActions)
	{
		DoValueActions(measure, value);
	}

	if (!m_ExpressionActions) return;

	auto& actions = *m_ExpressionActions;

	// IfCondition
	int i = 0;
	for (auto& item : actions.conditions)
	{
		++i;
		if (!item.value.empty() && (!item.tAction.empty() || !item.fAction.empty()))
		{
			double result = 0.0;
			const WCHAR* errMsg = measure.GetSkin()->GetMathParser().Parse(item.value.c_str(), &result);
			if (errMsg != nullptr)
			{
				if (!item.parseError)
				{
					if (i == 1)
					{
						LogErrorF(&measure, L"%s: IfCondition=%s", errMsg, item.value.c_str());
					}
					else
					{
						LogErrorF(&measure, L"%s: IfCondition%i=%s", errMsg, i, item.value.c_str());
					}
					item.parseError = true;
				}
			}
			else
			{
				item.parseError = false;

				if (result == 1.0)			// "True"
				{
					item.fCommitted = false;

					if (actions.conditionMode || !item.tCommitted)
					{
						item.tCommitted = true;
						GetRainmeter().ExecuteActionCommand(item.tAction.c_str(), &measure);
					}
				}
				else if (result == 0.0)	// "False"
				{
					item.tCommitted = false;

					if (actions.conditionMode || !item.fCommitted)
					{
						item.fCommitted = true;
						GetRainmeter().ExecuteActionCommand(item.fAction.c_str(), &measure);
					}
				}
			}
		}
	}

	// IfMatch
	i = 0;
	for (auto& item : actions.matches)
	{
		++i;
		if (!item.value.empty() && (!item.tAction.empty() || !item.fAction.empty()))
		{
			const char* error;

			Pcre re(item.value.c_str(), &error);
			if (!re)
			{
				if (!item.parseError)
				{
					if (i == 1)
					{
						LogErrorF(&measure, L"Error: \"%S\" in IfMatch=%s", error, item.value.c_str());
					}
					else
					{
						LogErrorF(&measure, L"Error: \"%S\" in IfMatch%i=%s", error, i, item.value.c_str());
					}

					item.parseError = true;
				}
			}
			else
			{
				item.parseError = false;

				const WCHAR* value = measure.GetStringValue();
				std::wstring_view str = value ? value : L"";
				int ovector[300];
				int rc = re.Execute(str, 0, ovector, (int)_countof(ovector));
				if (rc > 0)		// Match
				{
					item.fCommitted = false;

					if (actions.matchMode || !item.tCommitted)
					{
						item.tCommitted = true;
						GetRainmeter().ExecuteActionCommand(item.tAction.c_str(), &measure);
					}
				}
				else			// Not Match
				{
					item.tCommitted = false;

					if (actions.matchMode || !item.fCommitted)
					{
						item.fCommitted = true;
						GetRainmeter().ExecuteActionCommand(item.fAction.c_str(), &measure);
					}
				}
			}
		}
	}
}

void IfActions::SetState(double& value)
{
	// Set IfAction committed state to false if condition is not met with value = 0
	if (m_ValueActions)
	{
		auto& actions = *m_ValueActions;

		if (actions.equalValue != (int64_t)value)
		{
			actions.equalCommitted = false;
		}

		if (actions.aboveValue <= value)
		{
			actions.aboveCommitted = false;
		}

		if (actions.belowValue >= value)
		{
			actions.belowCommitted = false;
		}
	}

	if (!m_ExpressionActions) return;

	for (auto& item : m_ExpressionActions->conditions)
	{
		item.tCommitted = false;
		item.fCommitted = false;
	}

	for (auto& item : m_ExpressionActions->matches)
	{
		item.tCommitted = false;
		item.fCommitted = false;
	}
}
