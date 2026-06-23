/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureActionTimer.h"
#include "AsyncTask.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "Rainmeter.h"

class MeasureActionTimer::ActionTimerTask : public AsyncTask
{
public:
	static ActionTimerTask* Create(MeasureActionTimer* measure, size_t index)
	{
		auto* task = new ActionTimerTask(measure, index);
		if (!task->Start())
		{
			delete task;
			return nullptr;
		}

		return task;
	}

private:
	ActionTimerTask(MeasureActionTimer* measure, size_t index) :
		AsyncTask(measure),
		m_MeasureCriticalSection(measure->m_ActionsCriticalSection),
		m_ActionIndex(index)
	{
	}

	void StartWorkOnWorkerThread() override;
	void FinishWorkOnMainThread() override;

	std::shared_ptr<CriticalSection> m_MeasureCriticalSection;
	size_t m_ActionIndex;
};

void MeasureActionTimer::ActionTimerTask::StartWorkOnWorkerThread()
{
	size_t commandIndex = 0;
	while (!m_AbortRequested)
	{
		DWORD sleepTimeout = 0;

		{
			CriticalSectionLock lock(*m_MeasureCriticalSection);
			if (m_AbortRequested) break;

			auto measure = (MeasureActionTimer*)m_Requestor;
			if (m_ActionIndex >= measure->m_Actions.size()) break;

			const auto& commands = measure->m_Actions[m_ActionIndex].commands;
			if (commandIndex >= commands.size()) break;

			const auto& command = commands[commandIndex].c_str();
			if (_wcsnicmp(command, L"WAIT ", 5) == 0)
			{
				sleepTimeout = wcstoul(command + 5, nullptr, 10);
			}
			else
			{
				GetRainmeter().DelayedExecuteCommand(command, measure->GetSkin());
			}
		}

		if (sleepTimeout > 0)
		{
			Sleep(sleepTimeout);
		}

		++commandIndex;
	}
}

void MeasureActionTimer::ActionTimerTask::FinishWorkOnMainThread()
{
	if (!m_AbortRequested)
	{
		auto measure = (MeasureActionTimer*)m_Requestor;
		if (m_ActionIndex < measure->m_Actions.size() && measure->m_Actions[m_ActionIndex].task == this)
		{
			measure->m_Actions[m_ActionIndex].task = nullptr;
		}
	}
}

MeasureActionTimer::MeasureActionTimer(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_ActionsCriticalSection(std::make_shared<CriticalSection>()),
	m_Actions(),
	m_IgnoreWarnings(false)
{
}

MeasureActionTimer::~MeasureActionTimer()
{
	// m_Actions will be destroyed outside the scope of the lock, but since we set the abort flag
	// within the lock, we can know that the worker won't have a live reference to m_Actions.
	CriticalSectionLock lock(*m_ActionsCriticalSection);

	for (auto& action : m_Actions)
	{
		if (action.task)
		{
			action.task->AbortWhenPossible();
			action.task = nullptr;
		}
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

					parser.ReplaceMeasures(repeat[1]);
					parser.ReplaceMeasures(repeat[2]);

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
			else if (_wcsnicmp(tokens[i].c_str(), L"WAIT ", 5) == 0)
			{
				parser.ReplaceMeasures(tokens[i]);
			}
			else
			{
				tokens[i] = parser.ReadString(section, tokens[i].c_str(), L"[]", false);
			}
		}

		{
			CriticalSectionLock lock(*m_ActionsCriticalSection);
			if (index <= m_Actions.size())
			{
				m_Actions[index - 1].commands = std::move(tokens);
			}
			else
			{
				m_Actions.emplace_back(Action{ std::move(tokens) });
			}
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
			if (!m_Actions[number].task)
			{
				m_Actions[number].task = ActionTimerTask::Create(this, number);
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
			if (m_Actions[number].task)
			{
				// Lock not needed because the task checks the abort flag before executing each command.
				m_Actions[number].task->AbortWhenPossible();
				m_Actions[number].task = nullptr;
			}
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
