/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureCPU.h"
#include "Rainmeter.h"
#include "System.h"

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

FPNTQSI MeasureCPU::c_NtQuerySystemInformation = nullptr;
int MeasureCPU::c_NumOfProcessors = 0;
ULONG MeasureCPU::c_BufferSize = 0;

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

MeasureCPU::MeasureCPU(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Processor(),
	m_OldTime()
{
	m_MaxValue = 100.0;
}

MeasureCPU::~MeasureCPU()
{
}

/*
** Read the options specified in the ini file.
**
*/
void MeasureCPU::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	Measure::ReadOptions(parser, section);

	int processor = parser.ReadInt(section, L"Processor", 0);

	if (processor < 0 || processor > c_NumOfProcessors)
	{
		LogWarningF(this, L"CPU: Processor=%i is not valid", processor);
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
void MeasureCPU::UpdateValue()
{
	if (m_Processor == 0)
	{
		BOOL status;
		FILETIME ftIdleTime, ftKernelTime, ftUserTime;

		// get new CPU's idle/kernel/user time
		status = GetSystemTimes(&ftIdleTime, &ftKernelTime, &ftUserTime);
		if (status == 0) return;

		CalcUsage(Ft2Double(ftIdleTime),
			Ft2Double(ftKernelTime) + Ft2Double(ftUserTime));
	}
	else if (c_NtQuerySystemInformation)
	{
		LONG status;
		ULONG bufSize = c_BufferSize;
		BYTE* buf = (bufSize > 0) ? new BYTE[bufSize] : nullptr;

		int loop = 0;

		do
		{
			ULONG size = 0;

			status = c_NtQuerySystemInformation(SystemProcessorPerformanceInformation, buf, bufSize, &size);
			if (status == STATUS_INFO_LENGTH_MISMATCH)
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
			else
			{
				break;
			}

			++loop;
		}
		while (loop < 5);

		if (status == STATUS_SUCCESS)
		{
			if (bufSize != c_BufferSize)
			{
				// Store the new buffer size
				c_BufferSize = bufSize;
			}

			PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION systemPerfInfo = (PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)buf;

			int processor = m_Processor - 1;

			CalcUsage(Li2Double(systemPerfInfo[processor].IdleTime),
				Li2Double(systemPerfInfo[processor].KernelTime) + Li2Double(systemPerfInfo[processor].UserTime));
		}

		delete [] buf;
	}
}

/*
** Calculates the current CPU utilization value.
**
*/
void MeasureCPU::CalcUsage(double idleTime, double systemTime)
{
	// CurrentCpuUsage% = 100 - ((IdleTime / SystemTime) * 100)
	double dbCpuUsage = 100.0 - ((idleTime - m_OldTime[0]) / (systemTime - m_OldTime[1])) * 100.0;

	dbCpuUsage = min(dbCpuUsage, 100.0);
	m_Value    = max(dbCpuUsage, 0.0);

	// store new CPU's idle and system time
	m_OldTime[0] = idleTime;
	m_OldTime[1] = systemTime;
}

void MeasureCPU::InitializeStatic()
{
	c_NtQuerySystemInformation = (FPNTQSI)GetProcAddress(GetModuleHandle(L"ntdll"), "NtQuerySystemInformation");

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	c_NumOfProcessors = (int)systemInfo.dwNumberOfProcessors;
}

void MeasureCPU::FinalizeStatic()
{
}
