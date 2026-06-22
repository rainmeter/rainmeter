/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureActionTimer.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "Rainmeter.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

struct MeasureActionTimer::Action
{
	std::vector<std::wstring> actions;
	Skin* skin;
	std::mutex mutex;
	std::condition_variable signal;
	std::condition_variable cleanUp;
	std::atomic<bool> interrupt;
	std::atomic<bool> isRunning;

	Action(Skin* skin) :
		actions(),
		skin(skin),
		interrupt(false),
		isRunning(false)
	{
	}
};

MeasureActionTimer::MeasureActionTimer(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Actions(),
	m_IgnoreWarnings(false)
{
}

MeasureActionTimer::~MeasureActionTimer()
{
	StopAllActions();

	for (auto action : m_Actions)
	{
		delete action;
	}
}

void MeasureActionTimer::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	size_t index = 1;
	std::wstring action = parser.ReadString(section, L"ActionList1", L"", false);
	while (!action.empty())
	{
		std::vector<std::wstring> tokens = ConfigParser::Tokenize(action, L"|");
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			if (_wcsnicmp(tokens[i].c_str(), L"REPEAT ", 7) == 0)
			{
				std::vector<std::wstring> repeat = ConfigParser::Tokenize(tokens[i].substr(7), L",");
				if (repeat.size() == 3)
				{
					tokens.erase(tokens.begin() + i);

					const std::wstring repeatedAction = parser.ReadString(section, repeat[0].c_str(), L"[]", false);
					const std::wstring wait = L"Wait " + repeat[1];
					const int size = (_wtoi(repeat[2].c_str()) * 2) - 1;
					if (size <= 0)
					{
						continue;
					}

					size_t j = 0;
					for (; j < (size_t)size; ++j)
					{
						tokens.insert(tokens.begin() + (i + j), (j % 2 == 0) ? repeatedAction : wait);
					}

					i += j - 1;
				}
			}
			else if (_wcsnicmp(tokens[i].c_str(), L"WAIT ", 5) != 0)
			{
				tokens[i] = parser.ReadString(section, tokens[i].c_str(), L"[]", false);
			}
		}

		if (index <= m_Actions.size())
		{
			std::lock_guard<std::mutex> lock(m_Actions[index - 1]->mutex);
			m_Actions[index - 1]->actions = tokens;
		}
		else
		{
			Action* newAction = new Action(GetSkin());
			std::lock_guard<std::mutex> lock(newAction->mutex);
			newAction->actions = tokens;
			m_Actions.push_back(newAction);
		}

		WCHAR buffer[64];
		_snwprintf_s(buffer, _TRUNCATE, L"ActionList%zu", ++index);
		action = parser.ReadString(section, buffer, L"", false);
	}

	m_IgnoreWarnings = parser.ReadInt(section, L"IgnoreWarnings", 0) != 0;
}

void MeasureActionTimer::Command(const std::wstring& command)
{
	const WCHAR* args = command.c_str();

	auto parseAndValidateIndex = [&](size_t& number, size_t length) -> bool
	{
		args += length;
		number = (size_t)(_wtoi(args) - 1);
		return number < m_Actions.size();
	};

	if (_wcsnicmp(args, L"EXECUTE", 7) == 0)
	{
		size_t number = 0;
		if (parseAndValidateIndex(number, 7))
		{
			if (!m_Actions[number]->isRunning.exchange(true))
			{
				std::thread thread(ExecuteAction, m_Actions[number]);
				thread.detach();
			}
			else if (!m_IgnoreWarnings)
			{
				LogWarningF(this, L"'ActionList%i' is currently running", (int)number + 1);
			}
		}
		else if (!m_IgnoreWarnings)
		{
			LogWarningF(this, L"Invalid index '%i'", (int)number + 1);
		}
	}
	else if (_wcsnicmp(args, L"STOP", 4) == 0)
	{
		size_t number = 0;
		if (parseAndValidateIndex(number, 4))
		{
			StopAction(m_Actions[number]);
		}
		else if (!m_IgnoreWarnings)
		{
			LogWarningF(this, L"Invalid index '%i'", (int)number + 1);
		}
	}
	else
	{
		LogErrorF(this, L"Unknown command: %s", args);
	}
}

void MeasureActionTimer::StopAction(Action* action)
{
	action->interrupt = true;
	action->signal.notify_all();

	{
		std::unique_lock<std::mutex> lock(action->mutex);
		action->cleanUp.wait(lock, [&](){ return action->isRunning == false; });
	}

	action->interrupt = false;
}

void MeasureActionTimer::StopAllActions()
{
	for (auto action : m_Actions)
	{
		StopAction(action);
	}
}

void MeasureActionTimer::ExecuteAction(Action* action)
{
	std::vector<std::wstring> actions;
	{
		std::lock_guard<std::mutex> lock(action->mutex);
		actions = action->actions;
	}

	for (const auto& actionCommand : actions)
	{
		if (_wcsnicmp(actionCommand.c_str(), L"WAIT ", 5) == 0)
		{
			__int64 timeout = _wtoi64(actionCommand.substr(5).c_str());
			if (timeout > 0)
			{
				std::unique_lock<std::mutex> lock(action->mutex);
				if (action->signal.wait_for(lock, std::chrono::milliseconds(timeout), [&](){ return action->interrupt == true; }))
				{
					action->isRunning = false;
					lock.unlock();
					action->cleanUp.notify_all();
					return;
				}

				continue;
			}
		}

		GetRainmeter().DelayedExecuteCommand(actionCommand.c_str(), action->skin);
	}

	action->isRunning = false;
	action->cleanUp.notify_all();
}
