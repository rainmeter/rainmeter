/*
  Copyright (C) 2013 NAME

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

  Open Hardware Monitor code is subject to the Mozilla Public
  License:

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0.  If a copy of the MPL was not distributed with
  this file, You can obtain one at http://mozilla.org/MPL/2.0/.

  The original source code for this program is the project Open
  Hardware Monitor (http://openhardwaremonitor.org/).

  The intent of this inclusion is to allow Rainmeter to retrieve
  hardware related information without requiring another program
  to run in the background.
*/

#include "NativeInterface.h"
#include "CLIWrapper.h"

namespace OpenHardwareMonitor{

#ifdef __cplusplus
extern "C"
{
#endif

	__declspec(dllexport) void OHM_GPU_GetUsage(int Card)
	{
	}

	__declspec(dllexport) void OHM_GPU_GetTemperature(int Card)
	{
	}

	__declspec(dllexport) void OHM_GPU_GetCoreFreq(int Card)
	{
	}

	__declspec(dllexport) void OHM_GPU_GetMemFreq(int Card)
	{
	}

	__declspec(dllexport) void OHM_GPU_GetVoltage(int Card)
	{
	}

	__declspec(dllexport) void OHM_GPU_GetFanSpeed(int Card)
	{
	}

	__declspec(dllexport) void OHM_CPU_GetTemperature(int Core)
	{
	}

	__declspec(dllexport) void OHM_CPU_GetFrequency(int Core)
	{
	}

	__declspec(dllexport) void OHM_CPU_GetVoltage(int Core)
	{
	}

	__declspec(dllexport) void OHM_CPU_GetFanSpeed(int Card)
	{
	}

	__declspec(dllexport) void OHM_MB_GetTemperature(int Sensor)
	{
	}

	__declspec(dllexport) void OHM_MB_GetFanSpeed(int Sensor)
	{
	}

#ifdef __cplusplus
}
#endif

}