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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <windows.h>
#include <stdio.h>
#include "../../Library/Export.h"	// Rainmeter's exported functions


#pragma pack(push, 1)
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
#pragma pack(pop)

enum SensorType
{
	TYPE_TEMP,
	TYPE_FAN,
	TYPE_VOLT
};

enum ScaleType
{
	SCALE_SOURCE,
	SCALE_CENTIGRADE,
	SCALE_FARENHEIT,
	SCALE_KELVIN
};

struct MeasureData
{
	SensorType type;
	ScaleType scale;
	UINT number;

	MeasureData() : type(TYPE_TEMP), scale(SCALE_SOURCE), number() {}
};

void ReadSharedData(SensorType type, ScaleType scale, UINT number, double* value);

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData;
	*data = measure;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;

	LPCWSTR type =  RmReadString(rm, L"SpeedFanType", L"TEMPERATURE");
	if (_wcsicmp(L"TEMPERATURE", type) == 0)
	{
		measure->type = TYPE_TEMP;

		LPCWSTR scale = RmReadString(rm, L"SpeedFanScale", L"C");
		if (_wcsicmp(L"C", scale) == 0)
		{
			measure->scale = SCALE_CENTIGRADE;
		}
		else if (_wcsicmp(L"F", scale) == 0)
		{
			measure->scale = SCALE_FARENHEIT;
		}
		else if (_wcsicmp(L"K", scale) == 0)
		{
			measure->scale = SCALE_KELVIN;
		}
		else
		{
			WCHAR buffer[256];
			_snwprintf_s(buffer, _TRUNCATE, L"SpeedFanPlugin.dll: SpeedFanScale=%s is not valid in [%s]", scale, RmGetMeasureName(rm));
			RmLog(LOG_ERROR, buffer);
		}
	}
	else if (_wcsicmp(L"FAN", type) == 0)
	{
		measure->type = TYPE_FAN;
	}
	else if (_wcsicmp(L"VOLTAGE", type) == 0)
	{
		measure->type = TYPE_VOLT;
	}
	else
	{
		WCHAR buffer[256];
		_snwprintf_s(buffer, _TRUNCATE, L"SpeedFanPlugin.dll: SpeedFanType=%s is not valid in [%s]", type, RmGetMeasureName(rm));
		RmLog(LOG_ERROR, buffer);
	}

	measure->number = RmReadInt(rm, L"SpeedFanNumber", 0);
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	double value = 0.0;

	ReadSharedData(measure->type, measure->scale, measure->number, &value);

	return value;
}

PLUGIN_EXPORT void Finalize(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	delete measure;
}

/*
  Get the data from shared memory.
*/
void ReadSharedData(SensorType type, ScaleType scale, UINT number, double* value)
{
	HANDLE hData = OpenFileMapping(FILE_MAP_READ, FALSE, L"SFSharedMemory_ALM");
	if (hData == nullptr) return;

	SpeedFanData* ptr = (SpeedFanData*)MapViewOfFile(hData, FILE_MAP_READ, 0, 0, 0);
	if (ptr == 0)
	{
		CloseHandle(hData);
		return;
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
		RmLog(LOG_ERROR, L"SpeedFanPlugin.dll: Incorrect shared memory version");
	}

	UnmapViewOfFile(ptr);
	CloseHandle(hData);
}
