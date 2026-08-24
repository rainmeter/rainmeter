// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <windows.h>
#include <string>
#include <vector>

class ConfigParser;
class Measure;
class Skin;

// Helper class for IfCondition/IfMatch
class IfState
{
public:
	IfState(std::wstring value, std::wstring trueAction, std::wstring falseAction) :
		value(),
		tAction(),
		fAction(),
		parseError(false),
		tCommitted(false),
		fCommitted(false)
	{
		Set(value, trueAction, falseAction);
	}

	inline void Set(std::wstring value, std::wstring trueAction, std::wstring falseAction)
	{
		this->value = value;
		this->tAction = trueAction;
		this->fAction = falseAction;
	}

	std::wstring value;			// IfCondition/IfMatch
	std::wstring tAction;		// IfTrueAction/IfMatchAction
	std::wstring fAction;		// IfFalseAction/IfNotMatchAction
	bool parseError;
	bool tCommitted;
	bool fCommitted;
};

class IfActions
{
public:
	IfActions();
	~IfActions();

	IfActions(const IfActions& other) = delete;
	IfActions& operator=(IfActions other) = delete;

	void ReadOptions(ConfigParser& parser, std::wstring_view section);
	void ReadConditionOptions(ConfigParser& parser, std::wstring_view section);
	void DoIfActions(Measure& measure, double value);
	void SetState(double& value);

private:
	double m_AboveValue;
	double m_BelowValue;
	int64_t m_EqualValue;

	std::wstring m_AboveAction;
	std::wstring m_BelowAction;
	std::wstring m_EqualAction;

	bool m_AboveCommitted;
	bool m_BelowCommitted;
	bool m_EqualCommitted;

	std::vector<IfState> m_Conditions;
	bool m_ConditionMode;

	std::vector<IfState> m_Matches;
	bool m_MatchMode;
};
