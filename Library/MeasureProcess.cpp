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
