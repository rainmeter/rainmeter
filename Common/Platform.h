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

#ifndef RM_COMMON_PLATFORM_H_
#define RM_COMMON_PLATFORM_H_

namespace Platform {

typedef BOOL(WINAPI * LPFN_ISWOW64PROCESS)(HANDLE hProcess, PBOOL Wow64Process);
typedef BOOL(WINAPI * PGETPRODUCTINFO)(DWORD dwOSMajorVersion,
	DWORD dwOSMinorVersion,
	DWORD dwSpMajorVersion,
	DWORD dwSpMinorVersion,
	PDWORD pdwReturnedProductType);

LPCWSTR GetPlatformName(bool getExtendedInfo = false);
bool GetPlatformBit(bool& is64Bit);

}  // namespace Platform

#endif
