// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "MeasureProcess.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "System.h"
#include <TlHelp32.h>

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
	m_Value = System::IsProcessRunningCached(m_ProcessNameLowercase) ? 1.0 : -1.0;
}
