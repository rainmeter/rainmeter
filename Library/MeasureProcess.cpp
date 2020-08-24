/* Copyright (C) 2020 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureProcess.h"
#include "ConfigParser.h"
#include "Logger.h"
#include <chrono>
#include <TlHelp32.h>

std::vector<std::wstring> MeasureProcess::c_Processes;
std::thread MeasureProcess::c_ProcessThread;
std::promise<void> MeasureProcess::c_ProcessExitSignal;
std::future<void> MeasureProcess::c_ProcessFuture;
std::mutex MeasureProcess::c_ProcessMutex;
UINT MeasureProcess::c_References = 0;
int MeasureProcess::c_UpdateInterval = 250;  // milliseconds

MeasureProcess::MeasureProcess(Skin* skin, const WCHAR* name) : Measure(skin, name)
{
	if (c_References == 0)
	{
		std::lock_guard<std::mutex> lock(c_ProcessMutex);
		c_ProcessExitSignal = std::promise<void>();
		c_ProcessFuture = c_ProcessExitSignal.get_future();
		std::thread th(MeasureProcess::MonitorProcesses);
		c_ProcessThread = std::move(th);
		std::this_thread::sleep_for(std::chrono::milliseconds(25));  // Let the thread get an initial list of processes
	}

	++c_References;
}

MeasureProcess::~MeasureProcess()
{
	--c_References;
	if (c_References == 0)
	{
		std::lock_guard<std::mutex> lock(c_ProcessMutex);
		c_ProcessExitSignal.set_value();
		if (c_ProcessThread.joinable())
		{
			c_ProcessThread.join();
		}

		c_Processes.clear();
	}
}

void MeasureProcess::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	m_ProcessName = parser.ReadString(section, L"ProcessName", L"");
}

void MeasureProcess::UpdateValue()
{
	std::vector<std::wstring> processes;
	{
		std::lock_guard<std::mutex> lock(c_ProcessMutex);
		processes = c_Processes;
	}

	for (const auto& name : processes)
	{
		if (_wcsicmp(name.c_str(), m_ProcessName.c_str()) == 0)
		{
			m_Value = 1.0;
			return;
		}
	}

	m_Value = -1.0;
}

void MeasureProcess::MonitorProcesses()
{
	while (c_ProcessFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
	{
		{
			std::lock_guard<std::mutex> lock(c_ProcessMutex);

			c_Processes.clear();

			HANDLE thSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (thSnapshot != INVALID_HANDLE_VALUE)
			{
				PROCESSENTRY32 processEntry = { sizeof(processEntry) };
				if (Process32First(thSnapshot, &processEntry))
				{
					do
					{
						c_Processes.emplace_back(processEntry.szExeFile);
					}
					while (Process32Next(thSnapshot, &processEntry));
				}
				CloseHandle(thSnapshot);
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(c_UpdateInterval));
	}
}
