// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>

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
	IfActions() = default;

	IfActions(const IfActions& other) = delete;
	IfActions& operator=(IfActions other) = delete;

	void ReadOptions(ConfigParser& parser, std::wstring_view section);
	void ReadConditionOptions(ConfigParser& parser, std::wstring_view section);
	void DoIfActions(Measure& measure, double value);
	void SetState(double& value);

private:
	void DoValueActions(Measure& measure, double value);

	struct ValueActions
	{
		double aboveValue = 0.0;
		double belowValue = 0.0;
		int64_t equalValue = 0;

		std::wstring aboveAction;
		std::wstring belowAction;
		std::wstring equalAction;

		bool aboveCommitted = false;
		bool belowCommitted = false;
		bool equalCommitted = false;
	};

	struct ExpressionActions
	{
		std::vector<IfState> conditions;
		std::vector<IfState> matches;
		bool conditionMode = false;
		bool matchMode = false;
	};

	std::unique_ptr<ValueActions> m_ValueActions;
	std::unique_ptr<ExpressionActions> m_ExpressionActions;
};
