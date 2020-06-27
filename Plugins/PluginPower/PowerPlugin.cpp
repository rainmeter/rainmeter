/* Copyright (C) 2002 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include <windows.h>
#include <Powrprof.h>
#include <time.h>
#include <errno.h>
#include <crtdbg.h>
#include <stdlib.h>
#include "../../Common/RawString.h"
#include "../../Library/Export.h"	// Rainmeter's exported functions

typedef struct _PROCESSOR_POWER_INFORMATION
{
	ULONG Number;
	ULONG MaxMhz;
	ULONG CurrentMhz;
	ULONG MhzLimit;
	ULONG MaxIdleState;
	ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

enum MeasureType
{
	POWER_UNKNOWN,
	POWER_ACLINE,
	POWER_STATUS,
	POWER_STATUS2,
	POWER_LIFETIME,
	POWER_PERCENT,
	POWER_MHZ,
	POWER_HZ
};

struct MeasureData
{
	MeasureType type;
	RawString format;

	bool suppressError;
	bool updated;

	DWORD cachedBatteryLifeTime;

	void* rm;
	
	MeasureData() : type(POWER_UNKNOWN), suppressError(false), updated(false), cachedBatteryLifeTime(0UL), rm(nullptr) {}
};

UINT g_NumOfProcessors = 0U;
constexpr LONG NT_STATUS_SUCCESS = 0x00000000L;

void NullCRTInvalidParameterHandler(const wchar_t* expression, const wchar_t* function,  const wchar_t* file, unsigned int line, uintptr_t pReserved)
{
	// Do nothing.
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData;
	*data = measure;

	if (!g_NumOfProcessors)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		g_NumOfProcessors = (UINT)si.dwNumberOfProcessors;
	}

	measure->rm = rm;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;

	MeasureType oldType = measure->type;
	RawString oldFormat = measure->format;

	LPCWSTR value = RmReadString(rm, L"PowerState", L"");
	if (_wcsicmp(L"ACLINE", value) == 0)
	{
		measure->type = POWER_ACLINE;
		*maxValue = 1.0;
	}
	else if (_wcsicmp(L"STATUS", value) == 0)
	{
		measure->type = POWER_STATUS;
		*maxValue = 4.0;
	}
	else if (_wcsicmp(L"STATUS2", value) == 0)
	{
		measure->type = POWER_STATUS2;
		*maxValue = 255.0;
	}
	else if (_wcsicmp(L"LIFETIME", value) == 0)
	{
		measure->type= POWER_LIFETIME;

		value = RmReadString(rm, L"Format", L"%H:%M");
		measure->format = value;

		SYSTEM_POWER_STATUS sps;
		if (GetSystemPowerStatus(&sps))
		{
			*maxValue = sps.BatteryFullLifeTime;
		}
	}
	else if (_wcsicmp(L"MHZ", value) == 0)
	{
		measure->type = POWER_MHZ;
	}
	else if (_wcsicmp(L"HZ", value) == 0)
	{
		measure->type = POWER_HZ;
	}
	else if (_wcsicmp(L"PERCENT", value) == 0)
	{
		measure->type = POWER_PERCENT;
		*maxValue = 100.0;
	}

	if (measure->updated)
	{
		measure->suppressError =
			oldType == measure->type &&
			_wcsicmp(oldFormat.c_str(), measure->format.c_str()) == 0;
	}
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;

	measure->updated = true;

	switch (measure->type)
	{
	case POWER_HZ:
	case POWER_MHZ:
		{
			if (g_NumOfProcessors > 0U)
			{
				double value = 0.0;
				PROCESSOR_POWER_INFORMATION* ppi = new PROCESSOR_POWER_INFORMATION[g_NumOfProcessors];
				memset(ppi, 0, sizeof(PROCESSOR_POWER_INFORMATION) * g_NumOfProcessors);
				LONG status = CallNtPowerInformation(
					ProcessorInformation, nullptr, 0, ppi, sizeof(PROCESSOR_POWER_INFORMATION) * g_NumOfProcessors);
				if (status == NT_STATUS_SUCCESS)
				{
					value = (measure->type == POWER_MHZ) ? ppi[0].CurrentMhz : ppi[0].CurrentMhz * 1000000.0;
				}
				else if (!measure->suppressError)
				{
					// NTSTATUS codes:
					// https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-erref/596a1078-e883-4972-9bbc-49e60bebca55
					RmLogF(measure->rm, LOG_ERROR, L"Processor power status error: 0x%08x", status);
					measure->suppressError = true;
				}
				delete[] ppi;
				return value;
			}
		}
		break;
	}

	SYSTEM_POWER_STATUS sps;
	if (GetSystemPowerStatus(&sps))
	{
		switch (measure->type)
		{
		case POWER_ACLINE:
			return sps.ACLineStatus == 1 ? 1.0 : 0.0;

		case POWER_STATUS:
			if (sps.BatteryFlag & 128)
			{
				return 0.0;	// No battery
			}
			else if (sps.BatteryFlag & 8)
			{
				return 1.0;	// Charging
			}
			else if (sps.BatteryFlag & 4)
			{
				return 2.0;	// Critical
			}
			else if (sps.BatteryFlag & 2)
			{
				return 3.0;	// Low
			}
			else if (sps.BatteryFlag == 0 || sps.BatteryFlag & 1)
			{
				return 4.0;	// Medium/High
			}
			break;

		case POWER_STATUS2:
			return sps.BatteryFlag;

		case POWER_LIFETIME:
			measure->cachedBatteryLifeTime = sps.BatteryLifeTime;
			return sps.BatteryLifeTime;

		case POWER_PERCENT:
			return sps.BatteryLifePercent == 255 ? 100.0 : sps.BatteryLifePercent;
		}
	}
	else if (!measure->suppressError)
	{
		RmLogF(measure->rm, LOG_ERROR, L"Power status error: %ld", GetLastError());
		measure->suppressError = true;
	}

	return 0.0;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	static WCHAR buffer[128];
	MeasureData* measure = (MeasureData*)data;

	if (measure->type == POWER_LIFETIME)
	{
		DWORD value = measure->cachedBatteryLifeTime;
		if (value == -1)
		{
			return L"Unknown";
		}
		else
		{
			tm time = {0};
			time.tm_sec = value % 60;
			time.tm_min = (value / 60) % 60;
			time.tm_hour = value / 60 / 60;

			_invalid_parameter_handler oldHandler = _set_invalid_parameter_handler(NullCRTInvalidParameterHandler);
			_CrtSetReportMode(_CRT_ASSERT, 0);

			errno = 0;
			wcsftime(buffer, 128, measure->format.c_str(), &time);
			if (errno == EINVAL)
			{
				buffer[0] = L'\0';
			}

			_set_invalid_parameter_handler(oldHandler);

			return buffer;
		}
	}

	return nullptr;
}

PLUGIN_EXPORT void Finalize(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	delete measure;
}
