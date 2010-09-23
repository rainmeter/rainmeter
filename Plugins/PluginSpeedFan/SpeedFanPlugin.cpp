/*
  Copyright (C) 2004 Kimmo Pekkola

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
#include <string>
#include <map>
#include "..\..\Library\Export.h"	// Rainmeter's exported functions

/* The exported functions */
extern "C"
{
__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
__declspec( dllexport ) double Update2(UINT id);
__declspec( dllexport ) UINT GetPluginVersion();
__declspec( dllexport ) LPCTSTR GetPluginAuthor();
}

#pragma pack(1)

struct SpeedFanData 
{
	WORD version;
	WORD flags;
	INT MemSize;
	DWORD handle;
	WORD NumTemps;
	WORD NumFans;
	WORD NumVolts;
	INT temps[32];
	INT fans[32];
	INT volts[32];
};

enum SensorType 
{
	TYPE_TEMP,
	TYPE_FAN,
	TYPE_VOLT
};

enum TempScale
{
	SCALE_SOURCE,
	SCALE_CENTIGRADE,
	SCALE_FARENHEIT,
	SCALE_KELVIN
};

bool ReadSharedData(SensorType type, TempScale scale, UINT number, double* value);

static std::map<UINT, SensorType> g_Types;
static std::map<UINT, TempScale> g_Scales;
static std::map<UINT, UINT> g_Numbers;

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
	/* Read our own settings from the ini-file */
	LPCTSTR type = ReadConfigString(section, L"SpeedFanType", L"TEMPERATURE");
	if (type)
	{
		if (_wcsicmp(L"TEMPERATURE", type) == 0)
		{
			g_Types[id] = TYPE_TEMP;

			LPCTSTR scale = ReadConfigString(section, L"SpeedFanScale", L"C");
			if (scale)
			{
				if (_wcsicmp(L"C", scale) == 0)
				{
					g_Scales[id] = SCALE_CENTIGRADE;
				} 
				else if (_wcsicmp(L"F", scale) == 0)
				{
					g_Scales[id] = SCALE_FARENHEIT;
				} 
				else if (_wcsicmp(L"K", scale) == 0)
				{
					g_Scales[id] = SCALE_KELVIN;
				} 
				else
				{
					std::wstring error = L"SpeedFanScale=";
					error += scale;
					error += L" is not valid in measure [";
					error += section;
					error += L"].";
					MessageBox(NULL, error.c_str(), L"Rainmeter", MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
				}
			}
		} 
		else if (_wcsicmp(L"FAN", type) == 0)
		{
			g_Types[id] = TYPE_FAN;
		} 
		else if (_wcsicmp(L"VOLTAGE", type) == 0)
		{
			g_Types[id] = TYPE_VOLT;
		} 
		else
		{
			std::wstring error = L"SpeedFanType=";
			error += type;
			error += L" is not valid in measure [";
			error += section;
			error += L"].";
			MessageBox(NULL, error.c_str(), L"Rainmeter", MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
		}
	}
	
	LPCTSTR data = ReadConfigString(section, L"SpeedFanNumber", L"0");
	if (data)
	{
		g_Numbers[id] = _wtoi(data);
	}

	return 0;
}

/*
This function is called when new value should be measured.
The function returns the new value.
*/
double Update2(UINT id)
{
	double value = 0.0; 
	
	std::map<UINT, SensorType>::const_iterator type = g_Types.find(id);
	std::map<UINT, TempScale>::const_iterator scale = g_Scales.find(id);
	std::map<UINT, UINT>::const_iterator number = g_Numbers.find(id);
	
	if(type == g_Types.end() || number == g_Numbers.end())
	{
		return 0.0;		// No id in the map. How this can be ????
	}

	if ((*type).second == TYPE_TEMP)
	{
		if (scale != g_Scales.end() &&
			ReadSharedData((*type).second, (*scale).second, (*number).second, &value))
		{
			return value;
		}
	}
	else
	{
		if (ReadSharedData((*type).second, SCALE_SOURCE, (*number).second, &value))
		{
			return value;
		}
	}
	
	return 0.0;
}

/*
  If the measure needs to free resources before quitting.
  The plugin can export Finalize function, which is called
  when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	std::map<UINT, SensorType>::iterator i1 = g_Types.find(id);
	if (i1 != g_Types.end())
	{
		g_Types.erase(i1);
	}

	std::map<UINT, UINT>::iterator i2 = g_Numbers.find(id);
	if (i2 != g_Numbers.end())
	{
		g_Numbers.erase(i2);
	}

	std::map<UINT, TempScale>::iterator i3 = g_Scales.find(id);
	if (i3 != g_Scales.end())
	{
		g_Scales.erase(i3);
	}
}

/*
  Get the data from shared memory.
*/
bool ReadSharedData(SensorType type, TempScale scale, UINT number, double* value)
{
	SpeedFanData* ptr;
	HANDLE hData;
	
	hData = OpenFileMapping(FILE_MAP_READ, FALSE, L"SFSharedMemory_ALM");
	if (hData == NULL) return false;
	
	ptr = (SpeedFanData*)MapViewOfFile(hData, FILE_MAP_READ, 0, 0, 0);
	if (ptr == 0)
	{
		CloseHandle(hData);
		return false;
	}

	if (ptr->version == 1)
	{
		switch(type) 
		{
		case TYPE_TEMP:
			if (number < ptr->NumTemps)
			{
				*value = ptr->temps[number];
				*value /= 100.0;

				if (scale == SCALE_FARENHEIT)
				{
					*value *= 1.8;
					*value += 32.0;
				}
				else if (scale == SCALE_KELVIN)
				{
					*value += 273.15;
				}
			}
			break;

		case TYPE_FAN:
			if (number < ptr->NumTemps)
			{
				*value = ptr->fans[number];
			}
			break;

		case TYPE_VOLT:
			if (number < ptr->NumVolts)
			{
				*value = ptr->volts[number];
				*value /= 100.0;
			}
			break;
		}
	}
	else
	{
		LSLog(LOG_DEBUG, L"Rainmeter", L"SpeedFanPlugin: The shared memory has incorrect version.");
	}

	UnmapViewOfFile(ptr);
	CloseHandle(hData);
	
	return true;
}

UINT GetPluginVersion()
{
	return 1002;
}

LPCTSTR GetPluginAuthor()
{
	return L"Rainy (rainy@iki.fi)";
}