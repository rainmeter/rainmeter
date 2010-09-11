/*
  Copyright (C) 2001 Kimmo Pekkola

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

#include "StdAfx.h"
#include "MeasureCPU.h"
#include "Rainmeter.h"
#include "System.h"
#include "Error.h"

#define STATUS_SUCCESS					0
#define STATUS_INFO_LENGTH_MISMATCH		0xC0000004

#define SystemProcessorPerformanceInformation	8

#define Li2Double(x) ((double)((x).HighPart) * 4.294967296E9 + (double)((x).LowPart))
#define Ft2Double(x) ((double)((x).dwHighDateTime) * 4.294967296E9 + (double)((x).dwLowDateTime))

// ntdll!NtQuerySystemInformation (NT specific!)
//
// The function copies the system information of the
// specified type into a buffer
//
// NTSYSAPI
// NTSTATUS
// NTAPI
// NtQuerySystemInformation(
//    IN UINT SystemInformationClass,    // information type
//    OUT PVOID SystemInformation,       // pointer to buffer
//    IN ULONG SystemInformationLength,  // buffer size in bytes
//    OUT PULONG ReturnLength OPTIONAL   // pointer to a 32-bit
//                                       // variable that receives
//                                       // the number of bytes
//                                       // written to the buffer 
// );

/*
** CMeasureCPU
**
** The constructor
**
*/
CMeasureCPU::CMeasureCPU(CMeterWindow* meterWindow) : CMeasure(meterWindow)
{
	m_CPUFromRegistry = false;
	m_MaxValue = 100.0;
	m_MinValue = 0.0;
	m_FirstTime = true;

	m_Processor = 0;

	m_NtQuerySystemInformation = (PROCNTQSI)GetProcAddress(
										  GetModuleHandle(L"ntdll"),
										 "NtQuerySystemInformation"
										 );
	m_GetSystemTimes = (PROCGST)GetProcAddress(
								GetModuleHandle(L"kernel32"),
								"GetSystemTimes"
								);

	SYSTEM_INFO systemInfo = {0};
	GetSystemInfo(&systemInfo);
	m_NumOfProcessors = (int)systemInfo.dwNumberOfProcessors;
}

/*
** ~CMeasureCPU
**
** The destructor
**
*/
CMeasureCPU::~CMeasureCPU()
{
	if(m_CPUFromRegistry)
	{
		// Stop the counter if it was started
		HKEY hkey;
		DWORD dwDataSize;
		DWORD dwType;
		DWORD dwDummy;

		RegOpenKeyEx(HKEY_DYN_DATA, L"PerfStats\\StopStat", 0, KEY_ALL_ACCESS, &hkey); 
		dwDataSize = sizeof(dwDummy); 
		RegQueryValueEx(hkey, L"KERNEL\\CPUUsage", NULL, &dwType, (LPBYTE)&dwDummy, &dwDataSize); 
		RegCloseKey(hkey); 
	}
}

/*
** ReadConfig
**
** Reads the measure specific configs.
**
*/
void CMeasureCPU::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	CMeasure::ReadConfig(parser, section);

	int processor = parser.ReadInt(section, L"Processor", 0);

	if (processor < 0 || processor > m_NumOfProcessors)
	{
		DebugLog(L"[%s] Invalid Processor: %i", section, processor);

		processor = 0;
	}

	if (processor != m_Processor)
	{
		m_Processor = processor;
		m_FirstTime = true;
	}

	if (m_FirstTime)
	{
		if (m_Processor == 0 && m_GetSystemTimes == NULL)
		{
			m_OldTime.assign(m_NumOfProcessors * 2, 0.0);
		}
		else
		{
			m_OldTime.assign(2, 0.0);
		}
	}
}

/*
** Update
**
** Updates the current CPU utilization value. On NT the value is taken
** from the performance counters and on 9x we'll use the registry.
**
*/
bool CMeasureCPU::Update()
{
	if (!CMeasure::PreUpdate()) return false;

	if (CSystem::IsNT())
	{
		if (m_Processor == 0 && m_GetSystemTimes)
		{
			BOOL status;
			FILETIME ftIdleTime, ftKernelTime, ftUserTime;

			// get new CPU's idle/kernel/user time
			status = m_GetSystemTimes(&ftIdleTime, &ftKernelTime, &ftUserTime);
			if (status == 0) return false;

			CalcUsage(Ft2Double(ftIdleTime),
				Ft2Double(ftKernelTime) + Ft2Double(ftUserTime));
		}
		else if (m_NtQuerySystemInformation)
		{
			LONG status;
			BYTE* buf = NULL;
			ULONG bufSize = 0;

			int loop = 0;

			do
			{
				ULONG size = 0;

				status = m_NtQuerySystemInformation(SystemProcessorPerformanceInformation, buf, bufSize, &size);
				if (status == STATUS_SUCCESS) break;

				if (status == STATUS_INFO_LENGTH_MISMATCH)
				{
					if (size == 0)  // Returned required buffer size is always 0 on Windows 2000/XP.
					{
						if (bufSize == 0)
						{
							bufSize = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * m_NumOfProcessors;
						}
						else
						{
							bufSize += sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);
						}
					}
					else
					{
						if (size != bufSize)
						{
							bufSize = size;
						}
						else  // ??
						{
							bufSize += sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);
						}
					}

					if (buf) delete [] buf;
					buf = new BYTE[bufSize];
				}
				else  // failed
				{
					if (buf) delete [] buf;
					return false;
				}

				++loop;
			} while (loop < 10);

			if (status != STATUS_SUCCESS)  // failed
			{
				if (buf) delete [] buf;
				return false;
			}

			SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION* systemPerfInfo = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION*)buf;

			if (m_Processor == 0)
			{
				CalcAverageUsage(systemPerfInfo);
			}
			else
			{
				int processor = m_Processor - 1;

				CalcUsage(Li2Double(systemPerfInfo[processor].IdleTime),
					Li2Double(systemPerfInfo[processor].KernelTime) + Li2Double(systemPerfInfo[processor].UserTime));
			}

			delete [] buf;
		}
		else
		{
			return false;
		}
	}
	else
	{
		// It's a wintendo!
		HKEY hkey;
		DWORD dwDataSize;
		DWORD dwType;
		DWORD dwCpuUsage;

		if(m_FirstTime)
		{
			RegOpenKeyEx(HKEY_DYN_DATA, L"PerfStats\\StartStat", 0, KEY_ALL_ACCESS, &hkey); 
			dwDataSize = sizeof(dwCpuUsage); 
			RegQueryValueEx(hkey, L"KERNEL\\CPUUsage", NULL, &dwType, (LPBYTE)&dwCpuUsage, &dwDataSize); 
			RegCloseKey(hkey);

			m_FirstTime = false;
		}

		RegOpenKeyEx(HKEY_DYN_DATA, L"PerfStats\\StatData", 0, KEY_ALL_ACCESS, &hkey); 
		dwDataSize = sizeof(dwCpuUsage); 
		RegQueryValueEx(hkey, L"KERNEL\\CPUUsage", NULL, &dwType, (LPBYTE)&dwCpuUsage, &dwDataSize); 
		RegCloseKey(hkey); 

		m_Value = dwCpuUsage;
		m_CPUFromRegistry = true;
	}

	return PostUpdate();
}

/*
** CalcUsage
**
** Calculates the current CPU utilization value.
**
*/
void CMeasureCPU::CalcUsage(double idleTime, double systemTime)
{
	if (!m_FirstTime)
	{
		double dbCpuUsage;

		// CurrentCpuUsage% = 100 - ((IdleTime / SystemTime) * 100)
		dbCpuUsage = 100.0 - ((idleTime - m_OldTime[0]) / (systemTime - m_OldTime[1])) * 100.0;

		dbCpuUsage = min(dbCpuUsage, 100.0);
		m_Value    = max(dbCpuUsage, 0.0);
	}
	else
	{
		m_FirstTime = false;
	}

	// store new CPU's idle and system time
	m_OldTime[0] = idleTime;
	m_OldTime[1] = systemTime;
}

/*
** CalcAverageUsage
**
** Calculates the current CPU average utilization value.
** This function is used if GetSystemTimes function is not available.
**
*/
void CMeasureCPU::CalcAverageUsage(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION* systemPerfInfo)
{
	if(!m_FirstTime)
	{
		double dbIdleTimeDiff = 0, dbSystemTimeDiff = 0;
		double dbCpuUsage;

		for (int i = 0; i < m_NumOfProcessors; ++i)
		{
			double dbIdleTime, dbSystemTime;

			dbIdleTime = Li2Double(systemPerfInfo[i].IdleTime);
			dbSystemTime = Li2Double(systemPerfInfo[i].KernelTime) + Li2Double(systemPerfInfo[i].UserTime);

			dbIdleTimeDiff += dbIdleTime - m_OldTime[i * 2 + 0];
			dbSystemTimeDiff += dbSystemTime - m_OldTime[i * 2 + 1];

			// store new CPU's idle and system time
			m_OldTime[i * 2 + 0] = dbIdleTime;
			m_OldTime[i * 2 + 1] = dbSystemTime;
		}

		// CurrentCpuUsage% = 100 - ((IdleTime / SystemTime) * 100)
		dbCpuUsage = 100.0 - (dbIdleTimeDiff / dbSystemTimeDiff) * 100.0;

		dbCpuUsage = min(dbCpuUsage, 100.0);
		m_Value    = max(dbCpuUsage, 0.0);
	}
	else
	{
		// store new CPU's idle and system time
		for (int i = 0; i < m_NumOfProcessors; ++i)
		{
			m_OldTime[i * 2 + 0] = Li2Double(systemPerfInfo[i].IdleTime);
			m_OldTime[i * 2 + 1] = Li2Double(systemPerfInfo[i].KernelTime) + Li2Double(systemPerfInfo[i].UserTime);
		}

		m_FirstTime = false;
	}
}
