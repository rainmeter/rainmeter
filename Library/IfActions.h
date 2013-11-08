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

#ifndef __IFACTIONS_H__
#define __IFACTIONS_H__

#include <windows.h>
#include <string>
#include <vector>

class ConfigParser;
class Measure;
class MeterWindow;

class IfCondition
{
public:
	IfCondition(std::wstring value, std::wstring trueAction, std::wstring falseAction) :
		condition(),
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
		this->condition = value;
		this->tAction = trueAction;
		this->fAction = falseAction;
	}

	std::wstring condition;		// IfCondition
	std::wstring tAction;		// IfTrueAction
	std::wstring fAction;		// IfFalseAction
	bool parseError;
	bool tCommitted;
	bool fCommitted;
};

class IfActions
{
public:
	IfActions();
	~IfActions();

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

	std::vector<IfCondition> m_Conditions;
	bool m_ConditionMode;
};
#endif
