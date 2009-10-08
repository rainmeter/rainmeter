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

#pragma warning(disable: 4996)

#include "StdAfx.h"
#include "MeasureDiskSpace.h"
#include "Rainmeter.h"

/*
** CMeasureDiskSpace
**
** The constructor
**
*/
CMeasureDiskSpace::CMeasureDiskSpace(CMeterWindow* meterWindow) : CMeasure(meterWindow)
{
	m_Total = false;
	m_Label = false;
	m_IgnoreRemovable = true;
}

/*
** ~CMeasureDiskSpace
**
** The destructor
**
*/
CMeasureDiskSpace::~CMeasureDiskSpace()
{
}

/*
** Update
**
** Updates the current disk free space value. 
**
*/
bool CMeasureDiskSpace::Update()
{
	if (!CMeasure::PreUpdate()) return false;

	if (!m_Drive.empty())
	{
		UINT type = GetDriveType(m_Drive.c_str());
		if (type != DRIVE_CDROM && (!m_IgnoreRemovable || type != DRIVE_REMOVABLE))	// Ignore CD-ROMS and removable drives
		{
			ULARGE_INTEGER i64FreeBytesToCaller, i64TotalBytes, i64FreeBytes;

			if(GetDiskFreeSpaceEx(m_Drive.c_str(), &i64FreeBytesToCaller, &i64TotalBytes, &i64FreeBytes))
			{
				LARGE_INTEGER tmpInt;
				if (m_Total)
				{
					tmpInt.QuadPart = i64TotalBytes.QuadPart;
				}
				else
				{
					tmpInt.QuadPart = i64FreeBytes.QuadPart;
				}
				m_Value = (double)tmpInt.QuadPart;
			}
		}
		else
		{
			m_Value = 0;
		}

		if (m_Label) 
		{
			WCHAR volumeName[MAX_PATH] = {0};
			GetVolumeInformation(m_Drive.c_str(), volumeName, MAX_PATH, NULL, NULL, NULL, NULL, 0);
			m_LabelName = volumeName;
		}
	}

	return PostUpdate();
}

/*
** GetStringValue
**
** Returns the time as string.
**
*/
const WCHAR* CMeasureDiskSpace::GetStringValue(bool autoScale, double scale, int decimals, bool percentual)
{
	if (m_Label) 
	{
		return CheckSubstitute(m_LabelName.c_str());
	}

	return CMeasure::GetStringValue(autoScale, scale, decimals, percentual);
}

/*
** ReadConfig
**
** Reads the measure specific configs.
**
*/
void CMeasureDiskSpace::ReadConfig(CConfigParser& parser, const WCHAR* section)
{
	CMeasure::ReadConfig(parser, section);

	m_Drive = parser.ReadString(section, L"Drive", L"C:\\");
	if (m_Drive.empty())
	{
		DebugLog(L"Drive letter is not given.");
		m_Value = 0;
		m_LabelName = L"";
	}
	else if (m_Drive[m_Drive.length() - 1] != L'\\')  // E.g. "C:"
	{
		m_Drive += L"\\";  // A trailing backslash is required.
	}

	m_Total = (1 == parser.ReadInt(section, L"Total", 0));
	m_Label = (1 == parser.ReadInt(section, L"Label", 0));
	m_IgnoreRemovable = (1 == parser.ReadInt(section, L"IgnoreRemovable", 1));

	// Set the m_MaxValue
	if (!m_Drive.empty())
	{
		UINT type = GetDriveType(m_Drive.c_str());
		if (type != DRIVE_CDROM && (!m_IgnoreRemovable || type != DRIVE_REMOVABLE))	// Ignore CD-ROMS and removable drives
		{
			ULARGE_INTEGER i64FreeBytesToCaller, i64TotalBytes, i64FreeBytes;
	
			if(GetDiskFreeSpaceEx(m_Drive.c_str(), &i64FreeBytesToCaller, &i64TotalBytes, &i64FreeBytes))
			{
				LARGE_INTEGER tmpInt;
				tmpInt.QuadPart = i64TotalBytes.QuadPart;
				m_MaxValue = (double)tmpInt.QuadPart;
			}
		}
	}
}

