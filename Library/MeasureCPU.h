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

#ifndef __MEASURECPU_H__
#define __MEASURECPU_H__

#include "Measure.h"

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER Reserved1[2];
    ULONG Reserved2;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

typedef LONG (WINAPI *PROCNTQSI)(UINT,PVOID,ULONG,PULONG);
typedef BOOL (WINAPI *PROCGST)(LPFILETIME lpIdleTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime);

class CMeasureCPU : public CMeasure
{
public:
	CMeasureCPU(CMeterWindow* meterWindow);
	virtual ~CMeasureCPU();

	virtual void ReadConfig(CConfigParser& parser, const WCHAR* section);
	virtual bool Update();

protected:
	void CalcUsage(double idleTime, double systemTime);
	void CalcAverageUsage(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION* systemPerfInfo);

	bool m_CPUFromRegistry;
	bool m_FirstTime;

	int m_Processor;
	int m_NumOfProcessors;

	PROCNTQSI m_NtQuerySystemInformation;
	PROCGST   m_GetSystemTimes;

	std::vector<double> m_OldTime;
};

#endif
