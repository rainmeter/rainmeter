/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __IFACTIONS_H__
#define __IFACTIONS_H__

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

	void ReadOptions(ConfigParser& parser, const WCHAR* section);
	void ReadConditionOptions(ConfigParser& parser, const WCHAR* section);
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
#endif
