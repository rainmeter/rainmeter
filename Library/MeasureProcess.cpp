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
#include "System.h"
#include <TlHelp32.h>

std::unordered_set<std::wstring>& GetRunningProcessLowercase() {
	static std::unordered_set<std::wstring> s_Processes;
	static ULONGLONG s_LastUpdateTickCount = 0;
	const ULONGLONG updateInterval = 250; // ms

	ULONGLONG tickCount = System::GetTickCount64();
	if (tickCount >= (s_LastUpdateTickCount + updateInterval))
	{
		s_LastUpdateTickCount = tickCount;

		s_Processes = {};
		HANDLE thSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (thSnapshot != INVALID_HANDLE_VALUE)
		{
			PROCESSENTRY32 processEntry = { sizeof(processEntry) };
			if (Process32First(thSnapshot, &processEntry))
			{
				do
				{
					std::wstring name = processEntry.szExeFile;
					StringUtil::ToLowerCase(name);
					s_Processes.insert(name);
				} while (Process32Next(thSnapshot, &processEntry));
			}
			CloseHandle(thSnapshot);
		}
	}

	return s_Processes;
}

MeasureProcess::MeasureProcess(Skin* skin, const WCHAR* name) : Measure(skin, name)
{
}

MeasureProcess::~MeasureProcess()
{
}

void MeasureProcess::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	m_ProcessNameLowercase = parser.ReadString(section, L"ProcessName", L"");
	StringUtil::ToLowerCase(m_ProcessNameLowercase);
}

void MeasureProcess::UpdateValue()
{
	m_Value = GetRunningProcessLowercase().count(m_ProcessNameLowercase) ? 1.0 : -1.0;
}
