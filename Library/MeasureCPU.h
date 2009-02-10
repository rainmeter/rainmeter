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
  $Header: /home/cvsroot/Rainmeter/Library/MeasureCPU.h,v 1.1.1.1 2005/07/10 18:51:06 rainy Exp $

  $Log: MeasureCPU.h,v $
  Revision 1.1.1.1  2005/07/10 18:51:06  rainy
  no message

  Revision 1.7  2004/06/05 10:55:54  rainy
  Too much changes to be listed in here...

  Revision 1.6  2002/04/26 18:24:16  rainy
  Modified the Update method to support disabled measures.

  Revision 1.5  2002/01/16 16:07:00  rainy
  Fixed a bug with the CPU meter in 9x.

  Revision 1.4  2001/09/26 16:27:15  rainy
  Changed the interfaces a bit.

  Revision 1.3  2001/09/01 13:00:10  rainy
  Slight changes in the interface. The value is now measured only once if possible.

  Revision 1.2  2001/08/12 15:46:33  Rainy
  Adjusted Update()'s interface.
  Fixed a bug that prevent more than one CPU meter.

  Revision 1.1.1.1  2001/08/11 10:58:19  Rainy
  Added to CVS.

*/

#ifndef __MEASURECPU_H__
#define __MEASURECPU_H__

#include "Measure.h"

typedef struct
{
    DWORD   dwUnknown1;
    ULONG   uKeMaximumIncrement;
    ULONG   uPageSize;
    ULONG   uMmNumberOfPhysicalPages;
    ULONG   uMmLowestPhysicalPage;
    ULONG   uMmHighestPhysicalPage;
    ULONG   uAllocationGranularity;
    PVOID   pLowestUserAddress;
    PVOID   pMmHighestUserAddress;
    ULONG   uKeActiveProcessors;
    BYTE    bKeNumberProcessors;
    BYTE    bUnknown2;
    WORD    wUnknown3;
} SYSTEM_BASIC_INFORMATION;

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
