// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include <memory>
#include "ConfigParser.h"

class Measure;

class IfActions
{
public:
	IfActions();
	~IfActions();

	IfActions(const IfActions& other) = delete;
	IfActions& operator=(IfActions other) = delete;

	void ReadOptions(ConfigParser::OptionReader& reader);
	void ReadConditionOptions(ConfigParser::OptionReader& reader);
	void DoIfActions(Measure& measure, double value);
	void SetState(double& value);

private:
	void DoValueActions(Measure& measure, double value);

	struct ValueActions;
	struct ExpressionActions;

	std::unique_ptr<ValueActions> m_ValueActions;
	std::unique_ptr<ExpressionActions> m_ExpressionActions;
};
