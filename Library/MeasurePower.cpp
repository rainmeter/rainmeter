/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasurePower.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "Util.h"

#include <Powrprof.h>

typedef struct _PROCESSOR_POWER_INFORMATION
{
	ULONG Number;
	ULONG MaxMhz;
	ULONG CurrentMhz;
	ULONG MhzLimit;
	ULONG MaxIdleState;
	ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

constexpr LONG NT_STATUS_SUCCESS = 0x00000000L;

UINT MeasurePower::c_NumOfProcessors = 0U;

MeasurePower::MeasurePower(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_State(PowerState::UNKNOWN),
	m_SuppressError(false),
	m_HasBeenUpdated(false),
	m_CachedBatteryLifeTime(0UL),
	m_StringValue()
{
	if (!c_NumOfProcessors)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		c_NumOfProcessors = (UINT)si.dwNumberOfProcessors;
	}
}

MeasurePower::~MeasurePower()
{
}

void MeasurePower::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	PowerState oldState = m_State;
	std::wstring oldFormat = m_Format;

	std::wstring value = parser.ReadString(section, L"PowerState", L"");
	LPCWSTR state = value.c_str();
	if (_wcsicmp(L"ACLINE", state) == 0)
	{
		m_State = PowerState::ACLINE;
		m_MaxValue = 1.0;
	}
	else if (_wcsicmp(L"STATUS", state) == 0)
	{
		m_State = PowerState::STATUS;
		m_MaxValue = 4.0;
	}
	else if (_wcsicmp(L"STATUS2", state) == 0)
	{
		m_State = PowerState::STATUS2;
		m_MaxValue = 255.0;
	}
	else if (_wcsicmp(L"LIFETIME", state) == 0)
	{
		m_State = PowerState::LIFETIME;
		m_Format = parser.ReadString(section, L"Format", L"%H:%M");

		SYSTEM_POWER_STATUS sps;
		if (GetSystemPowerStatus(&sps))
		{
			m_MaxValue = sps.BatteryFullLifeTime;
		}
	}
	else if (_wcsicmp(L"MHZ", state) == 0)
	{
		m_State = PowerState::MHZ;
	}
	else if (_wcsicmp(L"HZ", state) == 0)
	{
		m_State = PowerState::HZ;
	}
	else if (_wcsicmp(L"PERCENT", state) == 0)
	{
		m_State = PowerState::PERCENT;
		m_MaxValue = 100.0;
	}
	else
	{
		m_State = PowerState::UNKNOWN;
	}

	if (m_HasBeenUpdated)
	{
		m_SuppressError = oldState == m_State && _wcsicmp(oldFormat.c_str(), m_Format.c_str()) == 0;
	}
}

void MeasurePower::UpdateValue()
{
	m_HasBeenUpdated = true;

	switch (m_State)
	{
	case PowerState::HZ:
	case PowerState::MHZ:
		if (c_NumOfProcessors > 0U)
		{
			std::vector<PROCESSOR_POWER_INFORMATION> ppi(c_NumOfProcessors);
			LONG status = CallNtPowerInformation(
				ProcessorInformation, nullptr, 0, ppi.data(), sizeof(PROCESSOR_POWER_INFORMATION) * c_NumOfProcessors);
			if (status == NT_STATUS_SUCCESS)
			{
				m_Value = (m_State == PowerState::MHZ) ? ppi[0].CurrentMhz : ppi[0].CurrentMhz * 1000000.0;
			}
			else
			{
				m_Value = 0.0;
				LogProcessorPowerError(status);
			}
			return;
		}
		break;
	}

	SYSTEM_POWER_STATUS sps;
	if (GetSystemPowerStatus(&sps))
	{
		switch (m_State)
		{
		case PowerState::ACLINE:
			m_Value = sps.ACLineStatus == 1 ? 1.0 : 0.0;
			return;

		case PowerState::STATUS:
			if (sps.BatteryFlag & 128)
			{
				m_Value = 0.0;	// No battery
			}
			else if (sps.BatteryFlag & 8)
			{
				m_Value = 1.0;	// Charging
			}
			else if (sps.BatteryFlag & 4)
			{
				m_Value = 2.0;	// Critical
			}
			else if (sps.BatteryFlag & 2)
			{
				m_Value = 3.0;	// Low
			}
			else if (sps.BatteryFlag == 0 || sps.BatteryFlag & 1)
			{
				m_Value = 4.0;	// Medium/High
			}
			else
			{
				m_Value = 0.0;
			}
			return;

		case PowerState::STATUS2:
			m_Value = sps.BatteryFlag;
			return;

		case PowerState::LIFETIME:
			m_CachedBatteryLifeTime = sps.BatteryLifeTime;
			m_Value = sps.BatteryLifeTime;
			return;

		case PowerState::PERCENT:
			m_Value = sps.BatteryLifePercent == 255 ? 100.0 : sps.BatteryLifePercent;
			return;
		}
	}
	else
	{
		LogPowerStatusError();
	}

	m_Value = 0.0;
}

const WCHAR* MeasurePower::GetStringValue()
{
	if (m_State == PowerState::LIFETIME)
	{
		DWORD value = m_CachedBatteryLifeTime;
		if (value == -1)
		{
			return CheckSubstitute(L"Unknown");
		}

		tm time = { 0 };
		time.tm_sec = value % 60;
		time.tm_min = (value / 60) % 60;
		time.tm_hour = value / 60 / 60;

		_invalid_parameter_handler oldHandler = _set_thread_local_invalid_parameter_handler(RmNullCRTInvalidParameterHandler);
		_CrtSetReportMode(_CRT_ASSERT, 0);

		errno = 0;
		wcsftime(m_StringValue, _countof(m_StringValue), m_Format.c_str(), &time);
		if (errno == EINVAL)
		{
			m_StringValue[0] = L'\0';
		}

		_set_thread_local_invalid_parameter_handler(oldHandler);

		return CheckSubstitute(m_StringValue);
	}

	return nullptr;
}

void MeasurePower::LogProcessorPowerError(LONG status)
{
	if (!m_SuppressError)
	{
		// NTSTATUS codes:
		// https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-erref/596a1078-e883-4972-9bbc-49e60bebca55
		LogErrorF(this, L"Processor power status error: 0x%08x", status);
		m_SuppressError = true;
	}
}

void MeasurePower::LogPowerStatusError()
{
	if (!m_SuppressError)
	{
		LogErrorF(this, L"Power status error: %ld", GetLastError());
		m_SuppressError = true;
	}
}
