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
#include "MeasureMemory.h"
#include "ConfigParser.h"

/*
** The constructor
**
*/
MeasureMemory::MeasureMemory(MeterWindow* meterWindow, const WCHAR* name) : Measure(meterWindow, name),
	m_Total(false)
{
	m_Total = false;
	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&stat);
	PERFORMANCE_INFORMATION info;
	info.cb = sizeof(PERFORMANCE_INFORMATION);
	GetPerformanceInfo(&info, info.cb);
	CheckSwapfileEnabled();

	if (m_bSwapfileEnabled == true)
	{
		m_MaxValue = (double)(__int64)(info.CommitLimit);
	}
	else
	{
		m_MaxValue = (double)(__int64)(stat.ullTotalPhys);
	}
}

/*
** The destructor
**
*/
MeasureMemory::~MeasureMemory()
{
}

/*
** Updates the current virtual memory value.
**
*/
void MeasureMemory::UpdateValue()
{
	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&stat);
	PERFORMANCE_INFORMATION info;
	info.cb = sizeof(PERFORMANCE_INFORMATION);
	GetPerformanceInfo(&info, info.cb);

	if (m_bSwapfileEnabled == true)
	{
		m_MaxValue = (double)(__int64)(stat.ullTotalPageFile);

		if (m_Total)
		{
			m_Value = m_MaxValue;
		}
		else
		{
			m_Value = (double)(__int64)(stat.ullTotalPageFile - stat.ullAvailPageFile);
		}
	}
	else
	{
		m_MaxValue = (double)(__int64)(stat.ullTotalPhys);

		if (m_Total)
		{
			m_Value = m_MaxValue;
		}
		else
		{
			m_Value = (double)(__int64)(stat.ullTotalPhys - stat.ullAvailPhys);
		}
	}
}

/*
** Read the options specified in the ini file.
**
*/
void MeasureMemory::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	double oldMaxValue = m_MaxValue;
	Measure::ReadOptions(parser, section);
	m_MaxValue = oldMaxValue;

	m_Total = parser.ReadBool(section, L"Total", false);
}

void MeasureMemory::CheckSwapfileEnabled(void)
{
	// This only needs to run once (called by constructor) because changing the swapfile size 
	// always requires a restart in the Windows operating system.
	#define BUFFERSIZE 256
	HKEY	hKey;
	DWORD	dwSize = BUFFERSIZE;
	DWORD	dwError;
	CHAR	SwapfileData[BUFFERSIZE];

	for (int i = 0; i < BUFFERSIZE; i++)
	{
		SwapfileData[i] = 0;
	}

	if( RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		dwError = RegQueryValueEx(hKey, TEXT("PagingFiles"),0,0, (LPBYTE)SwapfileData, &dwSize);

		if ( (dwError != ERROR_SUCCESS) || (SwapfileData[0] == 0) || (SwapfileData[0] == '/n') || (SwapfileData[0] == '/0') )
		{
			m_bSwapfileEnabled = false;
		}
		else
		{
			m_bSwapfileEnabled = true;
		}
	}
	else
	{
		m_bSwapfileEnabled = false;
	}

	RegCloseKey(hKey);
}
