/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef SKININSTALLER_UTIL_H_
#define SKININSTALLER_UTIL_H_

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

HINSTANCE GetInstanceHandle();

bool CloseRainmeterIfActive();
OSPLATFORM GetOSPlatform();

bool IsRunning(const WCHAR* name, HANDLE* hMutex);
bool CopyFiles(const std::wstring& strFrom, const std::wstring& strTo, bool bMove = false);
std::string ConvertToAscii(LPCTSTR str);
std::wstring ConvertToWide(LPCSTR str);

#endif
