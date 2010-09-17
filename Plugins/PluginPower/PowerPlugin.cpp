/*
  Copyright (C) 2002 Kimmo Pekkola

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#pragma warning(disable: 4786)
#pragma warning(disable: 4996)

#include <windows.h>
#include <math.h>
#include <map>
#include <string>
#include <time.h>
#include <Powrprof.h>
#include "..\..\Library\Export.h"	// Rainmeter's exported functions

typedef struct _PROCESSOR_POWER_INFORMATION 
{  
    ULONG Number;  
    ULONG MaxMhz;  
    ULONG CurrentMhz;  
    ULONG MhzLimit;  
    ULONG MaxIdleState;  
    ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

typedef LONG (WINAPI *FPCALLNTPOWERINFORMATION)(POWER_INFORMATION_LEVEL, PVOID, ULONG, PVOID, ULONG);

/* The exported functions */
extern "C"
{
__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
__declspec( dllexport ) LPCTSTR GetString(UINT id, UINT flags);
__declspec( dllexport ) UINT Update(UINT id);
__declspec( dllexport ) UINT GetPluginVersion();
__declspec( dllexport ) LPCTSTR GetPluginAuthor();
}

enum POWER_STATE
{
	POWER_UNKNOWN,
	POWER_ACLINE,
	POWER_STATUS,
	POWER_STATUS2,
	POWER_LIFETIME,
	POWER_PERCENT,
	POWER_MHZ
};

std::map<UINT, POWER_STATE> g_States;
std::map<UINT, std::wstring> g_Formats;
HINSTANCE hDLL = NULL;
int g_Instances = 0;
FPCALLNTPOWERINFORMATION fpCallNtPowerInformation = NULL;

/*
  This function is called when the measure is initialized.
  The function must return the maximum value that can be measured. 
  The return value can also be 0, which means that Rainmeter will
  track the maximum value automatically. The parameters for this
  function are:

  instance  The instance of this DLL
  iniFile   The name of the ini-file (usually Rainmeter.ini)
  section   The name of the section in the ini-file for this measure
  id        The identifier for the measure. This is used to identify the measures that use the same plugin.
*/
UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id)
{
	g_Instances++;
	if (hDLL == NULL)
    {
        hDLL = LoadLibrary(L"powrprof.dll");
        if (hDLL)
        {
            fpCallNtPowerInformation = (FPCALLNTPOWERINFORMATION)GetProcAddress(hDLL, "CallNtPowerInformation");
        }
    }

    POWER_STATE powerState = POWER_UNKNOWN;

	/* Read our own settings from the ini-file */
	LPCTSTR type = ReadConfigString(section, L"PowerState", L"");
	if(type) 
	{
		if (_wcsicmp(L"ACLINE", type) == 0)
		{
			powerState = POWER_ACLINE;
		} 
		else if (_wcsicmp(L"STATUS", type) == 0)
		{
			powerState = POWER_STATUS;
		} 
		else if (_wcsicmp(L"STATUS2", type) == 0)
		{
			powerState = POWER_STATUS2;
		} 
		else if (_wcsicmp(L"LIFETIME", type) == 0)
		{
			powerState= POWER_LIFETIME;

			LPCTSTR format = ReadConfigString(section, L"Format", L"%H:%M");
			if (format)
			{
				g_Formats[id] = format;
			}
		} 
		else if (_wcsicmp(L"MHZ", type) == 0)
        {
			powerState= POWER_MHZ;
        }
		else if (_wcsicmp(L"PERCENT", type) == 0)
		{
			powerState = POWER_PERCENT;
		}

		g_States[id] = powerState;
	}

	switch(powerState)
	{
	case POWER_ACLINE:
		return 1;

	case POWER_STATUS:
		return 4;

	case POWER_STATUS2:
		return 255;

	case POWER_LIFETIME:
		{
			SYSTEM_POWER_STATUS status;
			if (GetSystemPowerStatus(&status))
			{
				return status.BatteryFullLifeTime;
			}
		}
		break;

	case POWER_PERCENT:
		return 100;
	}

	return 0;
}

/*
  This function is called when new value should be measured.
  The function returns the new value.
*/
UINT Update(UINT id)
{
	SYSTEM_POWER_STATUS status;
	if (GetSystemPowerStatus(&status))
	{
		std::map<UINT, POWER_STATE>::iterator i = g_States.find(id);
		if (i != g_States.end())
		{
			switch((*i).second)
			{
			case POWER_ACLINE:
				return status.ACLineStatus == 1 ? 1 : 0;

			case POWER_STATUS:
				if (status.BatteryFlag & 128)
				{
					return 0;	// No battery
				}
				else if (status.BatteryFlag & 8)
				{
					return 1;	// Charging
				}
				else if (status.BatteryFlag & 4)
				{
					return 2;	// Critical
				}
				else if (status.BatteryFlag & 2)
				{
					return 3;	// Low
				}
				else if (status.BatteryFlag & 1)
				{
					return 4;	// High
				}
				break;

			case POWER_STATUS2:
				return status.BatteryFlag;

			case POWER_LIFETIME:
				return status.BatteryLifeTime;

			case POWER_PERCENT:
				return status.BatteryLifePercent > 100 ? 100 : status.BatteryLifePercent;

			case POWER_MHZ:
                if (fpCallNtPowerInformation)
                {
                    PROCESSOR_POWER_INFORMATION ppi[8];     // Assume that 8 processors are enough
                    memset(ppi, 0, sizeof(PROCESSOR_POWER_INFORMATION) * 8);
                    fpCallNtPowerInformation(ProcessorInformation, NULL, 0, ppi, sizeof(PROCESSOR_POWER_INFORMATION) * 8);
                    return ppi[0].CurrentMhz;
                }
			}
		}
	}

	return 0;
}
/*
  This function is called when the value should be
  returned as a string.
*/
LPCTSTR GetString(UINT id, UINT flags) 
{
	static WCHAR buffer[256];

	std::map<UINT, POWER_STATE>::iterator i = g_States.find(id);
	if (i != g_States.end())
	{
		if ((*i).second == POWER_LIFETIME)
		{
			SYSTEM_POWER_STATUS status;
			if (GetSystemPowerStatus(&status))
			{
				// Change it to time string
				if (status.BatteryLifeTime == -1)
				{
					return L"Unknown";
				}
				else
				{
					std::map<UINT, std::wstring>::iterator iter = g_Formats.find(id);
					if (iter != g_Formats.end())
					{
						tm time;
						memset(&time, 0, sizeof(tm));
						time.tm_sec = status.BatteryLifeTime % 60;
						time.tm_min = (status.BatteryLifeTime / 60) % 60;
						time.tm_hour = status.BatteryLifeTime / 60 / 60;

						wcsftime(buffer, 256, (*iter).second.c_str(), &time);
					}
					else
					{
						wsprintf(buffer, L"%i:%02i:%02i", status.BatteryLifeTime / 60 / 60, (status.BatteryLifeTime / 60) % 60, status.BatteryLifeTime % 60);
					}
					return buffer;
				}
			}
		}
	}
	return NULL;
}

/*
  If the measure needs to free resources before quitting.
  The plugin can export Finalize function, which is called
  when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	std::map<UINT, POWER_STATE>::iterator i = g_States.find(id);
	if (i != g_States.end())
	{
		g_States.erase(i);
	}

	std::map<UINT, std::wstring>::iterator i2 = g_Formats.find(id);
	if (i2 != g_Formats.end())
	{
		g_Formats.erase(i2);
	}

	g_Instances--;
    if (hDLL != NULL && g_Instances == 0)
    {
        FreeLibrary(hDLL);
        hDLL = NULL;
		fpCallNtPowerInformation = NULL;
    }
}

UINT GetPluginVersion()
{
	return 1003;
}

LPCTSTR GetPluginAuthor()
{
	return L"Rainy (rainy@iki.fi)";
}