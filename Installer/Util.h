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

#ifndef RM_INSTALLER_UTIL_H_
#define RM_INSTALLER_UTIL_H_

namespace Util {

bool IsSystem64Bit();

bool IsProcessUserAdmin();
bool CanProcessUserElevate();

bool CopyDirectory(const WCHAR* fromPath, const WCHAR* toPath);
bool CreateShortcutFile(const WCHAR* filePath, const WCHAR* targetPath);

bool GetRegistryDword(HKEY rootKey, const WCHAR* subKey, const WCHAR* value, DWORD* data);
bool GetRegistryString(HKEY rootKey, const WCHAR* subKey, const WCHAR* value, WCHAR* data, DWORD dataSize);
bool SetRegistryDword(HKEY rootKey, const WCHAR* subKey, const WCHAR* value, DWORD data);
bool SetRegistryString(HKEY rootKey, const WCHAR* subKey, const WCHAR* value, const WCHAR* data);

bool DownloadFile(const WCHAR* url, const WCHAR* file);

}  // namespace Util

#endif