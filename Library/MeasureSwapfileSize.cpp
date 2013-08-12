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
#include "MeasureSwapfileSize.h"
#include "ConfigParser.h"

/*
** The constructor
**
*/
MeasureSwapfileSize::MeasureSwapfileSize(MeterWindow* meterWindow, const WCHAR* name) : Measure(meterWindow, name),
	m_Total(false)
{
	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&stat);
	CheckSwapfileEnabled();

	if (m_bSwapfileEnabled == true)
	{
		m_MaxValue = (double)(__int64)(stat.ullTotalPageFile - stat.ullTotalPhys);
	}
	else
	{
		m_MaxValue = 0;
	}
}

/*
** The destructor
**
*/
MeasureSwapfileSize::~MeasureSwapfileSize()
{
}

/*
** Updates the current virtual memory value.
**
*/
void MeasureSwapfileSize::UpdateValue()
{
	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&stat);
	CheckSwapfileEnabled();

	if (m_bSwapfileEnabled == true)
	{
		m_MaxValue = (double)(__int64)(stat.ullTotalPageFile - stat.ullTotalPhys);

		if (m_Total)
		{
			m_Value = m_MaxValue;
		}
		else
		{
			m_Value = (double)(__int64)(m_MaxValue - (stat.ullAvailPageFile - stat.ullAvailPhys));
		}
	}
	else
	{
		m_Value = m_MaxValue = 0;
	}
}

/*
** Read the options specified in the ini file.
**
*/
void MeasureSwapfileSize::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	double oldMaxValue = m_MaxValue;
	Measure::ReadOptions(parser, section);
	m_MaxValue = oldMaxValue;

	m_Total = (1 == parser.ReadInt(section, L"Total", 0));
}

void MeasureSwapfileSize::CheckSwapfileEnabled(void)
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