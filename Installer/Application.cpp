/*
  Copyright (C) 2013 Birunthan Mohanathas

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
#include "Application.h"
#include "DialogInstall.h"
#include "Install.h"
#include "Resource.h"

bool IsSupportedPlatform();
bool IsSupportedCPU();

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR cmdLine, int)
{
	CoInitialize(nullptr);
	InitCommonControls();

	if (*cmdLine)
	{
		InstallOptions options;
		const int scans = swscanf(
			cmdLine, L"OPT:%259[^|]|%ld|%hd|%hd|%d",
			options.targetPath,
			&options.language,
			&options.type,
			&options.arch,
			&options.launchOnLogin);
		if (scans == 5)
		{
			DoInstall(options);
			return 0;
		}

		return 1;
	}

	if (!IsSupportedPlatform())
	{
		MessageBox(
			nullptr,
			L"Windows XP SP2 or higher is required to install Rainmeter.",
			L"Rainmeter Setup", MB_OK | MB_ICONERROR);
		return (int)InstallStatus::UnsupportedPlatform;
	}

	if (!IsSupportedCPU())
	{
		MessageBox(
			nullptr,
			L"A Pentium III or later processor is required to install Rainmeter.",
			L"Rainmeter Setup", MB_OK | MB_ICONERROR);
		return (int)InstallStatus::UnsupportedPlatform;
	}

	CDialogInstall::Create();

	return 0;
}

bool IsSupportedPlatform()
{
	OSVERSIONINFOEX osvi = { sizeof(OSVERSIONINFOEX) };
	GetVersionEx((OSVERSIONINFO*)&osvi);

	if (osvi.wProductType != VER_NT_WORKSTATION ||
		(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0))
	{
		// 2000 or lower.
		return false;
	}

	if (osvi.dwMajorVersion == 5 &&	osvi.dwMinorVersion == 1 &&
		osvi.wServicePackMajor <= (GetSystemMetrics(SM_SERVERR2) == 0 ? 0 : 1))
	{
		// XP SP1 or lower, 2003 SP0.
		return FALSE;
	}

	return TRUE;
}

bool IsSupportedCPU()
{
	return IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE) != 0;
}
