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
#include "Platform.h"

namespace Platform {

Version GetVersion()
{
	static Version s_Version = ([]()
	{
		OSVERSIONINFOEX osvi = {sizeof(OSVERSIONINFOEX)};
		if (GetVersionEx((OSVERSIONINFO*)&osvi))
		{
			switch (osvi.dwMajorVersion)
			{
			case 5:
				// Not checking for osvi.dwMinorVersion >= 1 because Rainmeter won't run on pre-XP.
				return Version::WinXP;

			case 6:
				switch (osvi.dwMinorVersion)
				{
				case 0:
					return Version::WinVista;  // Vista, Server 2008

				case 1:
					return Version::Win7;  // 7, Server 2008R2

				default:
					return Version::Win8;  // 8, Server 2012
				}
				break;
			}
		}

		return Version::Win8;  // newer OS
	})();

	return s_Version;
}

LPCWSTR GetPlatformName()
{
	OSVERSIONINFOEX osvi = { sizeof(OSVERSIONINFOEX) };
	if (GetVersionEx((OSVERSIONINFO*)&osvi))
	{
		if (osvi.dwMajorVersion == 5)
		{
			if (osvi.dwMinorVersion == 2)
			{
				return L"Windows 2003";
			}
			else if (osvi.dwMinorVersion == 1)
			{
				return L"Windows XP";
			}
		}
		else
		{
			if (osvi.dwMinorVersion == 3 && osvi.wProductType == VER_NT_WORKSTATION)
			{
				return L"Windows 8.1";
			}
			else if (osvi.dwMinorVersion == 3 && osvi.wProductType != VER_NT_WORKSTATION)
			{
				return L"Windows Server 2012 R2";
			}
			else if (osvi.dwMinorVersion == 2 && osvi.wProductType == VER_NT_WORKSTATION)
			{
				return L"Windows 8";
			}
			else if (osvi.dwMinorVersion == 2 && osvi.wProductType != VER_NT_WORKSTATION)
			{
				return L"Windows Server 2012";
			}
			else if (osvi.dwMinorVersion == 1 && osvi.wProductType == VER_NT_WORKSTATION)
			{
				return L"Windows 7";
			}
			else if (osvi.dwMinorVersion == 1 && osvi.wProductType != VER_NT_WORKSTATION)
			{
				return L"Windows Server 2008 R2";
			}
			else if (osvi.dwMinorVersion == 0 && osvi.wProductType == VER_NT_WORKSTATION)
			{
				return L"Windows Vista";
			}
			else if (osvi.dwMinorVersion == 0 && osvi.wProductType != VER_NT_WORKSTATION)
			{
				return L"Windows Server 2008";
			}
		}
	}

	return L"Unknown";
}

/*
** Returns |true| if the OS architecture can be found (either 32-bit or 64-bit),
**  or |false| if it cannot be determined.
**
** Note: IsWow64Process was introduced with Windows XP SP2.
*/
bool GetPlatformBit(bool& is64Bit)
{
#if _WIN64

	is64Bit = true;
	return true;

#elif _WIN32

	BOOL isWow64 = FALSE;

	LPFN_ISWOW64PROCESS fnIsWow64Process = 
		(LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(L"kernel32"), "IsWow64Process");

	if (fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &isWow64))
		{
			return false;
		}

		if (isWow64)
		{
			is64Bit = true;
		}
		else
		{
			is64Bit = false;
		}

		return true;
	}
	else
	{
		return false;
	}

#else

	return false;

#endif
}

}  // namespace Platform