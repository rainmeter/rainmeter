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
#include "Install.h"
#include "Resource.h"
#include "Application.h"

bool IsSupportedPlatform();
bool IsSupportedCPU();

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	int argCount = 0;
	LPWSTR* args = CommandLineToArgvW(GetCommandLine(), &argCount);
	if (args)
	{
		for (int i = 1; i < argCount; ++i)
		{
			WCHAR* name = &args[i][(args[i][0] == L'/') ? 1 : 0];
			WCHAR* value = wcschr(name, L':');
			if (value)
			{
				*value = L'\0';
				++value;
			}

			if (wcscmp(name, L"Silent") == 0)
			{
			}
		}

		if (argCount >= 2 && wcscmp(args[1], L"/ElevatedInstall") == 0)
		{
		}

		LocalFree(args);
	}

	InitCommonControls();

	if (!IsSupportedPlatform())
	{
		MessageBox(nullptr, L"Windows XP SP2 or higher is required to install Rainmeter.", nullptr, MB_OK | MB_ICONERROR);
		return (int)InstallStatus::UnsupportedPlatform;
	}

	if (!IsSupportedCPU())
	{
		MessageBox(nullptr, L"A Pentium III or later processor is required to install Rainmeter.", nullptr, MB_OK | MB_ICONERROR);
		return (int)InstallStatus::UnsupportedPlatform;
	}

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
