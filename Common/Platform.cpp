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

#include "Platform.h"
#include <Windows.h>

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

}  // namespace Platform