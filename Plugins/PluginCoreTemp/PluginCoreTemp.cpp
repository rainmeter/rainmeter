/*
  Copyright (C) 2011 Arthur Liberman

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
#include <wchar.h>
#include "CoreTempProxy.h"
#include "../../Library/Export.h"	// Rainmeter's exported functions

typedef enum eMeasureType
{
	MeasureTemperature,
	MeasureMaxTemperature,
	MeasureTjMax,
	MeasureLoad,
	MeasureVid,
	MeasureCpuSpeed,
	MeasureBusSpeed,
	MeasureBusMultiplier,
	MeasureCpuName
};

struct MeasureData
{
	eMeasureType type;
	int index;

	MeasureData() : type(), index() {}
};

CoreTempProxy proxy;

eMeasureType convertStringToMeasureType(LPCWSTR i_String);
bool areStringsEqual(LPCWSTR i_String1, LPCWSTR i_Strting2);
float getHighestTemp();

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	MeasureData* measure = new MeasureData;
	*data = measure;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	MeasureData* measure = (MeasureData*)data;

	LPCWSTR value = RmReadString(rm, L"CoreTempType", L"Temperature");
	measure->type = convertStringToMeasureType(value);

	if (measure->type == MeasureTemperature || measure->type == MeasureTjMax || measure->type == MeasureLoad)
	{
		measure->index = RmReadInt(rm, L"CoreTempIndex", 0);
	}
}

PLUGIN_EXPORT double Update(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	double result = 0;

	if (proxy.GetData())
	{
		switch (measure->type)
		{
		case MeasureTemperature:
			result = proxy.GetTemp(measure->index);
			break;

		case MeasureMaxTemperature:
			result = getHighestTemp();
			break;

		case MeasureTjMax:
			result = proxy.GetTjMax(measure->index);
			break;

		case MeasureLoad:
			result = proxy.GetCoreLoad(measure->index);
			break;

		case MeasureVid:
			result = proxy.GetVID();
			break;

		case MeasureCpuSpeed:
			result = proxy.GetCPUSpeed();
			break;

		case MeasureBusSpeed:
			result = proxy.GetFSBSpeed();
			break;

		case MeasureBusMultiplier:
			result = proxy.GetMultiplier();
			break;
		}
	}

	return result;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	static WCHAR buffer[128];

	switch (measure->type)
	{
	case MeasureVid:
		_snwprintf_s(buffer, _TRUNCATE, L"%.4f", proxy.GetVID());
		break;

	case MeasureCpuName:
		_snwprintf_s(buffer, _TRUNCATE, L"%S", proxy.GetCPUName());
		break;

	default:
		return nullptr;
	}

	return buffer;
}

PLUGIN_EXPORT void Finalize(void* data)
{
	MeasureData* measure = (MeasureData*)data;
	delete measure;
}

bool areStringsEqual(LPCWSTR i_String1, LPCWSTR i_Strting2)
{
	return _wcsicmp(i_String1, i_Strting2) == 0;
}

eMeasureType convertStringToMeasureType(LPCWSTR i_String)
{
	eMeasureType result;

	if (areStringsEqual(i_String, L"Temperature"))
	{
		result = MeasureTemperature;
	}
	else if (areStringsEqual(i_String, L"MaxTemperature"))
	{
		result = MeasureMaxTemperature;
	}
	else if (areStringsEqual(i_String, L"TjMax"))
	{
		result = MeasureTjMax;
	}
	else if (areStringsEqual(i_String, L"Load"))
	{
		result = MeasureLoad;
	}
	else if (areStringsEqual(i_String, L"Vid"))
	{
		result = MeasureVid;
	}
	else if (areStringsEqual(i_String, L"CpuSpeed"))
	{
		result = MeasureCpuSpeed;
	}
	else if (areStringsEqual(i_String, L"BusSpeed"))
	{
		result = MeasureBusSpeed;
	}
	else if (areStringsEqual(i_String, L"BusMultiplier"))
	{
		result = MeasureBusMultiplier;
	}
	else if (areStringsEqual(i_String, L"CpuName"))
	{
		result = MeasureCpuName;
	}
	else
	{
		result = MeasureTemperature;
		RmLog(LOG_WARNING, L"CoreTemp.dll: Invalid CoreTempType");
	}

	return result;
}

float getHighestTemp()
{
	float temp = -255;
	UINT coreCount = proxy.GetCoreCount();

	for (UINT i = 0; i < coreCount; ++i)
	{
		if (temp < proxy.GetTemp(i))
		{
			temp = proxy.GetTemp(i);
		}
	}

	return temp;
}
