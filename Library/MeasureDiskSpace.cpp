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
#include "MeasureDiskSpace.h"
#include "Rainmeter.h"
#include "System.h"

enum DRIVETYPE
{
	DRIVETYPE_ERROR       = 0,
	DRIVETYPE_REMOVED     = 1,
	DRIVETYPE_REMOVABLE   = 3,
	DRIVETYPE_FIXED       = 4,
	DRIVETYPE_NETWORK     = 5,
	DRIVETYPE_CDROM       = 6,
	DRIVETYPE_RAM         = 7,

	DRIVETYPE_MAX         = DRIVETYPE_RAM
};

/*
** The constructor
**
*/
CMeasureDiskSpace::CMeasureDiskSpace(CMeterWindow* meterWindow, const WCHAR* name) : CMeasure(meterWindow, name),
	m_Type(false),
	m_Total(false),
	m_Label(false),
	m_IgnoreRemovable(true),
	m_DiskQuota(false),
	m_OldTotalBytes()
{
}

/*
** The destructor
**
*/
CMeasureDiskSpace::~CMeasureDiskSpace()
{
}

/*
** Updates the current disk free space value.
**
*/
void CMeasureDiskSpace::UpdateValue()
{
	if (!m_Drive.empty())
	{
		const WCHAR* drive = m_Drive.c_str();
		UINT type = GetDriveType(drive);

		if (m_Type)
		{
			switch (type)
			{
			case DRIVE_UNKNOWN:
			case DRIVE_NO_ROOT_DIR:
				m_Value = DRIVETYPE_REMOVED;
				m_DriveInfo = L"Removed";
				break;
			case DRIVE_REMOVABLE:
				m_Value = DRIVETYPE_REMOVABLE;
				m_DriveInfo = L"Removable";
				break;
			case DRIVE_FIXED:
				m_Value = DRIVETYPE_FIXED;
				m_DriveInfo = L"Fixed";
				break;
			case DRIVE_REMOTE:
				m_Value = DRIVETYPE_NETWORK;
				m_DriveInfo = L"Network";
				break;
			case DRIVE_CDROM:
				m_Value = DRIVETYPE_CDROM;
				m_DriveInfo = L"CDRom";
				break;
			case DRIVE_RAMDISK:
				m_Value = DRIVETYPE_RAM;
				m_DriveInfo = L"Ram";
				break;
			default:
				m_Value = DRIVETYPE_ERROR;
				m_DriveInfo = L"Error";
				break;
			}
		}
		else
		{
			BOOL sizeResult = FALSE;
			ULONGLONG i64TotalBytes, i64FreeBytes;

			if (type != DRIVE_NO_ROOT_DIR)
			{
				if (type != DRIVE_CDROM && (!m_IgnoreRemovable || type != DRIVE_REMOVABLE))	// Ignore CD-ROMS and removable drives
				{
					UINT oldMode = SetErrorMode(0);
					SetErrorMode(oldMode | SEM_FAILCRITICALERRORS);  // Prevent the system from displaying message box
					if (!m_DiskQuota)
					{
						sizeResult = GetDiskFreeSpaceEx(drive, NULL, (PULARGE_INTEGER)&i64TotalBytes, (PULARGE_INTEGER)&i64FreeBytes);
					}
					else
					{
						sizeResult = GetDiskFreeSpaceEx(drive, (PULARGE_INTEGER)&i64FreeBytes, (PULARGE_INTEGER)&i64TotalBytes, NULL);
					}
					SetErrorMode(oldMode);  // Reset
				}
			}

			if (sizeResult)
			{
				m_Value = (double)(__int64)((m_Total) ? i64TotalBytes : i64FreeBytes);

				if (i64TotalBytes != m_OldTotalBytes)
				{
					// Total size was changed, so set new max value.
					m_MaxValue = (double)(__int64)i64TotalBytes;
					m_OldTotalBytes = i64TotalBytes;
				}
			}
			else
			{
				m_Value = 0.0;
				m_MaxValue = 0.0;
				m_OldTotalBytes = 0;
			}

			if (m_Label)
			{
				BOOL labelResult = FALSE;
				WCHAR volumeName[MAX_PATH + 1];

				if (type != DRIVE_NO_ROOT_DIR)
				{
					if (!m_IgnoreRemovable || type != DRIVE_REMOVABLE)	// Ignore removable drives
					{
						UINT oldMode = SetErrorMode(0);
						SetErrorMode(oldMode | SEM_FAILCRITICALERRORS);  // Prevent the system from displaying message box
						labelResult = GetVolumeInformation(drive, volumeName, MAX_PATH + 1, NULL, NULL, NULL, NULL, 0);
						SetErrorMode(oldMode);  // Reset
					}
				}

				m_DriveInfo = (labelResult) ? volumeName : L"";
			}
			else if (!m_DriveInfo.empty())
			{
				m_DriveInfo.clear();
			}
		}
	}
}

/*
** Returns the time as string.
**
*/
const WCHAR* CMeasureDiskSpace::GetStringValue(AUTOSCALE autoScale, double scale, int decimals, bool percentual)
{
	if (m_Type || m_Label)
	{
		return CheckSubstitute(m_DriveInfo.c_str());
	}

	return CMeasure::GetStringValue(autoScale, scale, decimals, percentual);
}

/*
** Read the options specified in the ini file.
**
*/
void CMeasureDiskSpace::ReadOptions(CConfigParser& parser, const WCHAR* section)
{
	double oldMaxValue = m_MaxValue;

	CMeasure::ReadOptions(parser, section);

	m_Drive = parser.ReadString(section, L"Drive", L"C:\\");
	if (m_Drive.empty())
	{
		Log(LOG_WARNING, L"FreeDiskSpace: Drive= empty");
		m_Value = 0.0;
		m_MaxValue = 0.0;
		m_OldTotalBytes = 0;
		m_DriveInfo.clear();
	}
	else if (!CSystem::IsPathSeparator(m_Drive[m_Drive.length() - 1]))  // E.g. "C:"
	{
		m_Drive += L'\\';  // A trailing backslash is required.
	}

	m_Type = (1 == parser.ReadInt(section, L"Type", 0));
	m_Total = (1 == parser.ReadInt(section, L"Total", 0));
	m_Label = (1 == parser.ReadInt(section, L"Label", 0));
	m_IgnoreRemovable = (1 == parser.ReadInt(section, L"IgnoreRemovable", 1));
	m_DiskQuota = (1 == parser.ReadInt(section, L"DiskQuota", 0));
	
	// Set the m_MaxValue
	if (!m_Initialized)
	{
		BOOL result = FALSE;
		ULONGLONG i64TotalBytes;

		if (!m_Drive.empty())
		{
			const WCHAR* drive = m_Drive.c_str();
			UINT type = GetDriveType(drive);
			if (type != DRIVE_NO_ROOT_DIR &&
				type != DRIVE_CDROM && (!m_IgnoreRemovable || type != DRIVE_REMOVABLE))	// Ignore CD-ROMS and removable drives
			{
				UINT oldMode = SetErrorMode(0);
				SetErrorMode(oldMode | SEM_FAILCRITICALERRORS);  // Prevent the system from displaying message box
				result = GetDiskFreeSpaceEx(drive, NULL, (PULARGE_INTEGER)&i64TotalBytes, NULL);
				SetErrorMode(oldMode);  // Reset
			}
		}

		if (result)
		{
			m_MaxValue = (double)(__int64)i64TotalBytes;
			m_OldTotalBytes = i64TotalBytes;
		}
		else
		{
			m_MaxValue = 0.0;
			m_OldTotalBytes = 0;
		}
	}
	else
	{
		if (m_Type)
		{
			m_MaxValue = DRIVETYPE_MAX;
			m_OldTotalBytes = 0;
		}
		else
		{
			m_MaxValue = oldMaxValue;
		}
	}
}
