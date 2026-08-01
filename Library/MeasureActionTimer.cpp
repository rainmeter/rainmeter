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
		m_Data(measure->m_Data),
		m_ActionIndex(index)
	{
	}

	void StartWorkOnWorkerThread() override;
	void FinishWorkOnMainThread() override;

	std::shared_ptr<MeasureActionTimer::SharedData> m_Data;
	size_t m_ActionIndex;
};

void MeasureActionTimer::ActionTimerTask::StartWorkOnWorkerThread()
{
	auto rainmeterWindow = GetRainmeter().GetWindow();

	size_t commandIndex = 0;
	while (!m_AbortRequested)
	{
		DWORD sleepTimeout = 0;

		{
			CriticalSectionLock lock(*m_Data->criticalSection);
			if (m_AbortRequested) break;
			if (!m_Data->active) break;

			if (m_ActionIndex >= m_Data->actions.size()) break;

			const auto& action = m_Data->actions[m_ActionIndex];
			if (commandIndex >= action.commands.size()) break;

			const auto& command = action.commands[commandIndex].c_str();
			if (_wcsnicmp(command, L"WAIT ", 5) == 0)
			{
				sleepTimeout = wcstoul(command + 5, nullptr, 10);
			}
			else
			{
				auto message = new ExecuteMessage{ m_Data, m_ActionIndex, commandIndex, action.generation };
				if (!PostMessage(rainmeterWindow, WM_RAINMETER_HANDLE_ACTION_TIMER_EXECUTE, (WPARAM)message, 0))
				{
					delete message;
				}
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
		CriticalSectionLock lock(*m_Data->criticalSection);
		if (m_Data->active && m_ActionIndex < m_Data->actions.size() && m_Data->actions[m_ActionIndex].task == this)
		{
			m_Data->actions[m_ActionIndex].task = nullptr;
		}
	}
}

MeasureActionTimer::MeasureActionTimer(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Data(std::make_shared<SharedData>(skin)),
	m_IgnoreWarnings(false)
{
}

MeasureActionTimer::~MeasureActionTimer()
{
	CriticalSectionLock lock(*m_Data->criticalSection);

	// The worker may keep the shared data alive so we need another flag that code can check to see
	// whether the measure still exists.
	m_Data->active = false;

	for (auto& action : m_Data->actions)
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
			CriticalSectionLock lock(*m_Data->criticalSection);
			if (index <= m_Data->actions.size())
			{
				m_Data->actions[index - 1].commands = std::move(tokens);
			}
			else
			{
				m_Data->actions.emplace_back(Action{ std::move(tokens) });
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

	auto parseIndex = [&](size_t& number, size_t length)
	{
		args += length;
		number = (size_t)(_wtoi(args) - 1);
	};

	if (_wcsnicmp(args, L"EXECUTE", 7) == 0)
	{
		size_t number = 0;
		parseIndex(number, 7);

		CriticalSectionLock lock(*m_Data->criticalSection);
		if (number < m_Data->actions.size())
		{
			if (!m_Data->actions[number].task)
			{
				m_Data->actions[number].task = ActionTimerTask::Create(this, number);
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
		parseIndex(number, 4);

		CriticalSectionLock lock(*m_Data->criticalSection);
		if (number < m_Data->actions.size())
		{
			if (m_Data->actions[number].task)
			{
				m_Data->actions[number].task->AbortWhenPossible();
				m_Data->actions[number].task = nullptr;
			}

			++m_Data->actions[number].generation;
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

void MeasureActionTimer::HandleExecuteMessage(WPARAM wParam, LPARAM lParam)
{
	auto message = (ExecuteMessage*)wParam;
	if (auto data = message->data.lock())
	{
		std::wstring command;
		Skin* skin = nullptr;

		{
			CriticalSectionLock lock(*data->criticalSection);
			if (data->active &&
				message->actionIndex < data->actions.size() &&
				message->commandIndex < data->actions[message->actionIndex].commands.size() &&
				message->generation == data->actions[message->actionIndex].generation)
			{
				command = data->actions[message->actionIndex].commands[message->commandIndex];
				skin = data->skin;
			}
		}

		if (skin && GetRainmeter().HasSkin(skin))
		{
			GetRainmeter().ExecuteCommand(command.c_str(), skin);
		}
	}

	delete message;
}
