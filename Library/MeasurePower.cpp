/* Copyright (C) 2002 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasurePower.h"
#include "ConfigParser.h"
#include "Rainmeter.h"

#include <Powrprof.h>
#include <time.h>
#include <errno.h>
#include <crtdbg.h>
#include <memory>

namespace
{
	constexpr LONG NT_STATUS_SUCCESS = 0x00000000L;
	UINT g_NumOfProcessors = 0U;

	void NullCRTInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
	{
		UNREFERENCED_PARAMETER(expression);
		UNREFERENCED_PARAMETER(function);
		UNREFERENCED_PARAMETER(file);
		UNREFERENCED_PARAMETER(line);
		UNREFERENCED_PARAMETER(pReserved);
	}
}

MeasurePower::MeasurePower(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Type(POWER_UNKNOWN),
	m_SuppressError(false),
	m_Updated(false),
	m_CachedBatteryLifeTime(0UL)
{
	if (!g_NumOfProcessors)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		g_NumOfProcessors = (UINT)si.dwNumberOfProcessors;
	}
}

MeasurePower::~MeasurePower()
{
}

void MeasurePower::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	MeasureType oldType = m_Type;
	std::wstring oldFormat = m_Format;

	Measure::ReadOptions(parser, section);

	const std::wstring& value = parser.ReadString(section, L"PowerState", L"");
	if (_wcsicmp(L"ACLINE", value.c_str()) == 0)
	{
		m_Type = POWER_ACLINE;
		m_MaxValue = 1.0;
	}
	else if (_wcsicmp(L"STATUS", value.c_str()) == 0)
	{
		m_Type = POWER_STATUS;
		m_MaxValue = 4.0;
	}
	else if (_wcsicmp(L"STATUS2", value.c_str()) == 0)
	{
		m_Type = POWER_STATUS2;
		m_MaxValue = 255.0;
	}
	else if (_wcsicmp(L"LIFETIME", value.c_str()) == 0)
	{
		m_Type = POWER_LIFETIME;
		m_Format = parser.ReadString(section, L"Format", L"%H:%M");

		SYSTEM_POWER_STATUS sps;
		if (GetSystemPowerStatus(&sps))
		{
			m_MaxValue = sps.BatteryFullLifeTime;
		}
	}
	else if (_wcsicmp(L"MHZ", value.c_str()) == 0)
	{
		m_Type = POWER_MHZ;
	}
	else if (_wcsicmp(L"HZ", value.c_str()) == 0)
	{
		m_Type = POWER_HZ;
	}
	else if (_wcsicmp(L"PERCENT", value.c_str()) == 0)
	{
		m_Type = POWER_PERCENT;
		m_MaxValue = 100.0;
	}

	if (m_Updated)
	{
		m_SuppressError =
			oldType == m_Type &&
			_wcsicmp(oldFormat.c_str(), m_Format.c_str()) == 0;
	}
}

void MeasurePower::UpdateValue()
{
	m_Updated = true;

	switch (m_Type)
	{
	case POWER_HZ:
	case POWER_MHZ:
		if (g_NumOfProcessors > 0U)
		{
			double value = 0.0;
			auto ppi = std::make_unique<PROCESSOR_POWER_INFORMATION[]>(g_NumOfProcessors);
			memset(ppi.get(), 0, sizeof(PROCESSOR_POWER_INFORMATION) * g_NumOfProcessors);
			LONG status = CallNtPowerInformation(ProcessorInformation, nullptr, 0, ppi.get(), sizeof(PROCESSOR_POWER_INFORMATION) * g_NumOfProcessors);
			if (status == NT_STATUS_SUCCESS)
			{
				value = (m_Type == POWER_MHZ) ? ppi[0].CurrentMhz : ppi[0].CurrentMhz * 1000000.0;
			}
			else if (!m_SuppressError)
			{
				LogErrorF(this, L"Power: Processor power status error: 0x%08x", status);
				m_SuppressError = true;
			}
			m_Value = value;
			return;
		}
		break;
	}

	SYSTEM_POWER_STATUS sps;
	if (GetSystemPowerStatus(&sps))
	{
		switch (m_Type)
		{
		case POWER_ACLINE:
			m_Value = sps.ACLineStatus == 1 ? 1.0 : 0.0;
			return;
		case POWER_STATUS:
			if (sps.BatteryFlag & 128) m_Value = 0.0;
			else if (sps.BatteryFlag & 8) m_Value = 1.0;
			else if (sps.BatteryFlag & 4) m_Value = 2.0;
			else if (sps.BatteryFlag & 2) m_Value = 3.0;
			else if (sps.BatteryFlag == 0 || sps.BatteryFlag & 1) m_Value = 4.0;
			else m_Value = 0.0;
			return;
		case POWER_STATUS2:
			m_Value = sps.BatteryFlag;
			return;
		case POWER_LIFETIME:
			m_CachedBatteryLifeTime = sps.BatteryLifeTime;
			m_Value = sps.BatteryLifeTime;
			return;
		case POWER_PERCENT:
			m_Value = sps.BatteryLifePercent == 255 ? 100.0 : sps.BatteryLifePercent;
			return;
		}
	}
	else if (!m_SuppressError)
	{
		LogErrorF(this, L"Power: Power status error: %ld", GetLastError());
		m_SuppressError = true;
	}

	m_Value = 0.0;
}

const WCHAR* MeasurePower::GetStringValue()
{
	static WCHAR buffer[128];
	if (m_Type == POWER_LIFETIME)
	{
		DWORD value = m_CachedBatteryLifeTime;
		if (value == (DWORD)-1)
		{
			return L"Unknown";
		}

		tm time = { 0 };
		time.tm_sec = value % 60;
		time.tm_min = (value / 60) % 60;
		time.tm_hour = value / 60 / 60;

		_invalid_parameter_handler oldHandler = _set_thread_local_invalid_parameter_handler(NullCRTInvalidParameterHandler);
		_CrtSetReportMode(_CRT_ASSERT, 0);

		errno = 0;
		wcsftime(buffer, 128, m_Format.c_str(), &time);
		if (errno == EINVAL)
		{
			buffer[0] = L'\0';
		}

		_set_thread_local_invalid_parameter_handler(oldHandler);
		return CheckSubstitute(buffer);
	}

	return nullptr;
}
