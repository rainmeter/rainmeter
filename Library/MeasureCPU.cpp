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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "MeasureCPU.h"
#include "Rainmeter.h"
#include "System.h"
#include "Error.h"

#define STATUS_SUCCESS					0
#define STATUS_INFO_LENGTH_MISMATCH		0xC0000004

#define SystemProcessorPerformanceInformation	8

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER Reserved1[2];
    ULONG Reserved2;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

#define Li2Double(x) ((double)((x).QuadPart))
#define Ft2Double(x) ((double)((x).dwHighDateTime) * 4.294967296E9 + (double)((x).dwLowDateTime))

PROCNTQSI CMeasureCPU::c_NtQuerySystemInformation = NULL;
int CMeasureCPU::c_NumOfProcessors = 0;
ULONG CMeasureCPU::c_BufferSize = 0;

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
** The constructor
**
*/
CMeasureCPU::CMeasureCPU(CMeterWindow* meterWindow, const WCHAR* name) : CMeasure(meterWindow, name),
	m_Processor(),
	m_OldTime()
{
	m_MaxValue = 100.0;

	if (c_NtQuerySystemInformation == NULL)
	{
		c_NtQuerySystemInformation = (PROCNTQSI)GetProcAddress(GetModuleHandle(L"ntdll"), "NtQuerySystemInformation");
	}
	if (c_NumOfProcessors == 0)
	{
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);
		c_NumOfProcessors = (int)systemInfo.dwNumberOfProcessors;
	}
}

/*
** The destructor
**
*/
CMeasureCPU::~CMeasureCPU()
{
}

/*
** Reads the measure specific configs.
**
*/
void CMeasureCPU::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	CMeasure::ReadConfig(parser, section);

	int processor = parser.ReadInt(section, L"Processor", 0);

	if (processor < 0 || processor > c_NumOfProcessors)
	{
		LogWithArgs(LOG_WARNING, L"CPU: Processor=%i invalid in [%s]", m_Processor, section);
		processor = 0;
	}

	if (processor != m_Processor)
	{
		m_Processor = processor;
		m_OldTime[0] = m_OldTime[1] = 0.0;
	}
}

/*
** Updates the current CPU utilization value.
**
*/
bool CMeasureCPU::Update()
{
	if (!CMeasure::PreUpdate()) return false;

	if (m_Processor == 0)
	{
		BOOL status;
		FILETIME ftIdleTime, ftKernelTime, ftUserTime;

		// get new CPU's idle/kernel/user time
		status = GetSystemTimes(&ftIdleTime, &ftKernelTime, &ftUserTime);
		if (status == 0) return false;

		CalcUsage(Ft2Double(ftIdleTime),
			Ft2Double(ftKernelTime) + Ft2Double(ftUserTime));
	}
	else if (c_NtQuerySystemInformation)
	{
		LONG status;
		ULONG bufSize = c_BufferSize;
		BYTE* buf = (bufSize > 0) ? new BYTE[bufSize] : NULL;

		int loop = 0;

		do
		{
			ULONG size = 0;

			status = c_NtQuerySystemInformation(SystemProcessorPerformanceInformation, buf, bufSize, &size);
			if (status == STATUS_SUCCESS || status != STATUS_INFO_LENGTH_MISMATCH) break;

			else  // status == STATUS_INFO_LENGTH_MISMATCH
			{
				if (size == 0)  // Returned required buffer size is always 0 on Windows 2000/XP.
				{
					if (bufSize == 0)
					{
						bufSize = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * c_NumOfProcessors;
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

				delete [] buf;
				buf = new BYTE[bufSize];
			}
			++loop;
		}
		while (loop < 5);

		if (status != STATUS_SUCCESS)  // failed
		{
			delete [] buf;
			return false;
		}

		if (bufSize != c_BufferSize)
		{
			// Store the new buffer size
			c_BufferSize = bufSize;
		}

		SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION* systemPerfInfo = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION*)buf;

		int processor = m_Processor - 1;

		CalcUsage(Li2Double(systemPerfInfo[processor].IdleTime),
			Li2Double(systemPerfInfo[processor].KernelTime) + Li2Double(systemPerfInfo[processor].UserTime));

		delete [] buf;
	}
	else
	{
		return false;
	}

	return PostUpdate();
}

/*
** Calculates the current CPU utilization value.
**
*/
void CMeasureCPU::CalcUsage(double idleTime, double systemTime)
{
	// CurrentCpuUsage% = 100 - ((IdleTime / SystemTime) * 100)
	double dbCpuUsage = 100.0 - ((idleTime - m_OldTime[0]) / (systemTime - m_OldTime[1])) * 100.0;

	dbCpuUsage = min(dbCpuUsage, 100.0);
	m_Value    = max(dbCpuUsage, 0.0);

	// store new CPU's idle and system time
	m_OldTime[0] = idleTime;
	m_OldTime[1] = systemTime;
}
