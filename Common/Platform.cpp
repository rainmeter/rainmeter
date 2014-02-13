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
#include <wchar.h>

namespace Platform {

LPCWSTR GetPlatformName(bool getExtendedInfo)
{
	static WCHAR buffer[256];
	wcscpy_s(buffer, L"");

	bool isServer = IsWindowsServer();

	OSVERSIONINFOEX osvi = { sizeof(OSVERSIONINFOEX) };
	if (GetVersionEx((OSVERSIONINFO*)&osvi))
	{
		// Note: Place newer versions at the top. Service pack helper functions are commented out for
		//       backward compatibility with the SysInfo plugin.

		if (IsWindows8Point1OrGreater())
		{
			wcscat_s(buffer, isServer ? L"Windows Server 2012 R2" : L"Windows 8.1");
		}
		else if (IsWindows8OrGreater())
		{
			wcscat_s(buffer, isServer ? L"Windows Server 2012" : L"Windows 8");
		}
		else if (IsWindows7OrGreater())
		{
			wcscat_s(buffer, isServer ? L"Windows Server 2008 R2" : L"Windows 7");
		}
		else if (IsWindowsVistaOrGreater())
		{
			wcscat_s(buffer, isServer ? L"Windows Server 2008" : L"Windows Vista");
		}
		else if (IsWindowsXPOrGreater())
		{
			if (GetSystemMetrics(SM_SERVERR2) != 0)
			{
				wcscat_s(buffer, L"Windows Server 2003 R2");
			}
			else if (osvi.wSuiteMask & VER_SUITE_WH_SERVER)
			{
				wcscat_s(buffer, L"Windows Home Server");
			}
			else
			{
				wcscat_s(buffer, isServer ? L"Windows Server 2003" : L"Windows XP");
			}
		}
		else
		{
			wcscat_s(buffer, L"Unknown");
		}

		if (getExtendedInfo && _wcsicmp(buffer, L"Unknown") != 0)
		{
			if (IsWindowsVistaOrGreater())
			{
				PGETPRODUCTINFO pGetProductInfo = (PGETPRODUCTINFO)
					GetProcAddress(GetModuleHandle(L"kernel32"), "GetProductInfo");
				if (pGetProductInfo)
				{
					DWORD dwType;
					if (pGetProductInfo(osvi.dwMajorVersion, osvi.dwMinorVersion,
						osvi.wServicePackMajor, osvi.wServicePackMinor, &dwType))
					{
						// Only use most common versions.
						// Full list at: http://msdn.microsoft.com/en-us/library/windows/desktop/ms724358%28v=vs.85%29.aspx
						switch (dwType)
						{
						case PRODUCT_ULTIMATE:
						case PRODUCT_ULTIMATE_N:
							wcscat_s(buffer, L" Ultimate"); break;

						case PRODUCT_PROFESSIONAL:
						case PRODUCT_PROFESSIONAL_N:
							wcscat_s(buffer, L" Professional"); break;

						case PRODUCT_PROFESSIONAL_WMC:
							wcscat_s(buffer, L" Professional Media Center"); break;

						case PRODUCT_HOME_PREMIUM:
						case PRODUCT_HOME_PREMIUM_N:
							wcscat_s(buffer, L" Home Premium"); break;

						case PRODUCT_HOME_BASIC:
						case PRODUCT_HOME_BASIC_N:
							wcscat_s(buffer, L" Home Basic"); break;

						case PRODUCT_STARTER:
						case PRODUCT_STARTER_N:
							wcscat_s(buffer, L" Starter"); break;

						case PRODUCT_ENTERPRISE:
						case PRODUCT_ENTERPRISE_EVALUATION:
						case PRODUCT_ENTERPRISE_N:
						case PRODUCT_ENTERPRISE_N_EVALUATION:
							wcscat_s(buffer, L" Enterprise"); break;

						case PRODUCT_BUSINESS:
						case PRODUCT_BUSINESS_N:
							wcscat_s(buffer, L" Business"); break;

						// Server Editions
						case PRODUCT_CLUSTER_SERVER:
						case PRODUCT_CLUSTER_SERVER_V:
							wcscat_s(buffer, L" Cluster Edition"); break;

						case PRODUCT_DATACENTER_SERVER:
						case PRODUCT_DATACENTER_SERVER_V:
						case PRODUCT_DATACENTER_SERVER_CORE:
						case PRODUCT_DATACENTER_SERVER_CORE_V:
						case PRODUCT_DATACENTER_EVALUATION_SERVER:
							wcscat_s(buffer, L" Datacenter Edition"); break;

						case PRODUCT_ENTERPRISE_SERVER:
						case PRODUCT_ENTERPRISE_SERVER_V:
						case PRODUCT_ENTERPRISE_SERVER_CORE:
						case PRODUCT_ENTERPRISE_SERVER_CORE_V:
						case PRODUCT_ENTERPRISE_SERVER_IA64:
							wcscat_s(buffer, L" Enterprise Edition"); break;

						case PRODUCT_SMALLBUSINESS_SERVER:
						case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
						case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM_CORE:
							wcscat_s(buffer, L" Small Business Server"); break;

						case PRODUCT_STANDARD_SERVER:
						case PRODUCT_STANDARD_SERVER_V:
						case PRODUCT_STANDARD_SERVER_CORE:
						case PRODUCT_STANDARD_SERVER_CORE_V:
							wcscat_s(buffer, L" Standard Edition"); break;

						case PRODUCT_WEB_SERVER:
						case PRODUCT_WEB_SERVER_CORE:
							wcscat_s(buffer, L" Web Server"); break;
						}
					}
				}
			}
			else if (IsWindowsXPOrGreater())
			{
				if (isServer)
				{
					if (osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER)
					{
						wcscat_s(buffer, L" Compute Cluster Edition");
					}
					else if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
					{
						wcscat_s(buffer, L" Datacenter Edition");
					}
					else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
					{
						wcscat_s(buffer, L" Enterprise Edition");
					}
					else if (osvi.wSuiteMask & VER_SUITE_BLADE)
					{
						wcscat_s(buffer, L" Web Edition");
					}
					else
					{
						wcscat_s(buffer, L" Standard Edition");
					}
				}
				else
				{
					if (osvi.wSuiteMask & VER_SUITE_PERSONAL)
					{
						wcscat_s(buffer, L" Home Edition");
					}
					else if (GetSystemMetrics(SM_MEDIACENTER))
					{
						wcscat_s(buffer, L" Media Center Edition");
					}
					else if (GetSystemMetrics(SM_STARTER))
					{
						wcscat_s(buffer, L" Starter Edition");
					}
					else if (GetSystemMetrics(SM_TABLETPC))
					{
						wcscat_s(buffer, L" Tablet PC Edition");
					}
					else
					{
						return L" Professional";
					}
				}
			}

			// Build number and 32/64 bit
			bool is64Bit = false;
			bool has64Bit = Platform::GetPlatformBit(is64Bit);
			_snwprintf_s(buffer, _TRUNCATE, L"%s %s-bit (build %d)",
				buffer,
				(has64Bit) ? (is64Bit) ? L"64" : L"32" : L"???",
				osvi.dwBuildNumber);

			// Service Pack
			if (osvi.szCSDVersion)
			{
				wcscat_s(buffer, L", ");
				wcscat_s(buffer, osvi.szCSDVersion);
			}
		}
	}

	return buffer;
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