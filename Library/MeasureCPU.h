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

typedef struct
{
    LARGE_INTEGER   liIdleTime;
    DWORD           dwSpare[76];
} SYSTEM_PERFORMANCE_INFORMATION;

typedef struct
{
    LARGE_INTEGER liKeBootTime;
    LARGE_INTEGER liKeSystemTime;
    LARGE_INTEGER liExpTimeZoneBias;
    ULONG         uCurrentTimeZoneId;
    DWORD         dwReserved;
} SYSTEM_TIME_INFORMATION;

typedef LONG (WINAPI *PROCNTQSI)(UINT,PVOID,ULONG,PULONG);

class CMeasureCPU : public CMeasure
{
public:
	CMeasureCPU(CMeterWindow* meterWindow);
	virtual ~CMeasureCPU();

	virtual bool Update();

protected:
	bool m_CPUFromRegistry;
	bool m_FirstTime;

	PROCNTQSI m_NtQuerySystemInformation;

	SYSTEM_PERFORMANCE_INFORMATION m_SysPerfInfo;
	SYSTEM_TIME_INFORMATION        m_SysTimeInfo;
	SYSTEM_INFO                    m_SystemInfo;
	LARGE_INTEGER                  m_OldIdleTime;
	LARGE_INTEGER                  m_OldSystemTime;
};

#endif
