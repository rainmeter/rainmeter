/*
  Copyright (C) 2011 Birunthan Mohanathas

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

#ifndef SKININSTALLER_APPLICATION_H_
#define SKININSTALLER_APPLICATION_H_

#include <string>
#include "zip.h"
#include "unzip.h"

#define MAX_LINE_LENGTH		4096
#define MB_ERROR			MB_OK | MB_TOPMOST | MB_ICONERROR

struct GlobalData
{
	std::wstring programPath;
	std::wstring settingsPath;
	std::wstring skinsPath;
	std::wstring iniFile;
};

struct OsNameVersion
{
	const WCHAR* name;
	const WCHAR* version;
};

enum OSPLATFORM
{
	OSPLATFORM_UNKNOWN = 0,
	OSPLATFORM_XP,
	OSPLATFORM_VISTA,
	OSPLATFORM_7
};

bool CloseRainmeterIfActive();
OSPLATFORM GetOSPlatform();

bool IsRunning(const WCHAR* name, HANDLE* hMutex);
bool CopyFiles(const std::wstring& strFrom, const std::wstring& strTo, bool bMove = false);
std::string ConvertToAscii(LPCTSTR str);
std::wstring ConvertToWide(LPCSTR str);

#endif
