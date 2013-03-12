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

enum Version
{
	WinXP,
	WinVista,
	Win7,
	Win8
};

Version GetVersion();

#define RM_PLATFORM_DECLARE_HELPERS(ver) \
	inline bool IsAtMost ## ver() { return GetVersion() <= ver; } \
	inline bool Is ## ver() { return GetVersion() == ver; } \
	inline bool IsAtLeast ## ver() { return GetVersion() >= ver; } \

RM_PLATFORM_DECLARE_HELPERS(WinXP)
RM_PLATFORM_DECLARE_HELPERS(WinVista)
RM_PLATFORM_DECLARE_HELPERS(Win7)
RM_PLATFORM_DECLARE_HELPERS(Win8)

}  // namespace Platform

#endif
