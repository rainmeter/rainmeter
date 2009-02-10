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
/*
  $Header: /home/cvsroot/Rainmeter/Library/MeasureCPU.cpp,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeasureCPU.cpp,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.10  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.9  2002/04/26 18:24:16  rainy
  Modified the Update method to support disabled measures.

  Revision 1.8  2002/03/31 09:58:54  rainy
  Added some comments

  Revision 1.7  2002/01/16 16:07:00  rainy
  Fixed a bug with the CPU meter in 9x.

  Revision 1.6  2001/10/28 10:22:49  rainy
  Changed the IsNT() function return value check

  Revision 1.5  2001/09/26 16:27:15  rainy
  Changed the interfaces a bit.

  Revision 1.4  2001/09/01 13:00:10  rainy
  Slight changes in the interface. The value is now measured only once if possible.

  Revision 1.3  2001/08/19 09:15:21  rainy
  Added support for value invert.

  Revision 1.2  2001/08/12 15:46:34  Rainy
  Adjusted Update()'s interface.
  Fixed a bug that prevent more than one CPU meter.

  Revision 1.1.1.1  2001/08/11 10:58:19  Rainy
  Added to CVS.

*/
#pragma warning(disable: 4996)

#include "MeasureCPU.h"
#include "Rainmeter.h"

#define SystemBasicInformation       0
#define SystemPerformanceInformation 2
#define SystemTimeInformation        3

#define Li2Double(x) ((double)((x).HighPart) * 4.294967296E9 + (double)((x).LowPart))

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

	m_NtQuerySystemInformation = NULL;
	m_OldIdleTime.QuadPart = 0;
	m_OldSystemTime.QuadPart = 0;

	m_NtQuerySystemInformation = (PROCNTQSI)GetProcAddress(
										  GetModuleHandle(L"ntdll"),
										 "NtQuerySystemInformation"
										 );

	GetSystemInfo(&m_SystemInfo);
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
** Update
**
** Updates the current CPU utilization value. On NT the value is taken
** from the performance counters and on 9x we'll use the registry.
**
*/
bool CMeasureCPU::Update()
{
	if (!CMeasure::PreUpdate()) return false;

	if (CRainmeter::IsNT() != PLATFORM_9X)
	{
		if (m_NtQuerySystemInformation)
		{
			// This code is 'borrowed' from http://www.codepile.com/tric21.shtml
			double dbIdleTime;
			double dbSystemTime;
			LONG status;

			// get new system time
			status = m_NtQuerySystemInformation(SystemTimeInformation, &m_SysTimeInfo, sizeof(m_SysTimeInfo), 0);
			if (status != NO_ERROR) return false;

			// get new CPU's idle time
			status = m_NtQuerySystemInformation(SystemPerformanceInformation,&m_SysPerfInfo,sizeof(m_SysPerfInfo),NULL);
			if (status != NO_ERROR) return false;

			// if it's a first call - skip it
			if(!m_FirstTime)
			{
				// CurrentValue = NewValue - OldValue
				dbIdleTime = Li2Double(m_SysPerfInfo.liIdleTime) - Li2Double(m_OldIdleTime);
				dbSystemTime = Li2Double(m_SysTimeInfo.liKeSystemTime) - Li2Double(m_OldSystemTime);

				// CurrentCpuIdle = IdleTime / SystemTime
				dbIdleTime = dbIdleTime / dbSystemTime;

				// CurrentCpuUsage% = 100 - (CurrentCpuIdle * 100) / NumberOfProcessors
				dbIdleTime = 100.0 - dbIdleTime * 100.0 / (double)m_SystemInfo.dwNumberOfProcessors + 0.5;

				m_Value = min(dbIdleTime, 100.0);
				m_Value = max(m_Value, 0.0);
		   }

			// store new CPU's idle and system time
			m_OldIdleTime = m_SysPerfInfo.liIdleTime;
			m_OldSystemTime = m_SysTimeInfo.liKeSystemTime;
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
		}

		RegOpenKeyEx(HKEY_DYN_DATA, L"PerfStats\\StatData", 0, KEY_ALL_ACCESS, &hkey); 
		dwDataSize = sizeof(dwCpuUsage); 
		RegQueryValueEx(hkey, L"KERNEL\\CPUUsage", NULL, &dwType, (LPBYTE)&dwCpuUsage, &dwDataSize); 
		RegCloseKey(hkey); 

		m_Value = dwCpuUsage;
		m_CPUFromRegistry = true;
	}

	m_FirstTime = false;

	return PostUpdate();
}
