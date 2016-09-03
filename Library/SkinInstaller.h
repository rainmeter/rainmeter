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

HINSTANCE GetInstanceHandle();

bool CloseRainmeterIfActive();

bool IsRunning(const WCHAR* name, HANDLE* hMutex);

#endif
