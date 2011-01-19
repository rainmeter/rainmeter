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
	m_OldTotalBytes = 0;
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
		BOOL sizeResult = FALSE;
		ULARGE_INTEGER i64TotalBytes, i64FreeBytes;

		UINT type = GetDriveType(m_Drive.c_str());
		if (type != DRIVE_NO_ROOT_DIR)
		{
			if (type != DRIVE_CDROM && (!m_IgnoreRemovable || type != DRIVE_REMOVABLE))	// Ignore CD-ROMS and removable drives
			{
				ULARGE_INTEGER i64FreeBytesToCaller;

				UINT oldMode = SetErrorMode(0);
				SetErrorMode(oldMode | SEM_FAILCRITICALERRORS);  // Prevent the system from displaying message box
				SetLastError(ERROR_SUCCESS);
				sizeResult = GetDiskFreeSpaceEx(m_Drive.c_str(), &i64FreeBytesToCaller, &i64TotalBytes, &i64FreeBytes);
				SetErrorMode(oldMode);  // Reset
			}
		}

		if (sizeResult)
		{
			m_Value = (double)((m_Total) ? i64TotalBytes : i64FreeBytes).QuadPart;

			if (i64TotalBytes.QuadPart != m_OldTotalBytes)
			{
				// Total size was changed, so set new max value.
				m_MaxValue = (double)i64TotalBytes.QuadPart;
				m_OldTotalBytes = i64TotalBytes.QuadPart;
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
			WCHAR volumeName[MAX_PATH] = {0};

			if (type != DRIVE_NO_ROOT_DIR)
			{
				if (!m_IgnoreRemovable || type != DRIVE_REMOVABLE)	// Ignore removable drives
				{
					UINT oldMode = SetErrorMode(0);
					SetErrorMode(oldMode | SEM_FAILCRITICALERRORS);  // Prevent the system from displaying message box
					labelResult = GetVolumeInformation(m_Drive.c_str(), volumeName, MAX_PATH, NULL, NULL, NULL, NULL, 0);
					SetErrorMode(oldMode);  // Reset
				}
			}

			m_LabelName = (labelResult) ? volumeName : L"";
		}
		else if (!m_LabelName.empty())
		{
			m_LabelName = L"";
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
const WCHAR* CMeasureDiskSpace::GetStringValue(AUTOSCALE autoScale, double scale, int decimals, bool percentual)
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
	double oldMaxValue = m_MaxValue;

	CMeasure::ReadConfig(parser, section);

	m_Drive = parser.ReadString(section, L"Drive", L"C:\\");
	if (m_Drive.empty())
	{
		Log(LOG_WARNING, L"Drive path is not given.");
		m_Value = 0.0;
		m_MaxValue = 0.0;
		m_OldTotalBytes = 0;
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
	if (!m_Initialized)
	{
		BOOL result = FALSE;
		ULARGE_INTEGER i64TotalBytes;

		if (!m_Drive.empty())
		{
			UINT type = GetDriveType(m_Drive.c_str());
			if (type != DRIVE_NO_ROOT_DIR &&
				type != DRIVE_CDROM && (!m_IgnoreRemovable || type != DRIVE_REMOVABLE))	// Ignore CD-ROMS and removable drives
			{
				ULARGE_INTEGER i64FreeBytesToCaller, i64FreeBytes;

				UINT oldMode = SetErrorMode(0);
				SetErrorMode(oldMode | SEM_FAILCRITICALERRORS);  // Prevent the system from displaying message box
				result = GetDiskFreeSpaceEx(m_Drive.c_str(), &i64FreeBytesToCaller, &i64TotalBytes, &i64FreeBytes);
				SetErrorMode(oldMode);  // Reset
			}
		}

		if (result)
		{
			m_MaxValue = (double)i64TotalBytes.QuadPart;
			m_OldTotalBytes = i64TotalBytes.QuadPart;
		}
		else
		{
			m_MaxValue = 0.0;
			m_OldTotalBytes = 0;
		}
	}
	else
	{
		m_MaxValue = oldMaxValue;
	}
}
