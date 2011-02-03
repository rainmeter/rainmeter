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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <windows.h>
#include <tchar.h>
#include <map>

#include "CoreTempProxy.h"

#include "../../Library/Export.h"	// Rainmeter's exported functions

#include "../../Library/DisableThreadLibraryCalls.h"	// contains DllMain entry point

extern "C"
{
	__declspec( dllexport ) UINT Initialize(HMODULE instance, LPCTSTR iniFile, LPCTSTR section, UINT id);
	__declspec( dllexport ) void Finalize(HMODULE instance, UINT id);
	__declspec( dllexport ) LPCTSTR GetString(UINT id, UINT flags);
	__declspec( dllexport ) double Update2(UINT id);
	__declspec( dllexport ) UINT GetPluginVersion();
	__declspec( dllexport ) LPCTSTR GetPluginAuthor();
}

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

std::map<UINT, eMeasureType> g_Types;
std::map<UINT, int> g_Indexes;
CoreTempProxy proxy;

eMeasureType convertStringToMeasureType(LPCWSTR i_String);
bool areStringsEqual(LPCWSTR i_String1, LPCWSTR i_Strting2);
double getValues(UINT i_Id);
LPCTSTR getString(UINT i_Id);
float getHighestTemp();

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
	/* 
	  Read our own settings from the ini-file 
	  The ReadConfigString can be used for this purpose. Plugins
	  can also read the config some other way (e.g. with 
	  GetPrivateProfileInt, but in that case the variables
	  do not work.
	*/
	LPCTSTR data = ReadConfigString(section, L"CoreTempType", L"Temperature");
	if (data)
	{
		eMeasureType type = convertStringToMeasureType(data);

		g_Types[id] = type;
		if (type == MeasureTemperature || type == MeasureTjMax || type == MeasureLoad)
		{
			data = ReadConfigString(section, L"CoreTempIndex", L"0");
			if (data)
			{
				g_Indexes[id] = _wtoi(data);
			}
			else
			{
				g_Indexes[id] = 0;
				LSLog(LOG_WARNING, L"Rainmeter", L"CoreTempPlugin: Selected CoreTempType requires CoreTempIndex, assuming 0.");
			}
		}
	}
	
	return 0;
}

/*
  This function is called when new value should be measured.
  The function returns the new value.
*/
double Update2(UINT id)
{
	double result = 0;

	if (proxy.GetData())
	{
		result = getValues(id);
	}

	return result;
}

LPCTSTR GetString(UINT id, UINT flags)
{
	return getString(id);
}

/*
  If the measure needs to free resources before quitting.
  The plugin can export Finalize function, which is called
  when Rainmeter quits (or refreshes).
*/
void Finalize(HMODULE instance, UINT id)
{
	std::map<UINT, eMeasureType>::iterator i1 = g_Types.find(id);
	if (i1 != g_Types.end())
	{
		g_Types.erase(i1);
	}

	std::map<UINT, int>::iterator i2 = g_Indexes.find(id);
	if (i2 != g_Indexes.end())
	{
		g_Indexes.erase(i2);
	}
}

/*
  Returns the version number of the plugin. The value
  can be calculated like this: Major * 1000 + Minor.
  So, e.g. 2.31 would be 2031.
*/
UINT GetPluginVersion()
{
	return 1000;
}

/*
  Returns the author of the plugin for the about dialog.
*/
LPCTSTR GetPluginAuthor()
{
	return L"Arthur Liberman - ALCPU (arthur_liberman@hotmail.com)";
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
		LSLog(LOG_WARNING, L"Rainmeter", L"CoreTempPlugin: Incorrect CoreTempType, assuming Temperature.");
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

double getValues(UINT i_Id)
{
	double value;

	switch (g_Types[i_Id])
	{
	case MeasureTemperature:
		value = proxy.GetTemp(g_Indexes[i_Id]);
		break;

	case MeasureMaxTemperature:
		value = getHighestTemp();
		break;

	case MeasureTjMax:
		value = proxy.GetTjMax(g_Indexes[i_Id]);
		break;

	case MeasureLoad:
		value = proxy.GetCoreLoad(g_Indexes[i_Id]);
		break;

	case MeasureVid:
		value = proxy.GetVID();
		break;

	case MeasureCpuSpeed:
		value = proxy.GetCPUSpeed();
		break;

	case MeasureBusSpeed:
		value = proxy.GetFSBSpeed();
		break;

	case MeasureBusMultiplier:
		value = proxy.GetMultiplier();
		break;

	default:
		value = 0;
		break;
	}

	return value;
}

LPCTSTR getString(UINT i_Id)
{
	static TCHAR string[1000];

	switch (g_Types[i_Id])
	{
	case MeasureTemperature:
		_stprintf_s(string, L"%.0f", proxy.GetTemp(g_Indexes[i_Id]));
		break;

	case MeasureMaxTemperature:
		_stprintf_s(string, L"%.0f", getHighestTemp());
		break;

	case MeasureTjMax:
		_stprintf_s(string, L"%d", proxy.GetTjMax(g_Indexes[i_Id]));
		break;

	case MeasureLoad:
		_stprintf_s(string, L"%d", proxy.GetCoreLoad(g_Indexes[i_Id]));
		break;

	case MeasureVid:
		_stprintf_s(string, L"%.4f", proxy.GetVID());
		break;

	case MeasureCpuSpeed:
		_stprintf_s(string, L"%.2f", proxy.GetCPUSpeed());
		break;

	case MeasureBusSpeed:
		_stprintf_s(string, L"%.2f", proxy.GetFSBSpeed());
		break;

	case MeasureBusMultiplier:
		_stprintf_s(string, L"%.1f", proxy.GetMultiplier());
		break;

	case MeasureCpuName:
		_stprintf_s(string, L"%S", proxy.GetCPUName());
		break;

	default:
		string[0] = '\0';
		break;
	}

	return string;
}