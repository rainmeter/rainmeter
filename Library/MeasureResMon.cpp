/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureResMon.h"
#include "ConfigParser.h"
#include "Logger.h"
#include <psapi.h>

MeasureResMon::MeasureResMon(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Type(Type::GDI),
	m_ProcessName()
{
}

MeasureResMon::~MeasureResMon()
{
}

void MeasureResMon::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	const WCHAR* type = parser.ReadString(section, L"ResCountType", L"GDI").c_str();
	if (_wcsicmp(L"GDI", type) == 0)
	{
		m_Type = Type::GDI;
	}
	else if (_wcsicmp(L"USER", type) == 0)
	{
		m_Type = Type::USER;
	}
	else if (_wcsicmp(L"HANDLE", type) == 0)
	{
		m_Type = Type::HANDLE;
	}
	else if (_wcsicmp(L"WINDOW", type) == 0)
	{
		m_Type = Type::WINDOW;
	}
	else
	{
		LogErrorF(this, L"ResMon: ResCountType=%s is not valid", type);
	}

	m_ProcessName = parser.ReadString(section, L"ProcessName", L"");
}

void MeasureResMon::UpdateValue()
{
	if (m_Type == Type::WINDOW)
	{
		UINT windowCount = 0U;
		EnumChildWindows(nullptr, [](HWND, LPARAM lParam) -> BOOL
		{
			UINT* count = (UINT*)lParam;
			++(*count);
			return TRUE;
		}, (LPARAM)&windowCount);
		m_Value = (double)windowCount;
		return;
	}

	const WCHAR* processName = m_ProcessName.c_str();
	const bool hasProcessName = !m_ProcessName.empty();

	DWORD processes[1024] = { 0 };
	DWORD bytesNeeded = 0UL;
	WCHAR buffer[1024] = { 0 };
	HMODULE module[1024] = { 0 };
	DWORD moduleBytesNeeded = 0UL;

	if (!EnumProcesses(processes, sizeof(processes), &bytesNeeded))
	{
		m_Value = 0.0;
		return;
	}

	DWORD flags = PROCESS_QUERY_INFORMATION;
	if (hasProcessName)
	{
		flags |= PROCESS_VM_READ;
	}

	UINT resourceCount = 0U;
	for (UINT i = 0U, isize = bytesNeeded / sizeof(DWORD); i < isize; ++i)
	{
		HANDLE process = OpenProcess(flags, TRUE, processes[i]);
		if (!process)
		{
			continue;
		}

		if (hasProcessName)
		{
			if (!EnumProcessModules(process, module, sizeof(module), &moduleBytesNeeded) ||
				!GetModuleBaseName(process, module[0], buffer, _countof(buffer)) ||
				_wcsicmp(buffer, processName) != 0)
			{
				CloseHandle(process);
				continue;
			}
		}

		if (m_Type == Type::GDI)
		{
			resourceCount += GetGuiResources(process, GR_GDIOBJECTS);
		}
		else if (m_Type == Type::USER)
		{
			resourceCount += GetGuiResources(process, GR_USEROBJECTS);
		}
		else if (m_Type == Type::HANDLE)
		{
			DWORD handleCount = 0UL;
			GetProcessHandleCount(process, &handleCount);
			resourceCount += handleCount;
		}

		CloseHandle(process);
	}

	m_Value = (double)resourceCount;
}
