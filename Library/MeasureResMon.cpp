// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

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

void MeasureResMon::ReadOptions(ConfigParser& parser, std::wstring_view section)
{
	Measure::ReadOptions(parser, section);

	static constexpr ConfigParser::EnumOption<Type> s_Types[] =
	{
		{ L"GDI", Type::GDI },
		{ L"USER", Type::USER },
		{ L"HANDLE", Type::HANDLE },
		{ L"WINDOW", Type::WINDOW },
	};
	m_Type = parser.ReadEnum(section, L"ResCountType", Type::GDI, s_Types);

	parser.ReadString(m_ProcessName, section, L"ProcessName", L"");
}

void MeasureResMon::UpdateValue()
{
	if (m_Type == Type::WINDOW)
	{
		UINT windowCount = 0;
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
	DWORD bytesNeeded = 0;
	WCHAR buffer[1024] = { 0 };
	HMODULE module[1024] = { 0 };
	DWORD moduleBytesNeeded = 0;

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

	UINT resourceCount = 0;
	for (UINT i = 0, isize = bytesNeeded / sizeof(DWORD); i < isize; ++i)
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
			DWORD handleCount = 0;
			GetProcessHandleCount(process, &handleCount);
			resourceCount += handleCount;
		}

		CloseHandle(process);
	}

	m_Value = (double)resourceCount;
}
