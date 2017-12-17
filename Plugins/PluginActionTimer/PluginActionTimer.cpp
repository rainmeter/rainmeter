/* Copyright (C) 2015 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <Windows.h>
#include "../API/RainmeterAPI.h"

// Copied from Rainmeter.h
#define RAINMETER_CLASS_NAME				L"DummyRainWClass"
#define RAINMETER_WINDOW_NAME				L"Rainmeter control window"
#define WM_RAINMETER_EXECUTE WM_APP + 2

// Copied from Rainmeter library
std::vector<std::wstring> Tokenize(const std::wstring& str, const std::wstring& delimiters)
{
	std::vector<std::wstring> tokens;

	size_t lastPos, pos = 0;
	do
	{
		lastPos = str.find_first_not_of(delimiters, pos);
		if (lastPos == std::wstring::npos) break;

		pos = str.find_first_of(delimiters, lastPos + 1);
		std::wstring token = str.substr(lastPos, pos - lastPos);  // len = (pos != std::wstring::npos) ? pos - lastPos : pos

		size_t pos2 = token.find_first_not_of(L" \t\r\n");
		if (pos2 != std::wstring::npos)
		{
			size_t lastPos2 = token.find_last_not_of(L" \t\r\n");
			if (pos2 != 0 || lastPos2 != (token.size() - 1))
			{
				// Trim white-space
				token.assign(token, pos2, lastPos2 - pos2 + 1);
			}
			tokens.push_back(token);
		}

		if (pos == std::wstring::npos) break;
		++pos;
	} while (true);

	return tokens;
}

struct Action
{
	std::vector<std::wstring> action;

	HWND rainmeterWindow;
	void* skin;
	std::mutex mutex;
	std::atomic<bool> interrupt;
	std::atomic<bool> isRunning;
	std::condition_variable signal;
	std::condition_variable cleanUp;

	Action(HWND _hwnd, void* _skin) :
		action(),
		rainmeterWindow(_hwnd),
		skin(_skin),
		interrupt(false),
		isRunning(false)
	{ }
};

struct Measure
{
	std::vector<Action*> list;
	bool ignoreWarnings;

	void* rm;
	HWND mainRainmeterWindow;

	Measure() : ignoreWarnings(false), mainRainmeterWindow(nullptr) { }
};

void ExecuteAction(Action* action);

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* measure = new Measure;
	*data = measure;

	measure->rm = rm;
	measure->mainRainmeterWindow = FindWindow(RAINMETER_CLASS_NAME, RAINMETER_WINDOW_NAME);
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	Measure* measure = (Measure*)data;

	void* skin = RmGetSkin(rm);

	size_t i = 1;
	std::vector<std::wstring> tokens;
	std::wstring action = RmReadString(rm, L"ActionList1", L"", FALSE);
	while (!action.empty())
	{
		tokens = Tokenize(action, L"|");
		for (size_t j = 0; j < tokens.size(); ++j)
		{
			if (_wcsnicmp(tokens[j].c_str(), L"REPEAT ", 7) == 0)
			{
				std::vector<std::wstring> repeat = Tokenize(tokens[j].substr(7), L",");
				if (repeat.size() == 3)
				{
					// Erase repeat command from tokens if valid;
					tokens.erase(tokens.begin() + j);

					// Replace any section variables
					repeat[1] = RmReplaceVariables(rm, repeat[1].c_str());  // Wait time
					repeat[2] = RmReplaceVariables(rm, repeat[2].c_str());  // Number of repeats

					const std::wstring act = RmReadString(rm, repeat[0].c_str(), L"[]", FALSE);
					const std::wstring wait = L"Wait " + repeat[1];
					const int size = (_wtoi(repeat[2].c_str()) * 2) - 1;	// because we dont want a |wait| after the last command
					if (size <= 0) continue;

					// Insert a |wait| in between each |act|.
					size_t k = 0;
					for (; k < size; ++k)
					{
						if (k % 2 == 0) tokens.insert(tokens.begin() + (j + k), act);
						else            tokens.insert(tokens.begin() + (j + k), wait);
					}

					j += (k - 1);		// Skip over inserted items
				}
			}
			else if (_wcsnicmp(tokens[j].c_str(), L"WAIT ", 5) == 0)
			{
				tokens[j] = RmReplaceVariables(rm, tokens[j].c_str());
			}
			else
			{
				tokens[j] = RmReadString(rm, tokens[j].c_str(), L"[]", FALSE);
			}
		}

		if (i <= measure->list.size())
		{
			std::lock_guard<std::mutex> lock(measure->list[i - 1]->mutex);
			measure->list[i - 1]->action = tokens;		// Update the command instead of creating new one
		}
		else
		{
			Action* act = new Action(measure->mainRainmeterWindow, skin);
			std::lock_guard<std::mutex> lock(act->mutex);
			act->action = tokens;
			measure->list.push_back(act);
		}

		action = RmReadString(rm, std::wstring(L"ActionList" + std::to_wstring(++i)).c_str(), L"", FALSE);
	}

	measure->ignoreWarnings = 0 != RmReadInt(rm, L"IgnoreWarnings", 0);
}

PLUGIN_EXPORT double Update(void* data)
{
	return 0.0;
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	Measure* measure = (Measure*)data;

	auto ParseAndValidateIndex = [&](size_t& number, int length) -> bool
	{
		args += length;
		number = (size_t)(_wtoi(args) - 1);
		if (number >= 0 && number < measure->list.size())
			return true;

		return false;
	};

	if (_wcsnicmp(args, L"EXECUTE", 7) == 0)
	{
		size_t number = 0;
		if (ParseAndValidateIndex(number, 7))
		{
			if (!measure->list[number]->isRunning)
			{
				std::thread thread(ExecuteAction, measure->list[number]);
				thread.detach();
			}
			else if (!measure->ignoreWarnings)
			{
				RmLogF(measure->rm, LOG_WARNING, L"'ActionList%i' is currently running", number + 1);
			}
		}
		else if (!measure->ignoreWarnings)
		{
			RmLogF(measure->rm, LOG_WARNING, L"Invalid index '%i'", number + 1);
		}
	}
	else if (_wcsnicmp(args, L"STOP", 4) == 0)
	{
		size_t number;
		if (ParseAndValidateIndex(number, 4))
		{
			measure->list[number]->interrupt = true;
			measure->list[number]->signal.notify_all();

			// Wait for thread to finish
			{
				std::unique_lock<std::mutex> lock(measure->list[number]->mutex);
				measure->list[number]->cleanUp.wait(lock, [&](){ return measure->list[number]->isRunning == false; });
			}

			measure->list[number]->interrupt = false;	// reset signal for future use
		}
		else if (!measure->ignoreWarnings)
		{
			RmLogF(measure->rm, LOG_WARNING, L"Invalid index '%i'", number + 1);
		}
	}
	else
	{
		RmLogF(measure->rm, LOG_ERROR, L"Unknown command: %s", args);
	}
}

PLUGIN_EXPORT void Finalize(void* data)
{
	Measure* measure = (Measure*)data;

	for (size_t i = 0; i < measure->list.size(); ++i)
	{
		measure->list[i]->interrupt = true;
		measure->list[i]->signal.notify_all();

		// Wait for thread to finish
		{
			std::unique_lock<std::mutex> lock(measure->list[i]->mutex);
			measure->list[i]->cleanUp.wait(lock, [&](){ return measure->list[i]->isRunning == false; });
		}

		delete measure->list[i];
	}

	measure->list.clear();
	delete measure;
}

void ExecuteAction(Action* action)
{
	action->isRunning = true;

	for (size_t i = 0; i < action->action.size(); ++i)
	{
		if (_wcsnicmp(action->action[i].c_str(), L"WAIT ", 5) == 0)
		{
			__int64 timeout = _wtoi64(action->action[i].substr(5).c_str());

			if (timeout > 0)
			{
				std::unique_lock<std::mutex> lock(action->mutex);
				if (action->signal.wait_for(lock, std::chrono::milliseconds(timeout), [&](){ return action->interrupt == true; }))
				{
					// Skin was refreshed or closed, or stopped via command
					action->isRunning = false;
					lock.unlock();
					action->cleanUp.notify_all();
					return;
				}
				else
				{
					// Wait is done, so continue to next set of commands
					continue;
				}
			}
		}

		SendNotifyMessage(action->rainmeterWindow, WM_RAINMETER_EXECUTE, (WPARAM)action->skin, (LPARAM)action->action[i].c_str());
	}

	action->isRunning = false;
}
